#pragma once
#include "utils/knet_log.hpp"

#ifdef USE_LLHTTP_PRASER 
#include "llhttp.h" 
using http_parser = llhttp_t; 
using  http_parser_settings  = llhttp_settings_t; 
#else 
#include "http_parser.hpp"
#endif 

 

namespace knet {
namespace http {

template <class T>
class HttpDecoder {

public:


	struct Header {
		std::string_view key;
		std::string_view value;
	};
	
	HttpDecoder(T & msg):http_message(msg) {}

	HttpDecoder(T & msg , const char* data, uint32_t len, bool inplace = false):http_message(msg) {

		input_data = data; 
		input_len = len; 
		inplace_flag = inplace; 
	}

	uint32_t parse_request(){ 
		return parse_request(input_data, input_len, inplace_flag); 
	}
	uint32_t parse_response(){ 
		return parse_response(input_data, input_len, inplace_flag); 
	}

	uint32_t parse_request(const char* data, uint32_t len, bool inplace = false) {
		init_parser(HTTP_REQUEST); 
		//http_parser_init(&parser_, HTTP_REQUEST);
		parser_.data = this;

		if (inplace) {
			return parser_execute( data, len);
		} else {
			raw_data = std::string(data, len);
			size_t msgLen = parser_execute( raw_data.data(), len);
			raw_data.resize(msgLen);
			return msgLen;
		}
	}

	uint32_t parse_response(const char* data, uint32_t len, bool inplace = false) {
		init_parser(HTTP_RESPONSE); 
		//http_parser_init(&parser_, HTTP_RESPONSE);
		parser_.data = this;
		if (inplace) {
			return parser_execute(data, len);
		} else {
			raw_data = std::string(data, len);
			size_t msgLen = parser_execute( data, len);
			raw_data.resize(msgLen);
			return msgLen;
		}
	}

	int32_t parser_execute(const char * data, uint32_t len) {

#ifdef USE_LLHTTP_PRASER 
		enum llhttp_errno err  = llhttp_execute(&parser_, data , len );
		if (err == HPE_OK){
			return strlen(data) ; 
		}
		return 0; 
#else 
		return http_parser_execute(&parser_, &parser_setting, data, len);
#endif 
	}

	static int parse_status(http_parser* parser, const char* pos, size_t len) {
		HttpDecoder* self = (HttpDecoder*)parser->data;
		self->http_status = std::string_view(pos, len);
		self->status_code = parser->status_code;
		return 0;
	}

	static int parse_url(http_parser* parser, const char* pos, size_t length) {

		HttpDecoder* self = static_cast<HttpDecoder*>(parser->data);
		dlog("handle url callback {} ", std::string(pos, length));
		if (self) {
			self->request_url = std::string(pos, length);
			self->http_path = self->request_url; 
			dlog("parsed request url is {}", self->request_url); 
			self->http_message.uri = std::string(pos, length);
		}
		return 0;
	}

 

	static int parse_field(http_parser* hp, const char* at, size_t len) {
		HttpDecoder* self = (HttpDecoder*)hp->data;
		self->parse_header.key = std::string_view(at, len);
		dlog("parse field {}", self->parse_header.key);

		return 0;
	}

	static int parse_value(http_parser* hp, const char* at, size_t len) {
		HttpDecoder* self = (HttpDecoder*)hp->data;
		self->parse_header.value = std::string_view(at, len);
		self->headers.emplace_back(self->parse_header);
		dlog("parse field {}", self->parse_header.value);
		return 0;
	}

	static int parse_body(http_parser* hp, const char* pos, size_t len) {
		HttpDecoder* self = (HttpDecoder*)hp->data;
		self->http_body = std::string_view(pos, len);
		return 0;
	}

	static int parse_header_complete(http_parser* hp) {
		HttpDecoder* self = (HttpDecoder*)hp->data;
		//struct llhttp_url_t urlInfo;
//
//		const int result =
//			http_parser_parse_url(self->request_url.data(), self->request_url.size(), 0, &urlInfo);
//		if (result != 0) {
//			elog("parser url failed"); 
//			return 0;
//		}
//
//		if (!(urlInfo.field_set & (1 << UF_PATH))) {
//			elog("parse path failed");
//			return -1;
//		}
//
//		self->http_path =  std::string_view(self->request_url.data() + urlInfo.field_data[UF_PATH].off,
//				urlInfo.field_data[UF_PATH].len);
//
//		dlog("http request path is {}", self->http_path); 
//
//		if (urlInfo.field_set & (1 << UF_QUERY)) {
//			self->http_query = std::string_view(self->request_url.data() + urlInfo.field_data[UF_QUERY].off,
//					urlInfo.field_data[UF_QUERY].len);
//		}

		return 0;
	}

	static int parse_message_begin(http_parser* hp) {
		dlog("parse message begin");
		return 0;
	}
	static int parse_message_complete(http_parser* hp) {
		dlog("parse message complete");
		return 0;
	}

	static int parse_chunk_header(http_parser* hp) {
		dlog("chunk content length {}", hp->content_length);
		return 0;
	}
	
	static int parse_chunk_complete(http_parser* hp) { return 0; }


	bool is_websocket(){ 
		return parser_.upgrade; 
	} 
	std::string_view get_header(const std::string &  key){
		for(auto & item :headers)
		{
			if (item.key == key)
			{
				return item.value; 
			}

		}
		return std::string_view(); 
	}

	std::string request_url;

	uint32_t status_code = 200;
	std::string_view http_status;
	std::string_view http_body;
	std::string_view http_path;
	std::string_view http_query;
	std::vector<Header> headers;

	T  & http_message; 
protected:


	void init_parser(int msgType ) {
#ifdef USE_LLHTTP_PRASER 
		llhttp_settings_init(&parser_setting);
#else 
		http_parser_settings_init(&parser_setting);
#endif 
		parser_setting.on_url = &HttpDecoder::parse_url;
		parser_setting.on_status = parse_status;
		parser_setting.on_body = parse_body;
		parser_setting.on_header_field = parse_field;
		parser_setting.on_header_value = parse_value;
		parser_setting.on_headers_complete = parse_header_complete;
		parser_setting.on_message_begin = parse_message_begin;
		parser_setting.on_message_complete = parse_message_complete;
		parser_setting.on_chunk_header = parse_chunk_header;
		parser_setting.on_chunk_complete = parse_chunk_complete;


#ifdef USE_LLHTTP_PRASER 
		llhttp_init(&parser_, HTTP_BOTH, &parser_setting);
#else 
		http_parser_init(&parser_, HTTP_REQUEST);
#endif 
	}
	std::string raw_data;
	Header parse_header;

	const char* input_data = nullptr; 
	uint32_t  input_len = 0; 
	bool inplace_flag = false; 

	http_parser parser_ = {0};
	http_parser_settings parser_setting;
};

} // namespace http
} // namespace knet
