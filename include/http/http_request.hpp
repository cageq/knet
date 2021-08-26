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
					http_encoder.init_http_message(this); 
					http_encoder.set_method(method);
					http_encoder.set_url(url);
					http_encoder.set_content(content, type); 
				}

				HttpRequest(const std::string& url, const std::string& query = "") {
					http_encoder.init_http_message(this); 
					http_encoder.http_url = url;
				}

				HttpRequest(const char* data, uint32_t len, bool inplace = false) {
					http_encoder.init_http_message(this); 
				}

				inline std::string encode() const {
					return http_encoder.encode();
				}

		
				void add_header(const std::string& key, const std::string& value) {
					http_encoder.add_header(key, value);
				}


				uint32_t parse(const char* data, uint32_t len, bool inplace = false) {
					http_decoder.init_http_message(this); 
					return http_decoder.parse_request(data, len, inplace);
				}

				std::string_view get_header(const std::string& key) {
					return http_decoder.get_header(key);
				}

				// std::string url() const { return http_decoder->request_url; }
				inline std::string url() const { return uri; }

				inline std::string path() const { 
					std::string_view p = http_decoder.http_path; 
					return std::string(p.data(), p.size()); 
				}

				inline std::string query() const { 
					std::string_view q = http_decoder.http_query; 
					return std::string(q.data(),q.size());
				}
				inline std::string body()  const {
					std::string_view q = http_decoder.http_body; 
					return std::string(q.data(),q.size());
				}
		 

				inline std::string to_string() const {
					return http_encoder.encode();
				}
				void reply(const std::string &msg, uint32_t code = 200){
					if(replier){
						replier(HttpResponse(msg, code )); 
					}
				}

				inline bool is_websocket() const {
					return http_decoder.is_websocket();
				}

				std::function<void(const HttpResponse&)> replier;

				enum  http_method method_value; 
				std::string method;
				std::string uri;
		 
				int http_version_major = 1;
				int http_version_minor = 0;

				HttpDecoder<HttpRequest> http_decoder;
				HttpEncoder<HttpRequest> http_encoder;
		};

		using HttpRequestPtr = std::shared_ptr<HttpRequest>;

	} // namespace http
} // namespace knet
