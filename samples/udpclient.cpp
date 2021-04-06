#include "knet.hpp"

#include "udp/udp_connector.hpp"
#include <chrono>
#include "console_sink.hpp"

using namespace knet::udp; 

class MyConnection : public UdpConnection<MyConnection >
{
    public:

    virtual ~MyConnection(){}
	virtual PackageType on_message(const std::string & msg ) {
	    wlog("on recv udp message {} , lenght is {}", msg.data(), msg.length());
	    return PACKAGE_USER;
	}
}; 

int main(int argc , char * argv[])
{   

	add_console_logger(); 
    UdpConnector<MyConnection> connector; 
    connector.start(); 

    //auto conn = connector.connect("192.168.110.12",9000); 
    auto conn = connector.connect("127.0.0.1",9000); 

	while(1){
		conn->send("hello world",11);  
//		dlog("udp client log "); 
		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    }

    return 0; 
}
