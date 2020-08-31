//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include "udp/udp_connection.hpp"

using asio::ip::udp;

namespace knet {
namespace udp {

template <typename T, typename Worker = EventWorker>
class UdpConnector {

public:
	using TPtr = std::shared_ptr<T>;
	using WorkerPtr = std::shared_ptr<Worker>;
	UdpConnector(WorkerPtr w = nullptr)
		: worker(w) {}

	using EventHandler = std::function<TPtr(TPtr, NetEvent, std::string_view)>;
	bool start(EventHandler evtHandler = nullptr) {
		event_handler = evtHandler;
		if (worker == nullptr) {
			worker = std::make_shared<Worker>();
			worker->start();
		}
		return true;
	}

	TPtr connect(const std::string& host, uint32_t port, uint32_t localPort = 0,
		const std::string& localAddr = "0.0.0.0") {

		TPtr conn = nullptr;
		if (event_handler) {
			conn = event_handler(nullptr, EVT_CREATE, {});
		}

		if (!conn) {
			conn = T::create();
		}
		asio::ip::address remoteAddr = asio::ip::make_address(host);
		asio::ip::udp::endpoint remotePoint(remoteAddr, port);

		conn->event_handler = event_handler;
		if (remoteAddr.is_multicast()) {
			conn->connect(worker->context(), remotePoint, localPort, localAddr);
		} else {
			udp::resolver resolver(worker->context());
			udp::resolver::results_type endpoints =
				resolver.resolve(remotePoint.protocol(), host, std::to_string(port));

			if (!endpoints.empty()) {
				conn->connect(worker->context(), *endpoints.begin(), localPort);
			}
		}
		return conn;
	}
	void stop() {}

private:
	WorkerPtr worker;
	EventHandler event_handler = nullptr;
};

} // namespace udp
} // namespace knet
