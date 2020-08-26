#include "knet.hpp"

#include "udp_connector.hpp"
#include <chrono>

using namespace knet::udp; 

class MyConnection : public UdpConnection<MyConnection >
{

public:
 
	virtual PackageType on_message(const std::string & msg ) {
		wlog("on recv udp message {} , lenght is {}", msg.data(), msg.length());
		return PACKAGE_USER;
	}
}; 

int main(int argc , char * argv[])
{   

    UdpConnector<MyConnection> connector; 
    connector.start(); 

    auto conn = connector.connect("127.0.0.1",9000); 

    while(1){

        conn->send("hello world",11);  
		dlog("udp client log "); 
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    }

    return 0; 
}
