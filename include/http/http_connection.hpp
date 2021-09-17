#pragma once
#include "knet.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include <cassert>
#include <regex>

using namespace knet::tcp;
namespace knet
{
	namespace http
	{

		using HttpHandler = std::function<HttpResponsePtr(const HttpRequestPtr &)>;
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
			HttpConnection(HttpRequestPtr req  = nullptr) : first_request(req) 
			{				 
			} 

			~HttpConnection() {}

			void init_routers(const HttpHandler & router,   HttpRouteMap * routers)				
			{ 
				global_router = router; 
				http_routers = routers; 
			} 

			inline void request(const HttpRequest &req)
			{
				this->send(req.to_string());
			}

			void reply(const HttpResponse &rsp)
			{
				if (this->is_connected())
				{
					this->send(rsp.to_string());
				}
				if (rsp.code() >= 200)
				{
					this->close();
				}
			}

			void reply(const std::string &msg, uint32_t code = 200)
			{
				HttpResponse rsp(msg, code);
				if (this->is_connected())
				{
					this->send(rsp.to_string());
				}
				if (code >= 200)
				{
					this->close();
				}
			}

			int send_first_request()
			{
				if (first_request)
				{
					return send(first_request->encode());
				}
				return -1;
			}

			virtual bool handle_data(const std::string_view &msg)
			{
				auto req = std::make_shared<HttpRequest>();
				auto msgLen = req->parse(msg.data(), msg.length());
				if (msgLen > 0)
				{
					auto conn = this->shared_from_this();
					req->replier = [conn](const HttpResponse &rsp){ conn->reply(rsp); };

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

					if (global_router)
					{
						auto rsp = global_router(req);
						if (rsp && rsp->code() != 0)
						{
							conn->reply(*rsp);
						}
					}
					else if ( http_routers != nullptr)
					{
						auto itr = http_routers->find(req->path());
						if (itr != http_routers->end())
						{
							if (itr->second)
							{
								auto rsp = itr->second(req);
								if (rsp->code() != 0)
								{								
									conn->reply(*rsp);
								}
							}
							else
							{
								conn->reply(HttpResponse(501));
							}
						}
						else
						{
							conn->reply(HttpResponse(404));
						}
					}else { 
						conn->reply(HttpResponse(404));
					}
				}

				return true;
			}

		private:
			HttpRequestPtr first_request;

			HttpHandler  global_router; 
			HttpRouteMap * http_routers{}; 
		};
		using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

	} // namespace http

} // namespace knet
