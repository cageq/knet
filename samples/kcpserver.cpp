
#include <chrono>
#include "knet.hpp"

#include "kcp/kcp_listener.hpp"
#include "console_sink.hpp"

using namespace knet::kcp;

class MyConnection : public KcpConnection<MyConnection> {

	public:
	MyConnection(asio::io_context &ctx):KcpConnection<MyConnection>(ctx){ 
		cid = 8888; 

	}
	virtual ~MyConnection(){}
    virtual PackageType on_message(const char* data, uint32_t len) { 
		ilog("on recv udp message {} , lenght is {} ,cid is ", data, len,cid); 
		this->send("response from server"); 
		return PACKAGE_USER;
	}

};

int main(int argc, char* argv[]) {

	kLogIns.add_sink<klog::ConsoleSink>(); 
	dlog("start kcp server"); 
	KcpListener<MyConnection> kcpLis;
	kcpLis.start(8700, [](MyConnection::TPtr, knet::NetEvent evt, const std::string & dv) {
		wlog("received connection event {} , {}", evt,event_string(evt));
		return nullptr;
	});

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
