#include <stdio.h>
#include "knet.hpp"
#include <iostream>

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession >
{
public:
	typedef std::shared_ptr<TcpSession> TcpSessionPtr;

	TcpSession()
	{
		bind_data_handler(&TcpSession::process_data);
		bind_event_handler([this](knet::NetEvent evt ) {

				if (evt == knet::NetEvent::EVT_CONNECT)
				{
					std::string msg("hello world");
					this->send(msg.c_str(), msg.length());
				}
			
			return true;
			});

	}
	virtual int32_t handle_package(const char* data, uint32_t len) {
		dlog("received package length is {}", len);
		return len;
	}
	virtual bool process_data(const std::string& msg)
	{
		dlog("received data length {} ", msg.length());
		this->send(msg.data(), msg.length());
		return true;
	}
};

int main(int argc, char** argv)
{
	kLogIns.add_sink<klog::ConsoleSink<std::mutex, true> >();
	dlog("init client ");
	TcpConnector<TcpSession>  connector;

	connector.start();


	//([](NetEvent evt, std::shared_ptr<TcpSession> pArg) {

	// switch(evt)
	// {
	// case EVT_CONNECT:
	// break; 
	// case EVT_CONNECT_FAIL:
	// break; 
	// case EVT_LISTEN:
	// break; 
	// case EVT_LISTEN_FAIL:
	// break; 
	// default:
	// ;
	// }

	// return 0; 
	// });


	auto conn = connector.add_connection({ "127.0.0.1", 8855 });
	//connector->add_connection("10.246.34.55", 8855);
 

	char c = getchar();
	while (c)
	{
		if (c == 'q')
		{
			break;
		}
		c = getchar();
	}

	return 0;
}

