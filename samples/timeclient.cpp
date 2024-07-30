#include <stdio.h>
#include "knet.hpp"
#include <iostream>
#include <chrono>
#include <sys/time.h> 

using namespace knet::tcp;

struct TestMsg{
    uint32_t length; 
    struct timeval server_time; 
    struct timeval time; 
    uint64_t index; 
	char data[70]; 
}; 

class TcpSession : public TcpConnection<TcpSession >
{
public:
	typedef std::shared_ptr<TcpSession> TcpSessionPtr;

	TcpSession()
	{

	}
	virtual int32_t handle_package(const char* data, uint32_t len) {
        if (len < sizeof (TestMsg) ){
            return -1; 
        }

		return sizeof(TestMsg);
	}

	virtual bool handle_data(char * data, uint32_t dataLen)
	{
        TestMsg * tMsg = (TestMsg*) data; 
        TestMsg recvMsg; 
        gettimeofday(&recvMsg.time,0); 
        //total_time  +=  (recvMsg.time.tv_usec - tMsg->time.tv_usec) ; 
       // knet_dlog("{}# {}:{} => {}:{} elapse {}",tMsg->index, tMsg->time.tv_sec, tMsg->time.tv_usec, recvMsg.time.tv_sec, recvMsg.time.tv_usec, recvMsg.time.tv_usec - tMsg->time.tv_usec  ); 

	    knet_dlog("{}# spent {}",tMsg->index,   (recvMsg.time.tv_sec * 1000000 + recvMsg.time.tv_usec) - (tMsg->time.tv_sec * 1000000 + tMsg->time.tv_usec)   ); 
        if (last_index +1 != tMsg->index){
            knet_elog("wrong seqence"); 
            exit(0); 
        }
        last_index = tMsg->index; 
		return true;
	}
	virtual bool handle_event(knet::NetEvent evt) {

//        knet_dlog("handle net event {}", evt); 

		if (evt == knet::NetEvent::EVT_CONNECT)
		{
            last_index = 0 ; 
            //TestMsg tMsg; 
            //gettimeofday(&tMsg.time,0); 
			//this->send((const char *)&tMsg, sizeof(TestMsg) );
		}
		return true; 
	}

    uint64_t total_time = 0 ; 
    uint64_t last_index = 0; 
};

int main(int argc, char** argv)
{
	knet_add_console_sink();  
	knet_dlog("init client ");
	TcpConnector<TcpSession>  connector;

	connector.start();

    std::string host = "127.0.0.1"; 


    bool asyncSend = true; 
    uint16_t port = 8888; 
    if (argc > 1) 
    {
        host = argv[1]; 
    }
    if (argc > 2){
        port = atoi(argv[2]); 
    }

    if (argc > 3){
        asyncSend = atoi(argv[3]) > 0; 
    }

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


    //KNetUrl url{"tcp",host, 8888}; 
    //url.set("delay", "0"); 

	auto conn = connector.add_connection({"tcp", host, port});
	conn->enable_reconnect(); 

    uint64_t index = 1; 
	uint32_t speed = 2000; 
    while(true){
        
        //knet_dlog("connection status {}", conn->is_connected()); 
        if (conn && conn->is_connected()){

            TestMsg recvMsg; 
            gettimeofday(&recvMsg.time,0); 
            recvMsg.index = index ++; 
            if (asyncSend){
                conn->send((const char *)&recvMsg, sizeof(TestMsg) );
            }else {
                conn->sync_send((const char *)&recvMsg, sizeof(TestMsg) );
            }
        }else{
            knet_dlog("not connected"); 
        }

		if (index %speed == 0)
		{
//			struct timespec interval{0}; 
//			interval.tv_nsec = 2000000; 
//			nanosleep(&interval, nullptr); 
//			usleep(1000000); 

			std::this_thread::sleep_for(std::chrono::microseconds(1)); 
		}

        if ( index >= 4000000){
			printf("finished\n"); 
            break; 
        }

    }

//
	char c = getchar();
//	while (c)
//	{
//		if (c == 'q')
//		{
//			break;
//		}
//		c = getchar();
//	}
//
    connector.stop(); 
	return 0;
}

