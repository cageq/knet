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
			bind_event_handler([this](  knet::NetEvent ){

			std::string msg("hello world"); 
			this->send(msg.c_str(),msg.length()); 
					return true; 
					} ); 
	
		}

		int32_t process_data(const std::string & msg , knet::MessageStatus status)
		{
			//    trace;
			dlog("received data {} ",msg); 
			this->send(msg.data(),msg.length());   
			return msg.length(); 
		}
}; 

int main(int argc, char** argv)
{
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


	for (int i = 0; i < 1; i++)
	{
		connector.add_connection({"127.0.0.1", 8899});
		//connector->add_connection("10.246.34.55", 8855);
	}

	char c = getchar(); 
	while (c)
	{
		if (c == 'q')
		{
			break;
		}	
		c = getchar()	; 
	}

	return 0;
}

