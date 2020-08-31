
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************
#pragma once

#include "http/http_parser.hpp"
#include "http/http_request.hpp"
#include "http/http_connection.hpp"
#include "wsock_shakehand.hpp"
#include <string_view>

#include "wsock_protocol.hpp"

using namespace knet::tcp;
using namespace knet::http;
 

namespace knet {
namespace websocket {
 


 
class WSockConnection : public TcpConnection<WSockConnection> 
{
public:
	using WsConnectionPtr = std::shared_ptr<WSockConnection>;
	WSockConnection(HttpRequestPtr firstReq = nullptr, WSockHandler<WSockConnection>   handler = {})
		: first_request(firstReq)
		, wsock_reader(4096), 
		wsock_handler(handler) {

		// http_parser_settings_init(&m_parse_settings);

		// http_parser_init(&m_parser, HTTP_REQUEST);

		bind_event_handler([this](WsConnectionPtr conn, NetEvent evt) {
			switch (evt) {
			case NetEvent::EVT_CONNECT:
				dlog("on connected event ");
				if (first_request) {
					auto msg = first_request->encode();
					dlog("send first  request  {} :\n{}", msg.length(), msg);
					this->msend(first_request->encode());
					m_status = WSockStatus::WSOCK_CONNECTING;
				} else {
				}

				break;
			default:;
			}

			return 0;
		});
		// bind_data_handler(&WSockConnection::handle_data);

		// bind_message_handler([this](const char* pMsg, uint32_t len, MessageStatus status) {
		// 	// dlog("handle message {}", std::string(pMsg, len));
		// 	dlog("handle message {}", pMsg);
		// 	dlog("message length {}", len);
		// 	dlog("message status {}", status);

		// 	//this->send_text(std::string(pMsg, len));
		// });
	}

	// void bind_message_handler(WSMessageHandler handler) { message_handler = handler; }
 

	// connection events
	void on_connect() {
		dlog("on connected ");
		 
	}

	// read on buffer for one package
	// return one package length ,if not enough return -1
	// int read_packet(const char* pData, uint32_t dataLen) {
	// 	if (m_status == WSockStatus::WSOCK_CONNECTING) {
	// 		size_t parseRst = http_parser_execute(&m_parser, &m_parse_settings, pData, dataLen);
	// 		dlog("received message \n{}", pData);
	// 		dlog("received data length is %ld parse result is %ld", dataLen, parseRst);
	// 		if (m_parser.upgrade) {
	// 			m_status = WSockStatus::WSOCK_OPEN;
	// 			dlog("success upgrade to websocket ");
	// 			return parseRst;
	// 		}
	// 	} else {
	// 	}

	// 	dlog("on read package ");
	// 	return dataLen;
	// }

	int send_text(const std::string& msg, OpCode op = TEXT_FRAME, bool mask = true) {
		dlog("send text length is {}", msg.length());
		std::vector<uint8_t> header;
		//uint8_t maskKey[4] = {0x12, 0x34, 0x56, 0x78}; 
		uint8_t maskKey[4] ; 
		for (auto& m : maskKey)
		{
			m = static_cast<uint8_t>(random()); 
		}


		header.assign(sizeof(uint16_t) + (msg.length() >= 126 ? 2 : 0) +
						  (msg.length() >= 65536 ? 6 : 0) + (mask ? 4 : 0),	0);
		uint8_t payloadByte = mask ?0x80:0; 

		header[0] = 0x80 | op;

		if (msg.length() < 126) {
			header[1] = (msg.length() & 0xff) | payloadByte;

			if (mask)
			{
				header[2] = maskKey[0];
				header[3] = maskKey[1];
				header[4] = maskKey[2];
				header[5] = maskKey[3];
			}
			
		} else if (msg.length() < 65535) {
			header[1] = 126 | payloadByte;
			header[2] = (msg.length() >> 8) & 0xff;
			header[3] = (msg.length() >> 0) & 0xff;

			if (mask)
			{
				header[4] = maskKey[0];
				header[5] = maskKey[1];
				header[6] = maskKey[2];
				header[7] = maskKey[3];
			}
			
		} else {

			header[1] = 127 | payloadByte;
			header[2] = (msg.length() >> 56) & 0xff;
			header[3] = (msg.length() >> 48) & 0xff;
			header[4] = (msg.length() >> 40) & 0xff;
			header[5] = (msg.length() >> 32) & 0xff;
			header[6] = (msg.length() >> 24) & 0xff;
			header[7] = (msg.length() >> 16) & 0xff;
			header[8] = (msg.length() >> 8) & 0xff;
			header[9] = (msg.length() >> 0) & 0xff;

			if (mask)
			{
				header[10] = maskKey[0];
				header[11] = maskKey[1];
				header[12] = maskKey[2];
				header[13] = maskKey[3];
			}
			
		}

		std::string sendMsg = msg;

		if (mask)
		{				
			for (uint32_t i = 0; i < sendMsg.length(); i++) {
				int j = i % 4;
				sendMsg[i] ^= maskKey[j];
			}
		}


		parse_message((const char*)header.data(), header.size());
		dlog("send head length {}, message length {} ", header.size(),sendMsg.length()); 

		return this->msend(std::string((char*)header.data(), header.size()), sendMsg);
	}

