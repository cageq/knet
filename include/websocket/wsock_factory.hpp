
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
 
#include "wsock_connection.hpp"
#include "http/http_response.hpp"
#include "http/http_url.hpp"

using namespace knet::tcp;
using namespace knet::http;

namespace knet {
namespace websocket {

template <class T = WSockConnection>
class WSockFactory : public TcpFactory<T>  , public UserEventHandler<T> {

public:
	using TPtr = std::shared_ptr<T>;

	WSockFactory() { dlog("init http factory"); }
	virtual bool handle_event(TPtr conn, NetEvent evt) {

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
		return true; 
	}

	virtual bool handle_data(TPtr conn, const std::string& msg ) {
		dlog("websocket handle data len is {}  websocket : {}", msg.length(), conn->is_websocket);

		if (conn->is_websocket) {

			auto readLen = conn->read_websocket(msg);
			dlog("read websocket data len is {}/{}", readLen, msg.length());
			return readLen;

		} else {

			dlog("websocket data is {} len is {}", msg, msg.length());
			if (conn->is_status(WSockStatus::WSOCK_INIT)) {
				auto req = std::make_shared<HttpRequest>();
				auto msgLen = req->parse_request(msg.data(), msg.length(), true);
				if (msgLen > 0) {

					HttpUrl urlInfo(req->url());
					dlog("handle websocket request ", req->url());
					ilog("request Upgrade header {}", req->get_header("Upgrade"));
					ilog("request Connection header {}", req->get_header("Connection"));
					if (req->get_header("Upgrade") == "websocket") {

						dlog("find request path is {}", urlInfo.path());
						auto itr = wsock_routers.find(urlInfo.path());

						if (itr != wsock_routers.end()) {
							dlog("found websocket handle , send back websocket response");
							auto ret = conn->upgrade_websocket(req);
							if (ret) {
								conn->is_websocket = true;
								conn->wsock_handler = itr->second;
							}
						} else {

							wlog("not found websocket handler :{}", urlInfo.path());
							conn->reply(HttpResponse(404));
						}

					} else {

						dlog("request path is {}", urlInfo.path());
						req->replier = [conn](const HttpResponse& rsp) { conn->reply(rsp); };

						auto itr = http_routers.find(urlInfo.path());
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
				} else {
					return 0;
				}
				return msgLen;
			} else if (conn->is_status(WSockStatus::WSOCK_CONNECTING)) {
				HttpResponse rsp;
				auto msgLen = rsp.parse_response(msg.data(), msg.length());
				if (msgLen > 0) {
					dlog("parse response len is {},  {}", msg.length(), msgLen);
					if (rsp.is_websocket()) {
						dlog("upgrade to websocket success");
						conn->is_websocket = true;
					}
				} else {
					return 0;
				}
			}
		}

		return true;
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

	//	return len;
	//};

	HttpRouteMap http_routers;
	std::unordered_map<std::string, WSockHandler<T>> wsock_routers;
};

} // namespace websocket
} // namespace knet
