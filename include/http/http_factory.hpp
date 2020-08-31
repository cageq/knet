#pragma once
 
#include "http_connection.hpp"
#include "http_response.hpp"
// #include "http_worker.hpp"

using namespace knet::tcp;

namespace knet {
namespace http {
template <class T = HttpConnection>
class HttpFactory : public ConnectionFactory<T> {

public:
	using TPtr = std::shared_ptr<T>;

	HttpFactory() { dlog("init http factory"); }

	virtual void destroy(TPtr conn) { dlog("connection factory destroy connection in factory "); }

	virtual void handle_event(TPtr conn, NetEvent evt) {

		ilog("handle event in http factory ", evt);

		switch (evt) {
		case EVT_THREAD_INIT:
			dlog("handle thread init event {}", std::this_thread::get_id());
			// worker_.start(routers);
			break;
		case EVT_THREAD_EXIT:
			dlog("handle thread exit event");
			// worker_.stop();
			break;
		case EVT_CREATE:
			dlog("handle new http session, thread id is", std::this_thread::get_id());

			break;
		case EVT_RECV:
			break;
		case EVT_SEND:

			//	conn->close();
			break;
		default:;
		}
	}

	virtual uint32_t handle_data(TPtr conn, const std::string_view & msg, MessageStatus status) {

		auto req = std::make_shared<HttpRequest>();
		auto msgLen = req->parse_request(msg.data(), msg.length());
		if (msgLen > 0) {
			//	req->conn = conn;
			req->replier = [conn](const HttpResponse& rsp) { conn->reply(rsp); };

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

			 auto itr = http_routers.find(req->url());
			 if (itr != http_routers.end()) {
			 	if (itr->second) {
			 		dlog("handle data in thread id {}", std::this_thread::get_id());
			 		auto rsp = itr->second(req);
			 		if (rsp.status_code != 0) {
			 			conn->reply(rsp);
			 		}
			 	} else {
			 		conn->reply(HttpResponse(501));
			 	}
			 } else {
			 	conn->reply(HttpResponse(404));
			 }
		}

		return msgLen;
	}

	// virtual int32_t handle_data(TPtr conn, const char* data, uint32_t len) {
		// dlog("handle data in http factory  {}", data);

		// auto req = std::make_shared<HttpRequest>();

		// dlog("http request url is {}", conn->request_url);

		// auto itr = http_routers.find(conn->request_url);
		// if (itr != http_routers.end()) {
		// 	if (itr->second) {
		// 		dlog("handle data in thread id {}", std::this_thread::get_id());
		// 		auto req = std::make_shared<HttpRequest>();
		// 		req->conn = conn;

		// 		req->data = std::string(data, len);
		// 		auto rsp = itr->second(req);
		// 		if (rsp.code != 0) {
		// 			conn->reply(rsp);
		// 		}
		// 		conn->request_url = "";
		// 	}
		// } else {
		// 	conn->reply(HttpResponse(404));
		// }

	// 	return len;
	// };

	HttpRouteMap http_routers;
};

} // namespace http
} // namespace knet
