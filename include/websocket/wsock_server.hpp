//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************


#pragma once
#include "knet.hpp"
#include "wsock_factory.hpp"
#include "wsock_connection.hpp"

using namespace knet::http;
namespace knet {
namespace websocket {

template <class Worker = knet::EventWorker, class Factory = WSockFactory <WSockConnection> >
class WSockServer {
public:
	using WorkerPtr = std::shared_ptr<Worker>;
 
	WSockServer(Factory* fac = nullptr, WorkerPtr lisWorker = std::make_shared<Worker>())
		: wsock_factory(fac == nullptr ? &default_factory : fac)
		, wsock_listener(wsock_factory, lisWorker) {
		lisWorker->start(); 
		wsock_listener.add_worker(lisWorker); // also as default worker
	}

	bool start( uint32_t port = 8888,const std::string& host ="127.0.0.1") { return wsock_listener.start(port, host); }

	void register_router(const std::string& path, HttpHandler handler) {
		dlog("register path {}", path);
		if (wsock_factory) {
			wsock_factory->http_routers[path] = handler;
		}  
	}

	void register_router(const std::string& path, WSockHandler<WSockConnection> handler) {
		dlog("register path {}", path);
		if (wsock_factory) {
			wsock_factory->wsock_routers[path] = handler;
		}  
	}
private:
	Factory* wsock_factory = nullptr;
	Factory default_factory;
	TcpListener<WSockConnection, Factory> wsock_listener;
};

} // namespace websocket
} // namespace knet
