#include <stdio.h>
#include "knet.hpp"
#include <iostream>

#include <sys/time.h>
// #include "co/coevent_worker.hpp"
using namespace knet::tcp;

struct TestMsg
{
	uint32_t length;
    struct timeval server_time; 
	struct timeval time;
	uint64_t index;
	char  data[70]; 
};

bool asyncSend = true; 
class TcpSession : public TcpConnection<TcpSession>
{
public:
	typedef std::shared_ptr<TcpSession> TcpSessionPtr;
	TcpSession()
	{
		// knet_ilog("session create with {}", val);
		//  bind_data_handler(&TcpSession::on_recv );
		// bind_event_handler([](   TcpSessionPtr, knet::NetEvent evt  ){
		//		knet_ilog("on recv event", evt);
		//		return 0;
		//		} );
	}
	virtual ~TcpSession()
	{
		knet_dlog("destroy tcp session");
	}
	virtual bool handle_event(knet::NetEvent evt)
	{

        if (evt == knet::NetEvent::EVT_DISCONNECT){
            last_index  = 0; 
        }
		//	knet_dlog("handle my tcp event {}", evt);

		return true;
	}
	virtual int32_t handle_package(const char *data, uint32_t len)
	{
		if (len < sizeof(TestMsg))
		{
			return -1;
		}
 
		return sizeof(TestMsg);
	}

	// will invoke in multi-thread , if you want to process it main thread , push it to msg queue
	virtual bool handle_data(char * data, uint32_t dataLen)
	{

		TestMsg *tMsg = (TestMsg *)data;
		TestMsg recvMsg;
		gettimeofday(&recvMsg.time, 0);
		//total_time += (recvMsg.time.tv_usec - tMsg->time.tv_usec);
		// knet_dlog("{}# {}:{} => {}:{} elapse {}",tMsg->index, tMsg->time.tv_sec, tMsg->time.tv_usec, recvMsg.time.tv_sec, recvMsg.time.tv_usec, recvMsg.time.tv_usec - tMsg->time.tv_usec  );

	    knet_dlog("{}# spent {}",tMsg->index,   (recvMsg.time.tv_sec * 1000000 + recvMsg.time.tv_usec) - (tMsg->time.tv_sec * 1000000 + tMsg->time.tv_usec)   ); 
		if (last_index + 1 != tMsg->index)
		{
			knet_elog("wrong seqence");
			exit(0);
		}
		last_index = tMsg->index;
        if (asyncSend){
            this->send(data, dataLen );
        }else{
            this->sync_send(data, dataLen); 
        }

		return true;
	}
	uint64_t total_time = 0;
	uint64_t last_index = 0;
};

int main(int argc, char **argv)
{
	knet_add_console_sink(); 

	int port = 8888;
    if (argc > 1){
        port = atoi(argv[1]); 
    }
    if (argc > 2){
        asyncSend = atoi(argv[2]) > 0; 
    }

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

	knet::NetOptions netOpts{}; 
	netOpts.sync_accept_threads = 4; 
	// TcpListener<TcpSession, ConnFactory<TcpSession>,  knet::KNetWorker, int32_t> listener(myworker,222);
	DefaultTcpListener<TcpSession> listener(myworker);
	bool ret = listener.start(port, netOpts);

	// tcpService.start();
	//  static int index  = 0;
	//	listener->bind_event_handler( [](NetEvent evt, std::shared_ptr<TcpSession>) {
	//			switch (evt)
	//			{
	//			case EVT_CREATE:
	//			knet_dlog("on create connection");
	//			break;
	//			case EVT_CONNECT:
	//			knet_dlog("on connection established");
	//			break;
	//			case EVT_CONNECT_FAIL:
	//			break;
	//
	//			default:;
	//			}
	//
	//			return 0;
	//			});
	knet_dlog("start server on port {} , status {}", port, ret);
	// co_sched.Start(4);

	while(1){

		std::this_thread::sleep_for(std::chrono::seconds(1)); 
	}
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

	// knet_dlog("quit server");
	return 0;
}
