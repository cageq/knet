#include "knet.hpp"

#include "kcp/kcp_connector.hpp"
#include <chrono>

using namespace knet::kcp;

class MyConnection : public KcpConnection<MyConnection> {

public:
	MyConnection()
		: KcpConnection<MyConnection>() {
			cid = 8888; 	
		}
	virtual ~MyConnection(){}
	virtual PackageType handle_package(const char* data, uint32_t len) {
		wlog("on recv udp message\n{} , lenght is {}", data, len);
		return PACKAGE_USER;
	}

};

int main(int argc, char* argv[]) {
 
	KNetLogIns.add_console(); 
	KcpConnector<MyConnection> connector;
	connector.start();

	auto conn = connector.connect("127.0.0.1", 8700, 8888);

	while (1) {

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		conn->send("hello world");
	}

	return 0;
}
