#pragma once
 
#include "knet.hpp"
#include "http_request.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include "http_url.hpp"
#include "http_connection.hpp"

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

	virtual void handle_event(ConnectionPtr conn, NetEvent evt) {
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

		HttpUrl httpUrl(url);

		std::cout << httpUrl << std::endl;

		auto req = std::make_shared<HttpRequest>(HttpMethod::HTTP_GET, url); 
		
		KNetUrl urlInfo("tcp", httpUrl.host(), httpUrl.port()) ; 
 
		auto conn = connector.add_connection(urlInfo, req );
		

		connections[conn->get_cid()] = conn;
		return true;
	}

private:
	std::unordered_map<uint64_t, std::shared_ptr<HttpConnection>> connections;
	TcpConnector<HttpConnection, HttpClient> connector;
};

} // namespace http
} // namespace knet
