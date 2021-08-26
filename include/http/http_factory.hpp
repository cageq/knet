#pragma once
 
#include "http_connection.hpp"
#include "http_response.hpp"

using namespace knet::tcp;

namespace knet {
namespace http {
template <class T = HttpConnection>
class HttpFactory : public KNetFactory<T> , public KNetHandler<T> {

public:
	using TPtr = std::shared_ptr<T>;
	HttpFactory() {  }

	virtual void destroy(TPtr conn) { dlog("destroy connection in factory "); }

	virtual bool handle_event(TPtr conn, NetEvent evt) {

	//	dlog("handle event in http factory ", evt);
		switch (evt) {
		case EVT_THREAD_INIT:
			//dlog("handle thread init event {}", std::this_thread::get_id());
			// worker_.start(routers);
			break;
		case EVT_THREAD_EXIT:
			dlog("handle thread exit event");
			// worker_.stop();
			break;
		case EVT_CREATE:
			//dlog("handle new http session, thread id is", std::this_thread::get_id());

			break;
		case EVT_RECV:
			break;
		case EVT_SEND:
			break;
		default:;
		}

		return true; 
	}

	inline void set_global_routers(const HttpHandler & router){
		user_routers = router; 
	}

	virtual bool handle_data(TPtr conn, const std::string & msg ) {

		auto req = std::make_shared<HttpRequest>();
		auto msgLen = req->parse(msg.data(), msg.length());
		if (msgLen > 0) {
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

			if (user_routers) {
				auto rsp = user_routers(req); 
				if (rsp && rsp->status_code != 0){
					conn->reply(*rsp); 
				}
			} else {
				auto itr = http_routers.find(req->path());
				if (itr != http_routers.end()) {
					if (itr->second) {
						auto rsp = itr->second(req);
						if (rsp->status_code != 0) {
							conn->reply(*rsp);
						}
					} else {
						conn->reply(HttpResponse(501));
					}
				} else {
					conn->reply(HttpResponse(404));
				}

			}
		}

		return true;
	}

	HttpHandler  user_routers; 

	HttpRouteMap http_routers;
};

} // namespace http
} // namespace knet
