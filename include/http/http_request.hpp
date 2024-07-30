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
				HttpRequest():http_decoder(this),http_encoder(this){

				} 

				//encode methods 
				HttpRequest(HttpMethod method, const std::string& url, const std::string& content = "",
						const std::string& type = "txt") :http_decoder(this),http_encoder(this) {					
			 		http_method = method; 
					http_url = url;		
					http_encoder.set_content(content, type); 
				}

				HttpRequest(const std::string& url, const std::string& query = ""):http_decoder(this),http_encoder(this) {
					http_url = url;
				}

				HttpRequest(const char* data, uint32_t len, bool inplace = false):http_decoder(this),http_encoder(this){
					 content = std::string(data, len); 
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
		 
					return http_decoder.parse_request(data, len, inplace);
				}

				std::string_view get_header(const std::string& key)  const {
					return http_decoder.get_header(key);
				}
 
				inline std::string url() const { return http_url; }

				inline std::string path() const { 
					
					return std::string(http_decoder.http_path); 
				}

				inline std::string query() const { 					
					return std::string(http_decoder.http_query);
				}
				inline std::string body()  const {					
					return std::string(http_decoder.http_body);
				}
		 
				inline bool is_websocket() const {
					return http_decoder.is_websocket();
				}
 
 				inline std::string to_string() const {
					return std::string(http_decoder.http_body);
				} 

				std::string content_type;
				std::string content;

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
