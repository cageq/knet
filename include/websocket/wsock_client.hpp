//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include "websocket/wsock_connection.hpp"
#include <iostream>
#include "knet.hpp"
#include "http/http_url.hpp"
#include "wsock_factory.hpp"

using namespace knet::tcp;
using namespace knet::http;

namespace knet {

namespace websocket {

class WSockClient {

public:
	WSockClient()
		: connector(&default_factory) {}
	void start() { connector.start(); }

	std::shared_ptr<WSockConnection> add_connection(const std::string& url, WSockHandler<WSockConnection>   handler = {} ) {

		HttpUrl urlInfo(url);
		std::cout << urlInfo << std::endl;
		

		auto firstReq = create_request(urlInfo);
		return connector.add_connection(KNetUrl("tcp",urlInfo.host(), urlInfo.port()), firstReq, handler );
	}

	HttpRequestPtr create_request(const HttpUrl& urlInfo) {
		auto firstReq = std::make_shared<HttpRequest>(HttpMethod::HTTP_GET, urlInfo.path());

		firstReq->add_header("Host", urlInfo.host());
		firstReq->add_header("Connection", "Upgrade");
		firstReq->add_header("Upgrade", "websocket");
		firstReq->add_header("Sec-WebSocket-Version", "13");
		firstReq->add_header("Accept-Language","en-us"); 
		firstReq->add_header("Accept-Encoding", "gzip,deflate");
		 

		auto cliKey = WSockHandshake::random_string(16);
		dlog("key is {}", cliKey);
		auto secKey = WSockHandshake::base64(cliKey);
		dlog("sec key is {}", secKey);

		firstReq->add_header("Sec-WebSocket-Key", secKey);
		return firstReq;
	}

public:
	WSockFactory<WSockConnection> default_factory;
	TcpConnector<WSockConnection, WSockFactory<WSockConnection>> connector;
};

} // namespace websocket
} // namespace knet
