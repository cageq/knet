#pragma once
#include <memory>
#include "http_message.hpp"
#include "http_decoder.hpp"

namespace knet {
namespace http {

class HttpResponse {

public:
	HttpResponse(const std::string& rsp = "", uint32_t c = 200) {

		this->status_code = c;
		http_encoder = std::make_unique<HttpEncoder<HttpResponse>>(*this);
		http_encoder->content = rsp;
		http_encoder->status_code = c;
	}
	HttpResponse(uint32_t c, const std::string rsp = "") {
		this->status_code = c;
		http_encoder = std::make_unique<HttpEncoder<HttpResponse>>(*this);
		http_encoder->content = rsp;
		http_encoder->status_code = c;
	}

	inline uint32_t parse_response(const char* data, uint32_t len, bool inplace = false) {
		if (!http_decoder)
		{
			http_decoder =  std::make_unique<HttpDecoder<HttpResponse>>(*this);
		}
		return http_decoder->parse_response(data, len, inplace);
	}

	void add_header(const std::string& key, const std::string& value)
	{
		if (http_encoder)
		{
			http_encoder->add_header(key,value); 
		}
	}

	uint32_t code() const { return status_code; }

	 inline std::string to_string() const { return http_encoder->encode(); }

	bool is_websocket() const {
		if (http_decoder) {
			return http_decoder->is_websocket();
		}
		return false;
	};

	std::string body() { return http_encoder->content; }
	std::string content;
	uint32_t status_code = 200;
	std::string uri;

	std::unique_ptr<HttpDecoder<HttpResponse>> http_decoder;
	std::unique_ptr<HttpEncoder<HttpResponse>> http_encoder;
};

} // namespace http
} // namespace knet
