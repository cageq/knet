#pragma once
#include "knet.hpp"
// #include "http_request.hpp"
// #include "http_response.hpp"
#include "http_context.hpp"
#include <cassert>
#include <regex>

using namespace knet::tcp;
namespace knet
{
	namespace http
	{

		struct HttpHandler {
			HttpRequestHandler http_handler; 			
			HttpContextHandler context_handler; 

			int32_t call(HttpContextPtr &ctx) {
				if (context_handler){
					return context_handler(ctx); 
				}
				if (http_handler){
					return http_handler(ctx->request, ctx->response); 
				}
				return ctx->response.write(); 
			}
		}; 

		using HttpRouteMap = std::unordered_map<std::string, HttpHandler>; 

		class RegexOrderable : public std::regex
		{
			std::string str;

		public:
			RegexOrderable(const char *regex_cstr) : std::regex(regex_cstr), str(regex_cstr) {}
			RegexOrderable(const std::string &regex_str) : std::regex(regex_str), str(regex_str) {}
			bool operator<(const RegexOrderable &rhs) const noexcept
			{
				return str < rhs.str;
			}
		};

		class HttpConnection : public TcpConnection<HttpConnection>
		{

		public:
			HttpConnection(HttpRequestPtr req  = nullptr) : first_request(req) {
			}

			~HttpConnection() {}

			void init_routers(  HttpHandler & router, HttpRouteMap * routers)				
			{ 
				global_router = router; 
				http_routers  = routers; 
			}


			inline void request(const HttpRequest &req)
			{
				this->send(req.to_string());
			}

			void write(const HttpResponse &rsp)
			{
				if (this->is_connected())
				{
					this->send(rsp.to_string());
				}
				// if (rsp.code() >= 200)
				// {
				// 	this->close();
				// }
				this->close(); 
			}

			void write(const std::string &msg, uint32_t code = 200)
			{				
				if (this->is_connected())
				{ 
					this->send(msg );
				}
				// if (code >= 200)
				// {
				// 	this->close();
				// }
				this->close(); 
			}

			int send_first_request()
			{
				if (first_request)
				{
					return send(first_request->encode());
				}
				return -1;
			}

			virtual bool handle_data(char * data, uint32_t dataLen) override 
			{
				auto ctx = std::make_shared<HttpContext>();  
				auto msgLen = ctx->request.parse(data, dataLen);
				if (msgLen > 0)
				{
					auto conn = this->shared_from_this(); 

					//bool hasHandler = false;
					//for(auto & router : http_routers){
					//	std::smatch path_match;
					//	RegexOrderable ruler(router.first) ;

					//	std::string url = req->url();
					//	std::regex_match(url , path_match, ruler );
					//	if(std::regex_match(url , path_match, ruler ))
					//	{
					//		auto rsp = router.second(req);
					//		if (rsp.status_code != 0) {
					//			conn->reply(rsp);
					//			hasHandler = true;
					//			return msgLen;
					//		}
					//	}
					//}
					//
					//if (!hasHandler)
					//{
					//	conn->reply(HttpResponse(404));
					//} 
	 
					ctx->response.writer = [=](const std::string &data){
						return conn->send(data); 						
					}; 

					// auto rsp = global_router.call(ctx);
					// if (rsp && rsp->code() != 0)
					// {
					// 	conn->write(*rsp);
					// 	return true; 
					// }
				 
					if ( http_routers != nullptr)
					{
						auto itr = http_routers->find(ctx->request.path());
						if (itr != http_routers->end())
						{
							auto &handler = itr->second;  
							auto rst = handler.call(ctx);  

							if (rst < 0){
								conn->write(HttpResponse(501));
							} 
							conn->close(); 
						}
						else
						{
							conn->write(HttpResponse(404));
							conn->close(); 
						}
					}
				}

				return true;
			}

		private:
			HttpRequestPtr first_request; 
			HttpHandler    global_router; 
			HttpRouteMap * http_routers = nullptr; 
		};
		using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

	} // namespace http

} // namespace knet
