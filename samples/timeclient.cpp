#include <stdio.h>
#include "knet.hpp"
#include <iostream>
#include <sys/time.h> 

using namespace knet::tcp;
struct TestMsg{
    uint32_t length; 
    struct timeval time; 
    uint64_t index; 
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

	virtual bool handle_data(const std::string& msg)
	{
        TestMsg * tMsg = (TestMsg*) msg.c_str(); 
        TestMsg recvMsg; 
        gettimeofday(&recvMsg.time,0); 
        total_time  +=  (recvMsg.time.tv_usec - tMsg->time.tv_usec) ; 
        dlog("from {}:{} => {}:{} elapse {} avg {}", tMsg->time.tv_sec, tMsg->time.tv_usec, recvMsg.time.tv_sec, recvMsg.time.tv_usec, recvMsg.time.tv_usec - tMsg->time.tv_usec,  tMsg->index - last_index ); 
        last_index = tMsg->index; 

		return true;
	}
	virtual bool handle_event(knet::NetEvent evt) {

//        dlog("handle net event {}", evt); 

		if (evt == knet::NetEvent::EVT_CONNECT)
		{
            TestMsg tMsg; 
            gettimeofday(&tMsg.time,0); 
			this->send((const char *)&tMsg, sizeof(TestMsg) );
		}
		return true; 
	}

    uint64_t total_time = 0 ; 
    uint64_t last_index = 0; 
};

int main(int argc, char** argv)
{
	KNetLogIns.add_console(); 
	dlog("init client ");
	TcpConnector<TcpSession>  connector;

	connector.start();

    std::string host = "127.0.0.1"; 

    if (argc > 1) 
    {
        host = argv[1]; 
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



	auto conn = connector.add_connection({"tcp", host, 8888});
	//conn->enable_reconnect(); 

    uint64_t index = 1; 
    while(true){

        if (conn){

            TestMsg recvMsg; 
            gettimeofday(&recvMsg.time,0); 
            recvMsg.index = index ++; 
            conn->send((const char *)&recvMsg, sizeof(TestMsg) );
            usleep(10); 

        }

    }

//
//	char c = getchar();
//	while (c)
//	{
//		if (c == 'q')
//		{
//			break;
//		}
//		c = getchar();
//	}
//
	return 0;
}

