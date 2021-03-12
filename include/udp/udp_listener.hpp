
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include <unordered_map>
#include "conn_factory.hpp"
#include "udp/udp_connection.hpp"

namespace knet {
	namespace udp {

		template <typename T, class Factory = ConnFactory<T>, typename Worker = EventWorker>
		class UdpListener : public NetEventHandler<T> {
		public:
			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<TPtr(TPtr, NetEvent, const std::string&)>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*;
			UdpListener(WorkerPtr w = nullptr)
				: worker(w) {
				if (worker == nullptr) {
					worker = std::make_shared<Worker>();
					worker->start();
				}
			}

			bool start(EventHandler evtHandler, uint32_t port, const std::string& lisHost = "0.0.0.0",
				const std::string& multiHost = "", bool reuse = true) {

				m.port = port;
				m.event_handler = evtHandler;
				asio::ip::address lisAddr = asio::ip::make_address(lisHost);

				m.server_socket = std::make_shared<udp::socket>(worker->context());

				// m.server_socket =
				// 	std::make_shared<udp::socket>(worker->context(),  udp::endpoint(lisAddr, port) );

				asio::ip::udp::endpoint lisPoint(lisAddr, port);

				m.server_socket->open(lisPoint.protocol());
				m.server_socket->set_option(asio::ip::udp::socket::reuse_address(reuse));
				m.server_socket->bind(lisPoint);

				m.multi_host = multiHost;

				if (!multiHost.empty()) {
					// Create the socket so that multiple may be bound to the same address.
					dlog("join multi address {}", multiHost);
					asio::ip::address multiAddr = asio::ip::make_address(multiHost);
					m.server_socket->set_option(asio::ip::multicast::join_group(multiAddr));
				}

				dlog("start udp server {}:{}", lisHost, port);
				do_receive();
				return true;
			}

			TPtr find_connection(udp::endpoint pt) {
				auto itr = m.connections.find(addrstr(pt));
				if (itr != m.connections.end()) {
					return itr->second;
				}
				return nullptr;
			}
			void broadcast(const std::string& msg) {

				if (m.multi_host.empty()) {
					for (auto& item : m.connections) {
						auto conn = item.second;
						conn->send(msg);
					}
				}
				else {
					auto buffer = std::make_shared<std::string>(std::move(msg));
					asio::ip::address multiAddr = asio::ip::make_address(m.multi_host);

					asio::ip::udp::endpoint multiPoint(multiAddr, m.port);
					dlog("broadcast message to {}:{}", multiPoint.address().to_string(), multiPoint.port());
					m.server_socket->async_send_to(asio::buffer(*buffer), multiPoint,
						[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
							if (!ec) {
								if (m.event_handler) {
									m.event_handler(nullptr, EVT_SEND, { nullptr, len });
								}
							}
							else {
								dlog("sent message error : {}, {}", ec.value(), ec.message());
							}
						});
				}
			}

			TPtr create_connection(udp::endpoint pt) {
				TPtr conn = nullptr;

				if (m.factory)
				{
					conn = m.factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}
		
 
				dlog("add remote connection {}", addrstr(pt));
				m.connections[addrstr(pt)] = conn;
				return conn;
			}

			void stop() {
				m.server_socket.close();
				m.connections.clear();
			}

		private:

			virtual bool handle_data(std::shared_ptr<T>, const std::string& msg) {

				return true;
			}
			virtual bool handle_event(std::shared_ptr<T>, NetEvent) {
				return true;
			}


			void do_receive() {

				m.server_socket->async_receive_from(asio::buffer(m.recv_buffer, max_length), m.remote_point,
					[this](std::error_code ec, std::size_t bytes_recvd) {
						if (!ec && bytes_recvd > 0) {

							dlog("get message from {}:{}", m.remote_point.address().to_string(),
								m.remote_point.port());
							auto conn = this->find_connection(m.remote_point);
							if (!conn) {
								conn = this->create_connection(m.remote_point);
								conn->init(m.server_socket, worker, this );
								//conn->udp_socket = m.server_socket;
								conn->remote_point = m.remote_point;
								// if (m.multi_host.empty()) {
								// 	conn->remote_point = m.remote_point;
								// } else {
								// 	conn->remote_point =
								// 		asio::ip::udp::endpoint(asio::ip::make_address(m.multi_host), m.port);
								// }
							}
							m.recv_buffer[bytes_recvd] = 0;
							conn->on_package(std::string((const char*)m.recv_buffer, bytes_recvd));

							if (m.event_handler) {
								m.event_handler(conn, EVT_RECV, { m.recv_buffer, bytes_recvd });
							}

						}
						else {
							elog("receive error");
						}
						do_receive();
					});
			}

			// void run() {
			// 	worker->post([this]() {
			// 		dlog("start udp server @ {}", m.port);
			// 		m.server_socket =
			// 			std::make_shared<udp::socket>(worker->context(), udp::endpoint(udp::v4(), m.port));
			// 		do_receive();
			// 	});
			// }

			enum { max_length = 4096 };

			struct {
				char recv_buffer[max_length];
				uint32_t port;
				std::string multi_host;
				udp::endpoint remote_point;
				std::shared_ptr<udp::socket> server_socket;
				std::unordered_map<std::string, TPtr> connections;
				EventHandler event_handler;

				FactoryPtr factory = nullptr;
			} m;

			WorkerPtr worker;

		};

	} // namespace udp
} // namespace knet
