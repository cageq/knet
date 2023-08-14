#include <stdio.h>

#include <iostream>

#define KNET_WITH_OPENSSL 
#include "knet.hpp"
#include "tls_context.hpp"
 

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession, TlsSocket<TcpSession> >
{
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() {
		 
		}

		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}

		// connection events
		void on_connect() {
			dlog("on connected "); 
		}
		//read on buffer for one package
		//return one package length ,if not enough return -1

		void on_disconnect()
		{
			dlog("on session disconnect");
			//   trace;
		}

		//will invoke in multi-thread , if you want to process it main thread , push it to msg queue

		bool handle_data(const std::string_view & msg ) {
			//		dlog(" connection id %d on thread %d", m_id, std::this_thread::get_id());
					dlog("received data {} ", msg);
                    return true; 
			
		}

	private:
		int m_id;
};

int main(int argc, char **argv)
{
	KNetLogIns.add_console();
	dlog("start server");
	TcpListener<TcpSession > listener; 
	uint16_t  port = 8899;
	auto sslCtx = init_ssl("ca/kstar.pem", "cert/dh2048.pem" ); 
	listener.start({"tcp", "127.0.0.1", port}, {}, sslCtx); 

	//tcpService.start();
	// static int index  = 0;
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

	dlog("quit server");
	return 0;
}
