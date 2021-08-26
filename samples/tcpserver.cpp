#include <stdio.h>
#include "knet.hpp"
#include <iostream>


// #include "co/coevent_worker.hpp"
using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession>{
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() { 
			//ilog("session create with {}", val);
			// bind_data_handler(&TcpSession::on_recv ); 
			//bind_event_handler([](   TcpSessionPtr, knet::NetEvent evt  ){ 
			//		ilog("on recv event", evt);
			//		return 0; 
			//		} ); 
		}
		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}
		virtual bool handle_event(knet::NetEvent evt) {

			dlog("handle my tcp event {}", evt); 

			return true; 
		}
	
		//will invoke in multi-thread , if you want to process it main thread , push it to msg queue
		virtual bool handle_data(const std::string &msg ) {
			dlog("handle data is {}", msg); 
			  dlog("handle data length {} ", msg.length()  ); 			
			//this->send(msg);
			
			std::string rst= "{\"code\":0, \"msg\":\"success\"}"; 
			this->send(rst); 
			return true; 		
		}
};

int main(int argc, char **argv)
{
	KNetLogIns.add_console(); 
 
    // dout << "test tcp server with cout format " << std::endl; 
    // iout << "test tcp server with cout format " << std::endl; 
    // wout << "test tcp server with cout format " << std::endl; 
    // eout << "test tcp server with cout format " << std::endl; 
    // dput("dddd", 333, 4444, 899.222); 
    // iput("adfasdfasdf", 21323131, 2323433); 

	// auto lisWorker = std::make_shared<knet::CoEventWorker>();
	// lisWorker->start(); 
 

	// std::vector<knet::EventWorkerPtr> workers ; 
	// workers.emplace_back(std::make_shared<knet::CoEventWorker>()); 
	// for(auto & worker : workers){
	// 	worker->start(); 
	// }
	// ConnFactory<TcpSession> factory; 
	// TcpListener<TcpSession> listener(&factory, workers); 

	std::shared_ptr<knet::KNetWorker> myworker = std::make_shared<knet::KNetWorker>();
	myworker->start();
	//TcpListener<TcpSession, ConnFactory<TcpSession>,  knet::KNetWorker, int32_t> listener(myworker,222); 
	DefaultTcpListener<TcpSession> listener(myworker); 
	int port = 8888;
	bool ret = listener.start( port); 

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
	dlog("start server on port {} , status {}", port,ret );
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
    listener.stop(); 

	//dlog("quit server");
	return 0;
}