	void parse_message(const char* pData, uint32_t len) {
		uint8_t* vals = (uint8_t*)pData;
		std::cout << "fin is " << ((vals[0] & 0x80) == 0x80) << std::endl;
		std::cout << "opcode is " << (vals[0] & 0x0f) << std::endl;
		std::cout << "length is " << (vals[1] & 0x7F) << std::endl;
		std::cout << "mask is " << ((vals[1] & 0x80) == 0x80) << std::endl;

		// uint64_t payloadLen = (vals[1] & 0x7f);
		// uint32_t headSize = sizeof(uint16_t);
	}

	// will invoke in multi-thread , if you want to process it main thread , push it to msg queue
	// int32_t handle_raw_data(char* pBuf, uint32_t len) {
	// 	// dlog(" connection id %d on thread %d", m_id, std::this_thread::get_id());

	// 	if (m_status == WSockStatus::WSOCK_CONNECTING) {
	// 		size_t parseRst = http_parser_execute(&m_parser, &m_parse_settings, pBuf, len);

	// 		dlog("received message \n{}", pBuf);
	// 		dlog("received data length is {} parse result is {}", len, parseRst);
	// 		if (m_parser.upgrade) {
	// 			m_status = WSockStatus::WSOCK_OPEN;
	// 			dlog("success upgrade to websocket ");
	// 			is_websocket = true;
	// 			// this->send_text("hello world");
	// 			return parseRst;
	// 		}
	// 	} else {
	// 		dlog("received buffer length {}", len);
	// 		uint32_t ret = wsock_reader.read(pBuf, len, message_handler);

	// 		dlog("have read buffer data {}", ret);
	// 		return ret;
	// 	}

	// 	return len;
	// }

	uint32_t read_websocket( const std::string & msg ) {

		return wsock_reader.read(msg.data(),msg.length(), std::bind(&WSockConnection::read_message, this, std::placeholders::_1, std::placeholders::_2));
	}

	bool upgrade_websocket(HttpRequestPtr req) {

		std::string_view secWebSocketKey = req->get_header("Sec-WebSocket-Key");

		if (secWebSocketKey.length() == 24) {

			char secWebSocketAccept[29] = {};
			WSockHandshake::generate(secWebSocketKey.data(), secWebSocketAccept);

			HttpResponse rsp("", 101);

			rsp.add_header("Upgrade", "websocket");
			rsp.add_header("Connection", "Upgrade");
			rsp.add_header("Sec-WebSocket-Accept", secWebSocketAccept);
			rsp.add_header("Accept-Encoding","gzip, deflate"); 
			reply(rsp);
			dlog("send websocket response success");

			is_websocket = true;
			return true;
		}else {
			elog("bad websocket key {}", secWebSocketKey);
			reply("bad websocket key",400); 
		}
		

		return false;
	}

	void reply(const HttpResponse& rsp) {
		dlog("send reply :{}", rsp.to_string());
		this->send(rsp.to_string());

		if (rsp.status_code >= 200) {
			this->close();
		}
	}

	WSockStatus get_status() { return m_status; }
	void set_status(WSockStatus status) { m_status = status; }
	bool is_status(WSockStatus status) { return m_status == status; }

	void reply(const std::string& msg, uint32_t code = 200, bool fin = true) {
		HttpResponse rsp(msg, code);

		if (this->is_connected()) {
			this->send(rsp.to_string());
		}
		if (fin) {
			this->close();
		}
	}


	void read_message(const std::string_view& msg , MessageStatus status)
	{

		if (wsock_handler.message)
		{
			wsock_handler.message(this->shared_from_this() , std::string(msg.data(), msg.size()),status ); 
		}
	}
	
	bool is_websocket = false;

	HttpRequestPtr first_request;
	WSMessageReader wsock_reader;
	WSockHandler<WSockConnection>  wsock_handler; 

private:
	MessageStatus msg_status;
	WSockStatus m_status = WSockStatus::WSOCK_INIT;

//	WSMessageHandler message_handler;

	
};

} // namespace websocket
} // namespace knet
