#include <stdio.h>
#include <iostream>
#include "knet.hpp"
#include "knet_handler.hpp"

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession>
{
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() { }

		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}
 
};


class MyFactory: public knet::KNetFactory<TcpSession>, public knet::KNetHandler<TcpSession> { 

	public:

		virtual void destroy(TPtr conn) {
			dlog("connection factory destroy connection in my factory "); 
		}	

		virtual bool handle_event(TPtr conn, knet::NetEvent evt) {
			ilog("handle event in connection my factory {}", evt ); 
			return true; 
		}

		virtual bool handle_data(TPtr conn, const std::string & msg ) { 
			conn->send(msg); 
			return msg.length() ;
		}; 

}; 

int main(int argc, char **argv)
{
 KNetLogIns.add_console(); 
	MyFactory factory; 
	dlog("start server");
	TcpListener<TcpSession,MyFactory> listener(&factory);
	int port = 8899;
	listener.start(  port); 
	//	listener->bind_event_handler( [](NetEvent evt, std::shared_ptr<TcpSession>) {
	//			switch (evt)
	//			{
	//			case EVT_CREATE:
	//			dlog("on create connection");
	//			break;
	//			case EVT_CONNECT:
	//			dlog("on connection established");
	//			break;
	//			case EVT_CONNECT_FAIL:
	//			break;
	//
	//			default:;
	//			}
	//
	//			return 0;
	//			});
	dlog("start server on port {}", port);

	char c = getchar();
	while (c)
	{
		if (c == 'q')
		{
			printf("quiting ...\n");
			break;
		}
		c = getchar();
	}

	listener.stop(); 
	dlog("quit server");
	return 0;
}
