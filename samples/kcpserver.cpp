
#include <chrono>
#include "knet.hpp"

#include "kcp/kcp_listener.hpp"

using namespace knet::kcp;

class MyConnection : public KcpConnection<MyConnection> {

	public:
 
	virtual ~MyConnection(){}
    virtual int32_t handle_package(const char* data, uint32_t len) { 
		knet_ilog("on message {} , lenght  {} ,cid is {}", data, len,cid); 
		this->send("response from server"); 
		return len;
	}
};

int main(int argc, char* argv[]) {

	knet_add_console_sink();  
	knet_dlog("start kcp server"); 
	knet::KNetWorkerPtr worker = std::make_shared<knet::KNetWorker>(); 
	worker->start(nullptr, 4); //4 threads 

	KcpListener<MyConnection> kcpLis(worker);
	kcpLis.start(8700, [](MyConnection::TPtr, knet::NetEvent evt, const std::string & dv) {
		knet_ilog("received connection event {} , {}", static_cast<uint32_t>(evt),event_string(evt));
		return nullptr;
	});

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
