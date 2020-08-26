#include <stdio.h>
#include "knet.hpp"
#include <iostream>
// #include "co/coevent_worker.hpp"
using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession>{
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() { 
			bind_data_handler(&TcpSession::on_recv ); 
			bind_event_handler([](   TcpSessionPtr, knet::NetEvent evt  ){ 
					ilog("on recv event", evt);
					return 0; 
					} ); 
		}
		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}
	
		//will invoke in multi-thread , if you want to process it main thread , push it to msg queue
		uint32_t on_recv(const std::string & msg , knet::MessageStatus status) {
			//		dlog(" connection id %d on thread %d", m_id, std::this_thread::get_id());
			this->send(msg);
			return msg.length(); 		
		}
};

int main(int argc, char **argv)
{
 

	// auto lisWorker = std::make_shared<knet::CoEventWorker>();
	// lisWorker->start(); 
 

	// std::vector<knet::EventWorkerPtr> workers ; 
	// workers.emplace_back(std::make_shared<knet::CoEventWorker>()); 
	// for(auto & worker : workers){
	// 	worker->start(); 
	// }
	// ConnectionFactory<TcpSession> factory; 
	// TcpListener<TcpSession> listener(&factory, workers); 


	TcpListener<TcpSession> listener; 
	int port = 8855;
	listener.start( port); 

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
  	//co_sched.Start(4);

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
