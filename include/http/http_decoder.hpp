#pragma once
#include "utils/knet_log.hpp"
#include "http_parser.hpp"
#include "utils/knet_log.hpp"
 
using namespace knet::log; 
namespace knet
{
	namespace http
	{

		template <typename XCoder, typename T>
		struct HttpParseHelper
		{

			static int parse_status(http_parser *parser, const char *pos, size_t len)
			{
				return 0;
			}
			static int parse_header_complete(http_parser *parser)
			{
				return 0;
			}

			static int parse_url(http_parser *parser, const char *pos, size_t length)
			{
			 
				return 0;
			}
		};

		class HttpRequest;

		template <typename XCoder>
		struct HttpParseHelper<XCoder, HttpRequest>
		{
			static int parse_status(http_parser *parser, const char *pos, size_t len)
			{				
				return 0;
			}
			
			static int parse_url(http_parser *parser, const char *pos, size_t length)
			{
				XCoder *self = static_cast<XCoder *>(parser->data);
				// knet_dlog("handle url callback {} ", std::string(pos, length));
				if (self)
				{
					self->request_url = std::string(pos, length);
					// knet_dlog("parsed request url is {}", self->request_url);
					self->http_message->http_url = std::string(pos, length);
				}
				return 0;
			}
			static int parse_header_complete(http_parser *parser)
			{
				XCoder *self = (XCoder *)parser->data;
				struct http_parser_url urlInfo;

				const int result =
					http_parser_parse_url(self->request_url.data(), self->request_url.size(), 0, &urlInfo);
				if (result != 0)
				{
					knet_elog("parser url failed");
					return 0;
				}

				if (!(urlInfo.field_set & (1 << UF_PATH)))
				{
					knet_elog("parse path failed");
					return -1;
				}

				self->http_path = std::string_view(self->request_url.data() + urlInfo.field_data[UF_PATH].off,
												   urlInfo.field_data[UF_PATH].len);

				knet_dlog("http request path is {}", self->http_path);

				if (urlInfo.field_set & (1 << UF_QUERY))
				{
					self->http_query = std::string_view(self->request_url.data() + urlInfo.field_data[UF_QUERY].off,
														urlInfo.field_data[UF_QUERY].len);
				}
				self->http_message->method_value = (enum http_method)parser->method;
				return 0;
			}
		};

		class HttpResponse;

		template <typename XCoder>
		struct HttpParseHelper<XCoder, HttpResponse>
		{
			static int parse_status(http_parser *parser, const char *pos, size_t len)
			{
				XCoder *self = (XCoder *)parser->data;
				self->http_status = std::string_view(pos, len);
				self->http_message->status_code = parser->status_code;
				return 0;
			}
			static int parse_url(http_parser *parser, const char *pos, size_t length)
			{
				return 0;
			}
			static int parse_header_complete(http_parser *parser)
			{
				return 0;
			}
		};

		template <class T>
		class HttpDecoder
		{

		public:
			struct Header
			{
				std::string_view key;
				std::string_view value;
			};

			using HttpCoder = HttpDecoder<T>;

			HttpDecoder(T *msg = nullptr) : http_message(msg), input_data(nullptr), input_len(0) {} 

			uint32_t parse_request(const char *data, uint32_t len, bool inplace = false)
			{
				inplace_flag = inplace;
				input_data = data;
				input_len = len;
				init_parser(HTTP_REQUEST);
 
				if (inplace)
				{
					return http_parser_execute(&msg_parser, &parser_setting, data, len);
				}
				else
				{
					raw_data = std::string(data, len);
					size_t msgLen = http_parser_execute(&msg_parser, &parser_setting, raw_data.data(), len);
					if (msgLen < len)
					{
						raw_data.resize(msgLen);
					}
					return msgLen;
				}
			}

			uint32_t parse_response(const char *data, uint32_t len, bool inplace = true)
			{
				inplace_flag = inplace;
				input_data = data;
				input_len = len;
				init_parser(HTTP_RESPONSE); 

				if (inplace)
				{
					return http_parser_execute(&msg_parser, &parser_setting, data, len);
				}
				else
				{
					raw_data = std::string(data, len);
					size_t msgLen = http_parser_execute(&msg_parser, &parser_setting, raw_data.data(), len);
					if (msgLen < len)
					{
						raw_data.resize(msgLen);
					}
					return msgLen;
				}
			}

			// static int parse_status(http_parser* parser, const char* pos, size_t len) {
			// 	HttpDecoder* self = (HttpDecoder*)parser->data;
			// 	self->http_status = std::string_view(pos, len);
			// 	self->http_message->status_code = parser->status_code;
			// 	return 0;
			// }

