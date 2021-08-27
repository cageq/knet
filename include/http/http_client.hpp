#pragma once
 
#include "knet.hpp"
#include "http_request.hpp"
#include "http_factory.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include "http_url.hpp"

using namespace knet::tcp;
namespace knet {
namespace http {

class HttpClient : public KNetFactory<HttpConnection> {

public:
	using HttpConnector = TcpConnector<HttpConnection, HttpClient>;

	HttpClient()
		: connector(this) {
		connector.start();
	}

	virtual void handle_event(TPtr conn, NetEvent evt) {
		switch (evt) {
		case NetEvent::EVT_CONNECT:
			conn->send_first_request() ;
			break;
		case NetEvent::EVT_DISCONNECT:
			connections.erase(conn->get_cid());
			break;

		default:;
		}
	}

	bool get(const std::string& url) {

		HttpUrl urlInfo(url);

		std::cout << urlInfo << std::endl;

		auto req = std::make_shared<HttpRequest>(HttpMethod::HTTP_GET, url);
		auto conn = connector.add_connection(ConnectionInfo(urlInfo.host(), urlInfo.port()), req );

		connections[conn->get_cid()] = conn;
		return true;
	}

private:
	std::unordered_map<uint64_t, std::shared_ptr<HttpConnection>> connections;
	TcpConnector<HttpConnection, HttpClient> connector;
};

} // namespace http
} // namespace knet
