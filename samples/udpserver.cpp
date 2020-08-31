
#include <chrono>
#include "knet.hpp"
 

#include "udp/udp_listener.hpp"

using namespace knet::udp;
bool running = true; 

class MyConnection : public UdpConnection<MyConnection> {

	public:
 
	virtual PackageType on_package(const std::string& msg) {
	    total ++; 

	    if (total %10000 == 0){
		auto end   = system_clock::now();
		auto duration = duration_cast<microseconds>(end - start);
//		std::cerr <<  "asio spend " << double(duration.count()) * microseconds::period::num / microseconds::period::den << " seconds" << std::endl;
		start = end;
	    }

	    if (total > 1000000) {
		    running = false; 

	    }

	//	ilog("on recv udp message {} , lenght is {} ,cid is ", msg, msg.length(),cid); 
		//std::string rsp = "message from server" ; 
	//	send(msg.data(), msg.length()); 
		return PACKAGE_USER;
	}
	

	uint64_t total =0; 
	std::chrono::system_clock::time_point start; 

};
int main(int argc, char* argv[]) {

	UdpListener<MyConnection> udpListener;
	udpListener.start([](MyConnection::TPtr conn, knet::NetEvent evt, const std::string &dv) {
		dlog("received connection event {}", evt);
		if (conn)
		{
		//std::string msg= "message from server" ; 
		//conn->send(msg.c_str(), msg.length()); 
		}
		return nullptr;
	},9000);

	while (running) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
