
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************
#pragma once
 #include <fmt/format.h>

#include "http/http_url.hpp" 
#include "http/http_parser.hpp"
#include "http/http_request.hpp"
#include "http/http_connection.hpp"
#include "wsock_shakehand.hpp" 
#include "wsock_protocol.hpp"
#include "tcp/asio/tls_socket.hpp"


using namespace knet::tcp;
using namespace knet::http;
 

namespace knet {
namespace websocket {
  
 
class WSockSslConnection : public TcpConnection<WSockSslConnection, TlsSocket<WSockSslConnection>> 
{
public:
	using WsConnectionPtr = std::shared_ptr<WSockSslConnection>;
	WSockSslConnection(HttpRequestPtr firstReq = nullptr, WSockHandler<WSockSslConnection>   handler = {})
		: first_request(firstReq)
		, wsock_reader(4096), 
		wsock_handler(handler) {

		// http_parser_settings_init(&m_parse_settings);

		// http_parser_init(&m_parser, HTTP_REQUEST);

		// bind_event_handler([this]( NetEvent evt) {
		// 	switch (evt) {
		// 	case NetEvent::EVT_CONNECT:
			
		// 		if (first_request) {
		// 			auto msg = first_request->encode();
		// 			knet_dlog("send first request  {} :\n{}", msg.length(), msg);
		// 			this->msend(first_request->encode());
		// 			m_status = WSockStatus::WSOCK_CONNECTING;
		// 		} 
		// 		break;
		// 	default:;
		// 	}

		// 	return true;
		// });
		// bind_data_handler(&WSockSslConnection::handle_data);

		// bind_message_handler([this](const char* pMsg, uint32_t len ) {
		// 	// knet_dlog("handle message {}", std::string(pMsg, len));
		// 	knet_dlog("handle message {}", pMsg);
		// 	knet_dlog("message length {}", len);
		// 	knet_dlog("message status {}", status);

		// 	//this->send_text(std::string(pMsg, len));
		// });
	}

	// void bind_message_handler(WSMessageHandler handler) { message_handler = handler; }
	
	virtual bool handle_event(knet::NetEvent evt) 
	{ 
		switch (evt) {
		case NetEvent::EVT_CONNECT:
		
			if (first_request) {
				auto msg = first_request->encode();
				knet_dlog("send first request  {} :\n{}", msg.length(), msg);
				m_status = WSockStatus::WSOCK_CONNECTING;
				this->msend(msg);				
			} 
			break;
		default:;
		}

		return true;
	}

	

	// read on buffer for one package
	// return one package length ,if not enough return -1
	// int read_packet(const char* pData, uint32_t dataLen) {
	// 	if (m_status == WSockStatus::WSOCK_CONNECTING) {
	// 		size_t parseRst = http_parser_execute(&m_parser, &m_parse_settings, pData, dataLen);
	// 		knet_dlog("received message \n{}", pData);
	// 		knet_dlog("received data length is %ld parse result is %ld", dataLen, parseRst);
	// 		if (m_parser.upgrade) {
	// 			m_status = WSockStatus::WSOCK_OPEN;
	// 			knet_dlog("success upgrade to websocket ");
	// 			return parseRst;
	// 		}
	// 	} else {
	// 	}

	// 	knet_dlog("on read package ");
	// 	return dataLen;
	// }

	int send_text(const std::string& msg, OpCode op = TEXT_FRAME, bool mask = true) {
		knet_dlog("send text length is {}", msg.length());
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
		knet_dlog("send head length {}, message length {} ", header.size(),sendMsg.length()); 
		return this->msend(std::string((char*)header.data(), header.size()), sendMsg);
	}

	void parse_message(const char* pData, uint32_t len) {
		uint8_t* vals = (uint8_t*)pData;
		knet_dlog("fin is     : {}" , ((vals[0] & 0x80) == 0x80) );
		knet_dlog("opcode is  : {}" , (vals[0] & 0x0f) );
		knet_dlog("length is  : {}" , (vals[1] & 0x7F) );
		knet_dlog("mask is    : {}" , ((vals[1] & 0x80) == 0x80) );

		// uint64_t payloadLen = (vals[1] & 0x7f);
		// uint32_t headSize = sizeof(uint16_t);
	}

	// will invoke in multi-thread , if you want to process it main thread , push it to msg queue
	// int32_t handle_raw_data(char* pBuf, uint32_t len) {
	// 	// knet_dlog(" connection id %d on thread %d", m_id, std::this_thread::get_id());

	// 	if (m_status == WSockStatus::WSOCK_CONNECTING) {
	// 		size_t parseRst = http_parser_execute(&m_parser, &m_parse_settings, pBuf, len);

	// 		knet_dlog("received message \n{}", pBuf);
	// 		knet_dlog("received data length is {} parse result is {}", len, parseRst);
	// 		if (m_parser.upgrade) {
	// 			m_status = WSockStatus::WSOCK_OPEN;
	// 			knet_dlog("success upgrade to websocket ");
	// 			is_websocket = true;
	// 			// this->send_text("hello world");
	// 			return parseRst;
	// 		}
	// 	} else {
	// 		knet_dlog("received buffer length {}", len);
	// 		uint32_t ret = wsock_reader.read(pBuf, len, message_handler);

