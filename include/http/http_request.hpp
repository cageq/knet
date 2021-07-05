#pragma once

#include "http_decoder.hpp"
#include "http_encoder.hpp"
#include "http_response.hpp"

#include <string>
#include <memory>
#include <functional>

namespace knet {
namespace http {
 
struct HttpRequest {
public:
	HttpRequest() {}
	HttpRequest(HttpMethod method, const std::string& url, const std::string& content = "",
		const std::string& type = "txt") {
		if (!http_encoder) {
			http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
		}
		http_encoder->set_method(method);
		http_encoder->set_url(url);
		if (!content.empty())
		{
			http_encoder->set_content(content);
			http_encoder->set_content_type(mime_types::to_mime(type));
		}
		
	}
	HttpRequest(const std::string& url, const std::string& query = "") {
		if (!http_encoder) {
			http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
		}
		http_encoder->http_url = url;
	}

	HttpRequest(const char* data, uint32_t len, bool inplace = false) {
		http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
	}

	uint32_t parse_request(const char* data, uint32_t len, bool inplace = false) {
		if (!http_decoder) {
			http_decoder = std::make_unique<HttpDecoder<HttpRequest>>(*this);
		}

		return http_decoder->parse_request(data, len, inplace);
	}
	std::string encode() {
		if (!http_encoder) {
			http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
		}
		return http_encoder->encode();
	}
	std::string encode(HttpMethod method, const std::string& url, const std::string& body,
		const std::string& type = "txt") {
		if (!http_encoder) {
			http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
		}
		return http_encoder->encode_request(method, url, body, type);
	}

	void add_header(const std::string& key, const std::string& value) {
		if (!http_encoder) {
			http_encoder = std::make_unique<HttpEncoder<HttpRequest>>(*this);
		}
		http_encoder->add_header(key, value);
	}
	std::string_view get_header(const std::string& key) {
		if (http_decoder) {
			return http_decoder->get_header(key);
		}
		return std::string_view();
	}

	// std::string url() const { return http_decoder->request_url; }
	inline std::string url() const { return uri; }

	inline uint32_t code() const { return http_decoder->status_code; }

	std::string to_string() const {
		if (http_encoder) {
			return http_encoder->encode();
		}
		return "";
	}
	void reply(const std::string &msg, uint32_t code = 200){
		if(replier){
			replier(HttpResponse(msg, code )); 
		}
	}

	
	bool is_websocket() const {
		if (http_decoder) {
			return http_decoder->is_websocket();
		}
		return false;
	};
	
	std::function<void(const HttpResponse&)> replier;

	std::string method;
	std::string uri;

	int http_version_major = 1;
	int http_version_minor = 0;

	std::unique_ptr<HttpDecoder<HttpRequest>> http_decoder;
	std::unique_ptr<HttpEncoder<HttpRequest>> http_encoder;
};

using HttpRequestPtr = std::shared_ptr<HttpRequest>;

} // namespace http
} // namespace knet
