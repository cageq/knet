
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include <unordered_map>
#include "udp/udp_connection.hpp"

namespace knet {
namespace udp {

template <typename T, typename Worker = EventWorker>
class UdpListener {
public:
	using TPtr = std::shared_ptr<T>;
	using EventHandler = std::function<TPtr(TPtr, NetEvent, const std::string  & )>;
	using WorkerPtr = std::shared_ptr<Worker>;
	UdpListener(WorkerPtr w = nullptr)
		: worker(w) {
		if (worker == nullptr) {
			worker = std::make_shared<Worker>();
			worker->start();
		}
	}

	bool start(EventHandler evtHandler, uint32_t port, const std::string& lisHost = "0.0.0.0",
		const std::string& multiHost = "", bool reuse = true) {

		m_port = port;
		event_handler = evtHandler;
		asio::ip::address lisAddr = asio::ip::make_address(lisHost);

		server_socket = std::make_shared<udp::socket>(worker->context());

		// server_socket =
		// 	std::make_shared<udp::socket>(worker->context(),  udp::endpoint(lisAddr, port) );

		asio::ip::udp::endpoint lisPoint(lisAddr, port);

		server_socket->open(lisPoint.protocol());
		server_socket->set_option(asio::ip::udp::socket::reuse_address(reuse));
		server_socket->bind(lisPoint);

		multi_host = multiHost;

		if (!multiHost.empty()) {
			// Create the socket so that multiple may be bound to the same address.
			dlog("join multi address {}", multiHost);
			asio::ip::address multiAddr = asio::ip::make_address(multiHost);
			server_socket->set_option(asio::ip::multicast::join_group(multiAddr));
		}

		dlog("start udp server {}:{}", lisHost, port);
		do_receive();
		return true;
	}

	TPtr find_connection(udp::endpoint pt) {
		auto itr = connections.find(addrstr(pt));
		if (itr != connections.end()) {
			return itr->second;
		}
		return nullptr;
	}
	void broadcast(const std::string& msg) {

		if (multi_host.empty()) {
			for (auto& item : connections) {
				auto conn = item.second;
				conn->send(msg);
			}
		} else {
			auto buffer = std::make_shared<std::string>(std::move(msg));
			asio::ip::address multiAddr = asio::ip::make_address(multi_host);

			asio::ip::udp::endpoint multiPoint(multiAddr, m_port);
			dlog("broadcast message to {}:{}", multiPoint.address().to_string(), multiPoint.port());
			server_socket->async_send_to(asio::buffer(*buffer), multiPoint,
				[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						if (event_handler) {
							event_handler(nullptr, EVT_SEND, {nullptr, len});
						}
					} else {
						dlog("sent message error : {}, {}", ec.value(), ec.message());
					}
				});
		}
	}

	TPtr create_connection(udp::endpoint pt) {
		TPtr conn = nullptr;
		if (event_handler) {
			conn = event_handler(nullptr, EVT_CREATE, {});
		}

		if (!conn) {
			conn = T::create();
		}
		conn->status = T::CONN_OPEN;
		conn->event_handler = event_handler;
		dlog("add remote connection {}", addrstr(pt));
		connections[addrstr(pt)] = conn;
		return conn;
	}

	void stop() {}

private:
	void do_receive() {

		server_socket->async_receive_from(asio::buffer(recv_buffer, max_length), remote_point,
			[this](std::error_code ec, std::size_t bytes_recvd) {
				if (!ec && bytes_recvd > 0) {

					dlog("get message from {}:{}", remote_point.address().to_string(),
						remote_point.port());
					auto conn = this->find_connection(remote_point);
					if (!conn) {
						conn = this->create_connection(remote_point);
						conn->sock = server_socket;

						conn->remote_point = remote_point;
						// if (multi_host.empty()) {
						// 	conn->remote_point = remote_point;
						// } else {
						// 	conn->remote_point =
						// 		asio::ip::udp::endpoint(asio::ip::make_address(multi_host), m_port);
						// }
					}
					recv_buffer[bytes_recvd] = 0;
					conn->on_package(std::string((const char*)recv_buffer, bytes_recvd));

					if (event_handler) {
						event_handler(conn, EVT_RECV, {recv_buffer, bytes_recvd});
					}

				} else {
					elog("receive error");
				}
				do_receive();
			});
	}

	// void run() {
	// 	worker->post([this]() {
	// 		dlog("start udp server @ {}", m_port);
	// 		server_socket =
	// 			std::make_shared<udp::socket>(worker->context(), udp::endpoint(udp::v4(), m_port));
	// 		do_receive();
	// 	});
	// }

	enum { max_length = 4096 };
	char recv_buffer[max_length];

	uint32_t m_port;

	std::string multi_host;

	WorkerPtr worker;
	std::shared_ptr<udp::socket> server_socket;
	std::unordered_map<std::string, TPtr> connections;
	EventHandler event_handler;

	udp::endpoint remote_point;
};

} // namespace udp
} // namespace knet
