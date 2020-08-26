#include <stdio.h>
#include "knet.hpp"
#include <iostream>

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession>
{
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() { }

		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}

	private:
		int m_id;
};


class MyFactory: public ConnectionFactory<TcpSession> { 

	public:

		virtual void destroy(TPtr conn) {
			dlog("connection factory destroy connection in my factory "); 
		}	

		virtual void handle_event(TPtr conn, knet::NetEvent evt) {
			ilog("handle event in connection my factory"); 
		}

		virtual int32_t handle_data(TPtr conn, char * data, uint32_t len) { 
			conn->send(data,len); 
			return len ;
		}; 

}; 

int main(int argc, char **argv)
{
 
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
