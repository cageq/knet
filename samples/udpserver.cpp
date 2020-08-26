
#include <chrono>
#include "knet.hpp"
 

#include "udp/udp_listener.hpp"

using namespace knet::udp;

class MyConnection : public UdpConnection<MyConnection> {

	public:
 
	virtual PackageType on_package(const std::string& msg) {
		ilog("on recv udp message {} , lenght is {} ,cid is ", msg, msg.length(),cid); 
		//std::string rsp = "message from server" ; 
		send(msg.data(), msg.length()); 
		return PACKAGE_USER;
	}

};
int main(int argc, char* argv[]) {

	UdpListener<MyConnection> udpListener;
	udpListener.start([](MyConnection::TPtr conn, NetEvent evt, std::string dv) {
		dlog("received connection event {}", evt);
		if (conn)
		{
		//std::string msg= "message from server" ; 
		//conn->send(msg.c_str(), msg.length()); 
		}
		return nullptr;
	},8700);

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