	// 		knet_dlog("have read buffer data {}", ret);
	// 		return ret;
	// 	}

	// 	return len;
	// }

	uint32_t read_websocket(const char * data, uint32_t len ) {
		return wsock_reader.read(data, len , std::bind(&WSockSslConnection::read_message, this, std::placeholders::_1 ));
	}

	uint64_t get_message_length(const char * data, uint32_t len ){	
		uint64_t headSize  = sizeof(uint16_t); 
		if (len < sizeof(MinFrameHead)){
			return 0; 
		}
		uint8_t* vals = (uint8_t*)data;
		uint32_t payloadMark = vals[1] & 0x7f;
		 uint32_t mask = ((vals[1] & 0x80) == 0x80);
		 if (mask )
		 {
			 headSize += sizeof(uint32_t ); 
		 }
	
		if (payloadMark < 126) {	 
			return headSize +  payloadMark ; 
		}else if (payloadMark == 126) {	
			headSize += sizeof(uint16_t);	
			uint16_t payloadLen = 0; 	 
			payloadLen= *(uint16_t*)((uint16_t*)data + 1);
			payloadLen = ntohs(payloadLen);
			return headSize + payloadLen; 
		}else {
			headSize += sizeof(uint64_t);	
			uint64_t payloadLen = 0; 	 
			payloadLen = *(uint64_t*)((uint16_t*)data + 1);
			payloadLen = ntohll(payloadLen);	
			return headSize + payloadLen; 
		}

		return 0; 
	}

	virtual int32_t handle_package(const char * data, uint32_t len ){
	
		// MinFrameHead head = {0};		
		// head.fin = ((vals[0] & 0x80) == 0x80);
		// head.opcode = (vals[0] & 0x0f);
		// head.len = (vals[1] & 0x7F);
		// head.mask = ((vals[1] & 0x80) == 0x80);
		if (is_websocket){
			uint64_t wsLen = get_message_length(data, len); 
			if (wsLen > len ){
				return 0; 
			}		
			return wsLen;
		}else {		

			if (is_passive()){
				current_request = std::make_shared<HttpRequest>();
				auto msgLen = current_request->parse(data, len, true);
				knet_dlog("parse request length {}\n{}",len,  data); 
				return msgLen; 
			}else {
				current_response = std::make_shared<HttpResponse>();
				auto msgLen = current_response->parse(data, len, true);
				knet_dlog("parse response length {}\n{}",len,  data); 
				return msgLen; 
			}
			
			return len; 
		}		
	}

	HttpRequestPtr current_request; 
	HttpResponsePtr current_response; 

	bool upgrade_websocket(const HttpRequest &  req) {

		std::string_view secWebSocketKey = req.get_header("Sec-WebSocket-Key");

		if (secWebSocketKey.length() == 24) {

			char secWebSocketAccept[29] = {};
			WSockHandshake::generate(secWebSocketKey.data(), secWebSocketAccept);

			HttpResponse rsp("", 101);
			rsp.add_header("Upgrade", "websocket");
			rsp.add_header("Connection", "Upgrade");
			rsp.add_header("Sec-WebSocket-Accept", secWebSocketAccept);
//			rsp.add_header("Accept-Encoding","gzip, deflate"); 
			write(rsp);
			if (req.is_websocket()){
				set_status(WSockStatus::WSOCK_OPEN);
				is_websocket = true;
			}			
		
			knet_dlog("upgrade websocket success");
			
			return true;
		}else {
			knet_elog("bad websocket key {}", secWebSocketKey);
			write("bad websocket key",400); 
		}
		

		return false;
	}

	void write(const HttpResponsePtr rsp) {
		this->send(rsp->to_string());

		if (rsp->code() >= 200) {
			this->close();
		}
	}

	void write(const HttpResponse& rsp) {
		this->send(rsp.to_string());
		if (rsp.code() >= 200) {
			this->close();
		}
	}

	WSockStatus get_status() { return m_status; }
	void set_status(WSockStatus status) { m_status = status; }
	bool is_status(WSockStatus status) { return m_status == status; }

	void write(const std::string& msg, uint32_t code = 200, bool fin = true) {
		HttpResponse rsp(msg, code);

		if (this->is_connected()) {
			this->send(rsp.to_string());
		}
		if (fin) {
			this->close();
		}
	}


	void read_message(const std::string_view& msg  )
	{
		if (wsock_handler.message)
		{
			wsock_handler.message(this->shared_from_this() , std::string(msg.data(), msg.size()) ); 
		}
	}
	
	bool is_websocket = false;

	HttpRequestPtr first_request;
	WSMessageReader wsock_reader;
	WSockHandler<WSockSslConnection>  wsock_handler; 

private: 
	WSockStatus m_status = WSockStatus::WSOCK_INIT; 	
};

} // namespace websocket
} // namespace knet
