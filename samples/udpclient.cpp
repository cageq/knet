#include "knet.hpp"

#include "udp/udp_connector.hpp"
#include <chrono>

using namespace knet::udp; 

class MyConnection : public UdpConnection<MyConnection >
{
    public:

    virtual ~MyConnection(){}
	virtual PackageType handle_package(const std::string & msg ) {
	    knet_wlog("on recv udp message {} , lenght is {}", msg.data(), msg.length());
	    return PACKAGE_USER;
	}
    virtual bool handle_data(const std::string &msg)
    {
        knet_dlog("hand udp message {}",msg); 
        return true; 
    }
}; 

int main(int argc , char * argv[])
{   

	knet_add_console_sink(); 
    UdpConnector<MyConnection> connector; 
    connector.start(); 

    //auto conn = connector.connect("192.168.110.12",9000); 
    auto conn = connector.connect("127.0.0.1",9000); 

	while(1){
		conn->send("hello world",11);  
//		knet_dlog("udp client log "); 
		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    }

    return 0; 
}
