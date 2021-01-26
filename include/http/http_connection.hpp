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

		using HttpHandler = std::function<HttpResponse(HttpRequestPtr)>;
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
			HttpConnection()
			{

				thrdid = std::this_thread::get_id();

				// http_parser_settings_init(&settings_);
				// settings_.on_url = &HttpConnection::handle_url_callback;

				// http_parser_init(&parser_, HTTP_REQUEST);
				// parser_.data = this;

				//	bind_package_handler(&HttpConnection::handle_package);
			}

			~HttpConnection() { wlog("destroy session"); }

			// int32_t handle_package(char* data, uint32_t len) {

			// 	auto req = std::make_shared<HttpRequest>();
			// 	uint32_t parsedLen = req->parse_request(data,len);
			// 	if (parsedLen > 0 )
			// 	{
			// 		http_request = req;
			// 	}
			// 	return parsedLen;
			// }

			// static int handle_url_callback(http_parser* parser, const char* pos, size_t length) {

			// 	HttpConnection* self = static_cast<HttpConnection*>(parser->data);
			// 	dlog("handle url callback {} ", std::string(pos, length));
			// 	if (self) {
			// 		self->request_url = std::string(pos, length);
			// 	}

			// 	return 0;
			// }

			// void handle_path(const std::string& path) {

			// 	request_url = path;
			// 	dlog("request path is {}", path);
			// }

			void request(const HttpRequest &req)
			{
				this->send(req.to_string());
			}

			void reply(const HttpResponse &rsp)
			{

				this->send(rsp.to_string());

				if (rsp.status_code != 100)
				{
					this->close();
				}
			}

			void reply(const std::string &msg, uint32_t code = 200, bool fin = true)
			{
				HttpResponse rsp(msg, code);

				if (this->is_connected())
				{
					this->send(rsp.to_string());
				}
				if (fin)
				{
					this->close();
				}
			}

			void release() { this->close(); }

			HttpRequestPtr first_request;

		private:
			std::thread::id thrdid;

			// http_parser parser_ = {0};
			// http_parser_settings settings_;
		};
		using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

	} // namespace http

} // namespace knet
