
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
 
#include "wsock_connection.hpp"
#include "http/http_response.hpp"
#include "http/http_url.hpp"
#include "wsock_router.hpp"

using namespace knet::tcp;
using namespace knet::http;

namespace knet {
namespace websocket {

template <class T = WSockConnection>
class WSockFactory : public KNetFactory<T>  , public KNetHandler<T> {

public:
	using TPtr = std::shared_ptr<T>;

	WSockFactory() {  }
	virtual bool handle_event(TPtr conn, NetEvent evt) {

		knet_ilog("handle event {} in websocket factory ", static_cast<uint32_t>(evt));

		switch (evt) {
		case EVT_THREAD_INIT:
			//knet_dlog("handle thread init event {}", std::this_thread::get_id());
			// worker_.start(routers);
			break;
		case EVT_THREAD_EXIT:
			knet_dlog("handle thread exit event");
			// worker_.stop();
			break;
		case EVT_CREATE:
			//knet_dlog("handle new http session, thread id is", std::this_thread::get_id());
			break;
		case EVT_RECV:
			break;
		case EVT_SEND:
			//	conn->close();
			break;
        case EVT_DISCONNECT:

            break; 
		default:;
		}
		return true; 
	}

	virtual bool handle_data(TPtr conn, char * data, uint32_t dataLen)  override {  

			knet_dlog("handle factory data , is websocket {}", conn->is_websocket); 
			if (conn->is_websocket) {
				auto readLen = conn->read_websocket(data, dataLen );
				knet_dlog("read websocket data len is {}/{}", readLen, dataLen);
				return true;
			}  	
			
			knet_dlog("websocket status is {}", static_cast<uint32_t>(conn->get_status()) ); 

	
			if (conn->is_status(WSockStatus::WSOCK_INIT)) {

				auto ctx = std::make_shared<HttpContext>(); 
				auto & req = ctx->request; 
				auto msgLen = req.parse(data, dataLen, true);
				if (msgLen > 0) {

					HttpUrl urlInfo(req.url());
					knet_dlog("handle websocket request ", req.url());
					knet_ilog("request Upgrade header {}", req.get_header("Upgrade"));
					knet_ilog("request Connection header {}", req.get_header("Connection"));
					if (req.get_header("Upgrade") == "websocket") {
						
						knet_dlog("find request path is {}", urlInfo.path());
						auto itr = wsock_routers.find(urlInfo.path());
						if (itr != wsock_routers.end()) {
							knet_dlog("found websocket handle , send back websocket response");
							auto ret = conn->upgrade_websocket(req);
							if (ret) {
								conn->is_websocket = true;
								conn->wsock_handler = itr->second;
							}
						} else {
							knet_wlog("not found websocket handler :{}", urlInfo.path());
							conn->write(HttpResponse(404));
						}

					} else {

						knet_dlog("request path is {}", urlInfo.path()); 

						auto itr = http_routers.find(urlInfo.path());
						if (itr != http_routers.end()) {
							auto & handler = itr->second; 
							 
								//knet_dlog("handle data in thread id {}", std::this_thread::get_id());
								auto ret = handler.call( ctx);
								if (ret < 0 ) {
									return false; 
								}
							
						} else {
							conn->write(HttpResponse(404));
						}
					}
				} else {
					return true;
				}
				return true;
			} else if (conn->is_status(WSockStatus::WSOCK_CONNECTING)) {
				HttpResponse rsp;
				auto msgLen = rsp.parse(data, dataLen);
				if (msgLen > 0) {
					knet_dlog("parse response len is {},  {}", dataLen, msgLen);
					if (rsp.is_websocket()) {
						knet_dlog("upgrade to websocket success");
						conn->is_websocket = true;
					}
				} else {
					return true;
				}
			}

		return true;
	}

	// virtual int32_t handle_data(TPtr conn, const char* data, uint32_t len) {
	// knet_dlog("handle data in http factory  {}", data);

	// auto req = std::make_shared<HttpRequest>();

	// knet_dlog("http request url is {}", conn->request_url);

	// auto itr = http_routers.find(conn->request_url);
	// if (itr != http_routers.end()) {
	// 	if (itr->second) {
	// 		knet_dlog("handle data in thread id {}", std::this_thread::get_id());
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
	// WSockRouter  wsock_router; 

};

} // namespace websocket
} // namespace knet
