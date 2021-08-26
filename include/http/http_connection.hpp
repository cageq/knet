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

		using HttpHandler = std::function<HttpResponsePtr (const HttpRequestPtr & )>;
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
			HttpConnection() { }
			HttpConnection(HttpRequestPtr req):first_request(req) {
			}

			~HttpConnection() { dlog("destroy http connection"); }

			void request(const HttpRequest &req)
			{
				this->send(req.to_string());
			}

			void reply(const HttpResponse &rsp)
			{
				this->send(rsp.to_string());
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
				if (code >= 200 )
				{
					this->close();
				}
			}

			int send_first_request(){
				if (first_request){
					return send(first_request->encode());
				}
				return -1; 
			}

		private:
			HttpRequestPtr first_request;

		};
		using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

	} // namespace http

} // namespace knet
