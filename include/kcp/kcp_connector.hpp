
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************
#pragma once
#include <unordered_map>
#include "kcp/kcp_connection.hpp"


using asio::ip::udp;

namespace knet {
namespace kcp {
template <typename T, typename Worker = EventWorker>
class KcpConnector {

public:
	using TPtr = std::shared_ptr<T>;
	using EventHandler = std::function<TPtr(TPtr, NetEvent, std::string_view)>;
	using WorkerPtr = std::shared_ptr<Worker>; 

	KcpConnector(WorkerPtr w = std::make_shared<Worker>() ):worker(w)  {}
	bool start(EventHandler evtHandler = nullptr) {
		event_handler = evtHandler;
		return true;
	}

	TPtr connect(const std::string& host, uint32_t port , uint64_t id =0) {

		TPtr conn = nullptr;
		if (event_handler) {
			conn = event_handler(nullptr, EVT_CREATE, {});
		}

		if (!conn) {
			conn = T::create(worker->context());
		}
		conn->cid = id;  
		connections[id] = conn;

		conn->event_handler = event_handler;
		udp::resolver resolver(worker->context());
		udp::resolver::results_type endpoints =
			resolver.resolve(udp::v4(), host, std::to_string(port));

		if (!endpoints.empty()) {
			conn->connect(worker->context(), *endpoints.begin());
		}
		return conn;
	}

	bool remove(TPtr conn){ 
		auto itr = connections.find(conn->cid); 
		if (itr != connections.end()){
			connections.erase(itr);  
			return true; 
		} 
		return false;
	}

	void stop() {  }

	void wait() {
	}

private:
	WorkerPtr worker; 
	std::unordered_map<uint64_t , TPtr>  connections; 
	EventHandler event_handler = nullptr;
};

} // namespace udp
} // namespace knet
