//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************


#pragma once
#include "knet.hpp"
#include "tls_context.hpp"
#include "wsock_factory.hpp"
#include "wsock_ssl_connection.hpp"

using namespace knet::http;
namespace knet {
namespace websocket {

template <class Worker = knet::KNetWorker, class Factory = WSockFactory <WSockSslConnection> >
class WSockSslServer {
public:
	using WorkerPtr = std::shared_ptr<Worker>;
 
	WSockSslServer(Factory* fac = nullptr, WorkerPtr lisWorker = std::make_shared<Worker>())
		: wsock_factory(fac == nullptr ? &default_factory : fac)
		, wsock_listener(wsock_factory, lisWorker) {
		lisWorker->start(); 
		wsock_listener.add_worker(lisWorker); // also as default worker
	}

	inline asio::ssl::context* init_ssl(const std::string& chainFile, const std::string& dhFile,
		const std::string& password = "test") {

		ssl_password = password;
		ssl_context = new asio::ssl::context(asio::ssl::context::sslv23);
		ssl_context->set_options(asio::ssl::context::default_workarounds |
								 asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
		ssl_context->set_password_callback(std::bind(&WSockSslServer::get_password, this));
		ssl_context->use_certificate_chain_file(chainFile);
		ssl_context->use_private_key_file(chainFile, asio::ssl::context::pem);
		ssl_context->use_tmp_dh_file(dhFile);

		// ssl_context->load_verify_file(caFile);
		return ssl_context;
	}
	std::string get_password() const { return ssl_password; }
	bool start( uint16_t port = 8888,const std::string& host ="127.0.0.1") {
	
		if (ssl_context == nullptr) {
			knet_wlog("please init ssl context first");
		}
		return wsock_listener.start({"tcp", host, port},{},  ssl_context); 
	 }

	void register_router(const std::string& path, HttpHandler handler) {
		knet_dlog("register path {}", path);
		if (wsock_factory) {
			wsock_factory->http_routers[path] = handler;
		}  
	}

	void register_router(const std::string& path, WSockHandler<WSockSslConnection> handler) {
		knet_dlog("register path {}", path);
		if (wsock_factory) {
			wsock_factory->wsock_routers[path] = handler;
		}  
	}
private:
	asio::ssl::context* ssl_context = nullptr;
	Factory* wsock_factory = nullptr;
	Factory default_factory;
	std::string ssl_password = "";
	TcpListener<WSockSslConnection, Factory> wsock_listener;
};

} // namespace websocket
} // namespace knet
