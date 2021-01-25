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
		bind_event_handler([this](knet::NetEvent) {

			std::string msg("hello world");
			this->send(msg.c_str(), msg.length());
			return true;
			});

	}

	virtual bool process_data(const std::string& msg)
	{
		//    trace;
		dlog("received data {} ", msg);
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

	while (1) {
		conn->send("hello world");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

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

