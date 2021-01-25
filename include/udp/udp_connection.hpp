
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include <string>
#include <memory>
#include <asio.hpp>
#include "klog.hpp"
 
#include "utils/timer.hpp"
#include "event_worker.hpp"


using namespace knet::utils;
using namespace std::chrono;


namespace knet {
namespace udp {
using asio::ip::udp;

using UdpSocketPtr = std::shared_ptr<udp::socket>;

inline std::string addrstr(udp::endpoint pt) {
	return pt.address().to_string() + ":" + std::to_string(pt.port());
}

template <typename T>
class UdpConnection : public std::enable_shared_from_this<T> {
public:
	enum ConnectionStatus { CONN_IDLE, CONN_OPEN, CONN_CLOSING, CONN_CLOSED };
	enum PackageType {
		PACKAGE_PING,
		PACKAGE_PONG,
		PACKAGE_USER,
	};

	UdpConnection() { status = CONN_IDLE; }
	

	using TPtr = std::shared_ptr<T>;
	using EventHandler = std::function<TPtr(TPtr, knet::NetEvent, const std::string & )>;
	using Buffers = std::vector<std::string>;

	static TPtr create() { return std::make_shared<T>(); }

	int32_t send(const char* data, std::size_t length) {
		if (udp_socket) {
			dlog("send message {} to {}:{}", length, remote_point.address().to_string(),
				remote_point.port());

			dlog("send thread id is {}", std::this_thread::get_id());

			auto buffer = std::make_shared<std::string>(data, length);
			udp_socket->async_send_to(asio::buffer(*buffer), remote_point,
				[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						if (event_handler) {
							event_handler(this->shared_from_this(), EVT_SEND, {nullptr, len});
						}
						dlog("sent out thread id is {}", std::this_thread::get_id());
					} else {
						dlog("sent message error : {}, {}", ec.value(), ec.message());
					}
				}); 

			// udp_socket->async_send_to(asio::buffer(data, length), remote_point,
			// 	[this](std::error_code ec, std::size_t len /*bytes_sent*/) {
			// 		if (!ec) {
			// 			if (event_handler) {
			// 				event_handler(this->shared_from_this(), EVT_SEND, {nullptr, len});
			// 			}
			// 			dlog("sent out thread id is {}", std::this_thread::get_id());
			// 		} else {
			// 			dlog("sent message error : {}, {}", ec.value(), ec.message());
			// 		}
			// 	});

			return 0;
		}
		else {

			elog("no valid socket"); 
		}
		return -1;
	}

	bool join_group(const std::string& multiHost) {

		if (!multiHost.empty()) {
			// Create the socket so that multiple may be bound to the same address.
			dlog("join multi address {}", multiHost);
			asio::ip::address multiAddr = asio::ip::make_address(multiHost);
			udp_socket->set_option(asio::ip::multicast::join_group(multiAddr));
		}
		return true;
	}

	int32_t send(const std::string& msg) {
		dlog("send message to remote {}:{}", remote_point.address().to_string(),
			remote_point.port());
		if (udp_socket) {
			auto buffer = std::make_shared<std::string>(std::move(msg));
			udp_socket->async_send_to(asio::buffer(*buffer), remote_point,
				[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						if (event_handler) {
							event_handler(this->shared_from_this(), EVT_SEND, {nullptr, len});
						}
					} else {
						dlog("sent message error : {}, {}", ec.value(), ec.message());
					}
				});
		}

		return 0;
	}

	int32_t sync_send(const std::string& msg) {
		if (udp_socket) {
			asio::error_code ec;
			auto ret = udp_socket->send_to(asio::buffer(msg), remote_point, 0, ec);
			if (!ec) {
				return ret;
			}
		}
		return -1;
	}

	virtual PackageType on_package(const std::string& msg) {
		 
		wlog("{}", msg.data()); 
		return PACKAGE_USER;
	}

	bool ping() { return true; }
	bool pong() { return true; }
	uint32_t cid = 0;
	void close() {
		if (udp_socket && udp_socket->is_open()) {
			udp_socket->close();
		}
	}

private:
	template <class, class, class>
	friend class UdpListener;
	template <class, class, class>
	friend class UdpConnector;

	void connect(asio::io_context& ctx, udp::endpoint pt, uint32_t localPort = 0,
		const std::string& localAddr = "0.0.0.0") {
		remote_point = pt;
		// this->udp_socket = std::make_shared<udp::socket>(ctx, udp::endpoint(udp::v4(), localPort));
		this->udp_socket = std::make_shared<udp::socket>(ctx);
		if (udp_socket) {
			asio::ip::udp::endpoint lisPoint(asio::ip::make_address(localAddr), localPort);
			udp_socket->open(lisPoint.protocol());
			udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
			if (localPort > 0) {
				udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
				udp_socket->bind(lisPoint);
			}

			m.timer = std::unique_ptr<Timer>(new Timer(ctx));
			m.timer->start_timer(
				[this]() {
					std::chrono::steady_clock::time_point nowPoint =
						std::chrono::steady_clock::now();
					auto elapseTime = std::chrono::duration_cast<std::chrono::duration<double>>(
						nowPoint - last_msg_time);

					if (elapseTime.count() > 3) {
						if (event_handler) {
							event_handler(this->shared_from_this(), EVT_DISCONNECT, {});
						}
					}
					ilog("check heartbeat timer ");
				},
				4000000);
			status = CONN_OPEN;

			if (remote_point.address().is_multicast()) {
				join_group(remote_point.address().to_string());
			}

			do_receive();
		}
	}

	void do_receive() {
		udp_socket->async_receive_from(asio::buffer(recv_buffer, max_length), sender_point,
			[this](std::error_code ec, std::size_t bytes_recvd) {
				if (!ec && bytes_recvd > 0) {

					last_msg_time = std::chrono::steady_clock::now();
					dlog("get message from {}:{}", sender_point.address().to_string(),
						sender_point.port());
					recv_buffer[bytes_recvd] = 0;
					this->on_package(std::string((const char*)recv_buffer, bytes_recvd));
					if (event_handler) {
						event_handler(
							this->shared_from_this(), EVT_RECV, {recv_buffer, bytes_recvd});
					}
					do_receive();
				} else {
					elog("async receive error {}, {}", ec.value(), ec.message());
				}
			});
	}

	
	// std::string cid() const { return remote_host + std::to_string(remote_port); }
	enum { max_length = 4096 };
	char recv_buffer[max_length];
	ConnectionStatus status;
	udp::endpoint sender_point;
	udp::endpoint remote_point;
	udp::endpoint multicast_point;
	EventHandler event_handler = nullptr;

private: 
	UdpSocketPtr udp_socket;
	std::chrono::steady_clock::time_point last_msg_time;

	struct {
		std::unique_ptr<Timer> timer = nullptr;
	} m ; 
};

} // namespace udp
} // namespace knet
