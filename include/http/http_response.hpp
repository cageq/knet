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
			HttpResponse() = default; 
			HttpResponse(const std::string &rsp, uint32_t code = 200,  const std::string &type = "txt")
			{
				http_encoder.init_http_message(this);
				status_code = code;
				http_encoder.set_content(rsp, type);
			}

			HttpResponse(uint32_t c, const std::string &rsp = "", const std::string &type = "txt")
			{
				http_encoder.init_http_message(this);
				status_code = c;
				http_encoder.set_content(rsp, type);
			}

			inline uint32_t parse(const char *data, uint32_t len, bool inplace = false)
			{
				http_decoder.init_http_message(this);
				length  =  http_decoder.parse_response(data, len, inplace);
				return length; 
			}

			void add_header(const std::string &key, const std::string &value)
			{
				http_encoder.add_header(key, value);
			}

			std::string json(){

				return ""; 
			}

			inline uint32_t code() const { return status_code; }

			inline std::string to_string() const { return http_encoder.encode(); }

			inline bool is_websocket() const
			{
				return http_decoder.is_websocket();
			}

			inline std::string body() const { return http_encoder.content; }

			std::string uri;
			std::string content;
			uint32_t status_code = 0;
			uint32_t length      = 0; 
			std::function<int32_t (const char * , uint32_t)> writer; 

		private:			
			HttpDecoder<HttpResponse> http_decoder;
			HttpEncoder<HttpResponse> http_encoder;
		};
		using HttpResponsePtr = std::shared_ptr<HttpResponse>;

	} // namespace http
} // namespace knet
