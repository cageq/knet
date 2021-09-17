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

	}
	virtual int32_t handle_package(const char* data, uint32_t len) {
		dlog("received package length is {}", len);
		return len;
	}

	virtual bool handle_data(const std::string_view& msg)
	{

		dlog("received data   {} ", msg );
		std::string mymsg("hello world");
		//this->send(mymsg);

		return true;
	}
	virtual bool handle_event(knet::NetEvent evt) {

		if (evt == knet::NetEvent::EVT_CONNECT)
		{
			std::string msg("hello world");
			this->send(msg.c_str(), msg.length());
//			this->msend(123445,msg);
		}
		return true; 
	}


};

int main(int argc, char** argv)
{
	KNetLogIns.add_console(); 
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


	auto conn = connector.add_connection({ "127.0.0.1", 8888});
	//connector->add_connection("10.246.34.55", 8855);
	conn->enable_reconnect(); 


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

