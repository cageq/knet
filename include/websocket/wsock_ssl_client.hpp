#pragma once

#include <iostream>
#include "knet.hpp"
#include "http/http_url.hpp"
#include "wsock_factory.hpp"
#include "websocket/wsock_ssl_connection.hpp"

using namespace knet::tcp;
using namespace knet::http;

namespace knet {

namespace websocket {

class WSockSslClient {

public:
	WSockSslClient()
		: connector(&default_factory) {}
	void start() { connector.start(); }

	std::shared_ptr<WSockSslConnection> add_connection(
		const std::string& url,WSockHandler<WSockSslConnection>   handler = {} ,  const std::string& caFile = "cert/ca.pem") {

		HttpUrl urlInfo(url);
		std::cout << urlInfo << std::endl;

		auto firstReq = create_request(urlInfo); 
		 
		return connector.add_ssl_connection(KNetUrl("tcp",urlInfo.host(), urlInfo.port()), caFile, firstReq, handler);
	}

	HttpRequestPtr create_request(const HttpUrl& urlInfo) {
		auto firstReq = std::make_shared<HttpRequest>(HttpMethod::HTTP_GET, urlInfo.path());

		firstReq->add_header("Host", urlInfo.host());
		firstReq->add_header("Connection", "Upgrade");
		firstReq->add_header("Upgrade", "websocket");
		firstReq->add_header("Sec-WebSocket-Version", "13");

		auto cliKey = WSockHandshake::random_string(16);
		dlog("key is {}", cliKey);
		auto secKey = WSockHandshake::base64(cliKey);
		dlog("sec key is {}", secKey);

		firstReq->add_header("Sec-WebSocket-Key", secKey);

		return firstReq;
	}

public:
	WSockFactory<WSockSslConnection> default_factory;
	TcpConnector<WSockSslConnection, WSockFactory<WSockSslConnection>> connector;
};

} // namespace websocket
} // namespace gnet