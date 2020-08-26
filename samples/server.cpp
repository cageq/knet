#include <stdio.h>
#include "knet.hpp"
#include <iostream>
#include <memory>
#ifdef __linux__
#include <malloc.h>
#endif 

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession> {
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() {
			bind_data_handler(&TcpSession::on_recv ); 

			bind_event_handler([](  TcpSessionPtr,NetEvent ){
					return 0; 
					} ); 
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
		// int read_packet(const char *pData, uint32_t dataLen) {
		// 	return dataLen;
		// }

		void on_disconnect() {
			dlog("on session disconnect");
			//   trace;
		}

		//will invoke in multi-thread , if you want to process it main thread , push it to msg queue
		uint32_t on_recv(const std::string_view & msg , MessageStatus status) {
			//		dlog(" connection id  on thread {}", m_id, std::this_thread::get_id());
			if (status == MessageStatus::MESSAGE_NONE)
			{
				if (msg.length() < 4) 
				{
					wlog("buffer not enough for head"); 
					return 0; 
				}
				dlog("process head first {} :{}", msg.length() , msg ); 

				return 10; 
			}
			else {
				dlog("process data chunk {} :{}", msg.length() , msg); 
				return msg.length() ;
			}
	//		this->send(pBuf, 10);
			return 10; 
		}

	private:
		int m_id;
};

int main(int argc, char **argv)
{
	dlog("start server");
	TcpListener<TcpSession> listener;
	int port = 8899;
	listener.start(  port); 
	dlog("start server on port {}", port);
	char c = getchar();
	while (c)
	{

		if (c == 'q')
		{
			printf("quiting ...\n");
			break;
		}
		else if (c == 'f') {
#ifdef __linux__
			malloc_trim(0); 
#endif 
		}
		c = getchar();
	}

	dlog("quit server");
	return 0;
}