			// static int parse_url(http_parser* parser, const char* pos, size_t length) {
			// 	HttpDecoder* self = static_cast<HttpDecoder*>(parser->data);
			// 	knet_dlog("handle url callback {} ", std::string(pos, length));
			// 	if (self) {
			// 		self->request_url = std::string(pos, length);
			// 		knet_dlog("parsed request url is {}", self->request_url);
			// 		self->http_message->uri = std::string(pos, length);
			// 	}
			// 	return 0;
			// }

			static int parse_field(http_parser *hp, const char *at, size_t len)
			{
				HttpDecoder *self = (HttpDecoder *)hp->data;
				self->parse_header.key = std::string_view(at, len);
				knet_dlog("parse field {}", self->parse_header.key);
				return 0;
			}

			static int parse_value(http_parser *hp, const char *at, size_t len)
			{
				HttpDecoder *self = (HttpDecoder *)hp->data;
				self->parse_header.value = std::string_view(at, len);
				self->headers.emplace_back(self->parse_header);
				knet_dlog("parse field {}", self->parse_header.value);
				return 0;
			}

			static int parse_body(http_parser *hp, const char *pos, size_t len)
			{
				HttpDecoder *self = (HttpDecoder *)hp->data;
				self->http_body = std::string_view(pos, len);
				return 0;
			}

			// static int parse_header_complete(http_parser* hp) {
			// 	HttpDecoder* self = (HttpDecoder*)hp->data;
			// 	struct http_parser_url urlInfo;

			// 	const int result =
			// 		http_parser_parse_url(self->request_url.data(), self->request_url.size(), 0, &urlInfo);
			// 	if (result != 0) {
			// 		knet_elog("parser url failed");
			// 		return 0;
			// 	}

			// 	if (!(urlInfo.field_set & (1 << UF_PATH))) {
			// 		knet_elog("parse path failed");
			// 		return -1;
			// 	}

			// 	self->http_path =  std::string_view(self->request_url.data() + urlInfo.field_data[UF_PATH].off,
			// 			urlInfo.field_data[UF_PATH].len);

			// 	knet_dlog("http request path is {}", self->http_path);

			// 	if (urlInfo.field_set & (1 << UF_QUERY)) {
			// 		self->http_query = std::string_view(self->request_url.data() + urlInfo.field_data[UF_QUERY].off,
			// 				urlInfo.field_data[UF_QUERY].len);
			// 	}

			// 	return 0;
			// }

			static int parse_message_begin(http_parser *hp)
			{
				knet_dlog("parse message begin");
				return 0;
			}
			static int parse_message_complete(http_parser *hp)
			{
				knet_dlog("parse message complete");
				return 0;
			}

			static int parse_chunk_header(http_parser *hp)
			{
				knet_dlog("chunk content length {}", hp->content_length);
				return 0;
			}

			static int parse_chunk_complete(http_parser *hp) { return 0; }

			bool is_websocket() const
			{
				return msg_parser.upgrade;
			}
			std::string_view get_header(const std::string &key) const 
			{
				for (auto &item : headers)
				{
					if (item.key == key)
					{
						return item.value;
					}
				}
				return std::string_view();
			}

			std::string request_url;

			std::string_view http_status;
			std::string_view http_body;
			std::string_view http_path;
			std::string_view http_query;
			std::vector<Header> headers;

			T *http_message;

		protected:
			void init_parser(enum http_parser_type msgType )
			{
				http_parser_settings_init(&parser_setting);
				parser_setting.on_url = &HttpParseHelper<HttpCoder, T>::parse_url;
				parser_setting.on_status = &HttpParseHelper<HttpCoder, T>::parse_status;
				parser_setting.on_body = parse_body;
				parser_setting.on_header_field = parse_field;
				parser_setting.on_header_value = parse_value;
				parser_setting.on_headers_complete = &HttpParseHelper<HttpCoder, T>::parse_header_complete;
				parser_setting.on_message_begin = parse_message_begin;
				parser_setting.on_message_complete = parse_message_complete;
				parser_setting.on_chunk_header = parse_chunk_header;
				parser_setting.on_chunk_complete = parse_chunk_complete;
				
				http_parser_init(&msg_parser, msgType);
				msg_parser.data = this;
			}
			std::string raw_data;
			Header parse_header;
			const char *input_data;
			uint32_t input_len;
			bool inplace_flag = false;
			http_parser msg_parser = {};
			http_parser_settings parser_setting;
		};

	} // namespace http
} // namespace knet
