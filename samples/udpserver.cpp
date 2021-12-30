
#include <chrono>
#include "knet.hpp"
#include "udp/udp_listener.hpp"

using namespace knet::udp;
bool running = true; 

class MyConnection : public UdpConnection<MyConnection> {

	public:
	virtual ~MyConnection(){}
 
	virtual PackageType handle_package(const std::string& msg) {
	    total ++; 

	    if (total %20000 == 0){

		auto now = std::chrono::high_resolution_clock::now();                                          
		std::chrono::duration<double, std::micro> period = now - measure_time;                         
		//auto period = std::chrono::duration_cast<std::chrono::milliseconds>(now - measure_time);     
		measure_time  = now ;                                                                          
		std::cout << "Package count " << total<< " elapse "<< period.count() << std::endl;   

//		auto end   = system_clock::now();
//		auto duration = duration_cast<microseconds>(end - start);
//		std::cerr <<  "asio spend " << double(duration.count()) * microseconds::period::num / microseconds::period::den << " seconds" << std::endl;
//		start = end;
	    }

	//    if (total > 1000000) {
	//	    running = false; 
	//    }

	//	ilog("on recv udp message {} , lenght is {} ,cid is ", msg, msg.length(),cid); 
		//std::string rsp = "message from server" ; 
	//	send(msg.data(), msg.length()); 
		return PACKAGE_USER;
	}
	

	uint64_t total =0; 
	std::chrono::time_point<std::chrono::high_resolution_clock> measure_time = std::chrono::high_resolution_clock::now(); 

};
int main(int argc, char* argv[]) {

	KNetLogIns.add_console(); 
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
