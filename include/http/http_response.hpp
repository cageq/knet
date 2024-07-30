#pragma once
#include <memory>
#include <functional>
#include "http_message.hpp"
#include "http_decoder.hpp"

namespace knet
{
	namespace http
	{

		class HttpResponse
		{
		public:
			friend class HttpDecoder<HttpResponse>;
			friend class HttpEncoder<HttpResponse>;
			
			HttpResponse():http_decoder(this), http_encoder(this){
			}


			HttpResponse(const std::string &rsp, uint32_t code = 200,  const std::string &type = "txt"):http_decoder(this), http_encoder(this)
			{ 
				status_code = code;
				http_encoder.set_content(rsp, type);
			}

			HttpResponse(uint32_t c, const std::string &rsp = "", const std::string &type = "txt"):http_decoder(this), http_encoder(this)
			{	 
				status_code = c;
				http_encoder.set_content(rsp, type);
			}

			inline uint32_t parse(const char *data, uint32_t len, bool inplace = false)
			{
 
				length  =  http_decoder.parse_response(data, len, inplace);
				return length; 
			}

			void add_header(const std::string &key, const std::string &value)
			{
				http_encoder.add_header(key, value);
			}

			int32_t  json(const std::string & msg ){ 
				if (writer)
				{
					http_encoder.set_content_type("application/json");  
					return writer(to_string());
				}
				return -1 ; 
			}

			inline uint32_t code() const { return status_code; }

			inline std::string to_string() const { return http_encoder.encode(); }

			inline bool is_websocket() const
			{
				return http_decoder.is_websocket();
			}

			inline std::string body() const { return content; }

			int32_t write(const std::string & msg, uint32_t code = 200){
				if (writer){
					
					status_code = status_code  == 0? code: status_code; 				
					http_encoder.set_content(msg); 
					std::cout << to_string() << std::endl; 
					return writer(to_string()); 
				}
				return -1; 
			}

			int32_t write(){
				if (writer){
					status_code = status_code  == 0? 200:status_code; 
					return writer(to_string()); 
				}
				return -1; 
			}

			std::string uri;
			std::string content_type;
			std::string content;
			uint32_t status_code = 0;
			uint32_t length      = 0; 
			std::function<int32_t (const std::string & msg )> writer; 
		private:			
			HttpDecoder<HttpResponse> http_decoder;
			HttpEncoder<HttpResponse> http_encoder;
		};
		using HttpResponsePtr = std::shared_ptr<HttpResponse>;

	} // namespace http
} // namespace knet
