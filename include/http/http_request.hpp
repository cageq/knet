#pragma once

#include "http_decoder.hpp"
#include "http_encoder.hpp"
#include "http_response.hpp"

#include <string>
#include <memory>
#include <functional>

namespace knet {
	namespace http {

		class HttpRequest {
			public:

				
				HttpRequest()  = default; 

				//encode methods 
				HttpRequest(HttpMethod method, const std::string& url, const std::string& content = "",
						const std::string& type = "txt") {
					http_encoder.init_http_message(this); 
			 		http_method = method; 
					http_url = url;
					http_encoder.set_content(content, type); 
				}

				HttpRequest(const std::string& url, const std::string& query = "") {
					http_encoder.init_http_message(this); 
					http_url = url;
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

	
				inline std::string dump() const {
					return http_encoder.encode();
				} 

 
				//decode methods 
				uint32_t parse(const char* data, uint32_t len, bool inplace = false) {
					http_decoder.init_http_message(this); 
					return http_decoder.parse_request(data, len, inplace);
				}

				std::string_view get_header(const std::string& key)  const {
					return http_decoder.get_header(key);
				}
 
				inline std::string url() const { return http_url; }

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
		 
				inline bool is_websocket() const {
					return http_decoder.is_websocket();
				}
 
 				inline std::string to_string() const {
					return std::string(http_decoder.http_body);
				} 

	
				HttpMethod http_method;
				enum  http_method method_value; 
				std::string method; 
				std::string http_url; 
				int http_version_major = 1;
				int http_version_minor = 0; 
				HttpDecoder<HttpRequest> http_decoder;
				HttpEncoder<HttpRequest> http_encoder;
		};

		using HttpRequestPtr = std::shared_ptr<HttpRequest>;

	} // namespace http
} // namespace knet
