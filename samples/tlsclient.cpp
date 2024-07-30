#include <stdio.h>
#define KNET_WITH_OPENSSL 
#include "knet.hpp"
#include <iostream>
//#include "tcp/tls_socket.hpp"

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession, TlsSocket<TcpSession>> {
public:
	using TcpSessionPtr = std::shared_ptr<TcpSession>;
	TcpSession() {

        /*
		bind_event_handler([](TcpSessionPtr session, knet::NetEvent evt ) {
			knet_dlog("handle event in event handler");
			if (evt == knet::NetEvent::EVT_CONNECT)
			{
				session->send(std::string("hello workd"));
			}
			

			return 0;
		});
        */
	}

	void on_connect() {
		knet_dlog("on connected ");
		std::string msg("hello world");
		this->send(msg.c_str(), msg.length());
	}
    virtual bool handle_event(knet::NetEvent evt) {
        knet_dlog("handle event in event handler");
			if (evt == knet::NetEvent::EVT_CONNECT)
			{
				this->send(std::string("hello workd"));
			}
        return true; 
    }

	// WARN: if data length(datalLen) is less than a full package size ,please return -1;
	//警告:如果包长(datalLen)小于一个完整消息包长度或包头长度，请返回-1 ,否则会导致死循环
	int read_packet(const char* pData, uint32_t dataLen) { return dataLen; }

	void on_disconnect() {
		// trace;
		knet_dlog("on disconnected");
	}

	void on_recv(const char* pBuf, uint32_t len) {
		//    trace;
		knet_dlog("received data %s ", pBuf);
		std::string msg(pBuf, len);
	//	this->send(msg.c_str(), len);
	}
};

int main(int argc, char** argv) {

	knet_add_console_sink();
	knet_dlog("init client ");
	TcpConnector<TcpSession> connector;

	//( [](NetEvent evt, std::shared_ptr<TcpSession> pArg) {

	//  switch(evt)
	//  {
	//  case EVT_CONNECT:
	//  break;
	//  case EVT_CONNECT_FAIL:
	//  break;
	//  case EVT_LISTEN:
	//  break;
	//  case EVT_LISTEN_FAIL:
	//  break;
	//  default:
	//  ;
	//  }

	//  return 0;
	//  });

	// for (int i = 0; i < 100; i++)
//	{
	auto conn = 	connector.add_connection({"tcp", "127.0.0.1", 8899});
		// connector->add_connection("10.246.34.55", 8899);
	//}
    while(1){

        std::string msg = "hello world" ; 
        conn->send(msg); 

        knet_dlog("send message {}" , msg); 
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
    }

	char c = getchar();
	while (c) {
		if (c == 'q') {
			break;
		}
		c = getchar();
	}

	return 0;
}
