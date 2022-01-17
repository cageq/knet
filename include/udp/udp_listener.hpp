
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include <unordered_map>
#include "knet_factory.hpp"
#include "udp/udp_connection.hpp"

namespace knet {
	namespace udp {

		template <typename T, class Factory = KNetFactory<T>, typename Worker = KNetWorker>
		class UdpListener : public KNetHandler<T> {
		public:
			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<TPtr(TPtr, NetEvent, const std::string&)>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*;
			UdpListener(WorkerPtr w = nullptr)
				: net_worker(w) {
				if (net_worker == nullptr) {
					net_worker = std::make_shared<Worker>();
					net_worker->start();
				}
			}

			bool start(EventHandler evtHandler, uint32_t port, const std::string& lisHost = "0.0.0.0",
				const std::string& multiHost = "", bool reuse = true) {

				udp_port = port;
				event_handler = evtHandler;
				asio::ip::address lisAddr = asio::ip::make_address(lisHost);
				server_socket = std::make_shared<udp::socket>(net_worker->context());
				// server_socket =
				// 	std::make_shared<udp::socket>(net_worker->context(),  udp::endpoint(lisAddr, port) );

				asio::ip::udp::endpoint lisPoint(lisAddr, port);

				server_socket->open(lisPoint.protocol());
				server_socket->set_option(asio::ip::udp::socket::reuse_address(reuse));
				server_socket->bind(lisPoint);
		

				if (!multiHost.empty()) {
					multi_host = multiHost;
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
				}
				else {
					auto buffer = std::make_shared<std::string>(std::move(msg));
					asio::ip::address multiAddr = asio::ip::make_address(multi_host);

					asio::ip::udp::endpoint multiPoint(multiAddr, udp_port);
					dlog("broadcast message to {}:{}", multiPoint.address().to_string(), multiPoint.port());
					server_socket->async_send_to(asio::buffer(*buffer), multiPoint,
						[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
							if (!ec) {
								if (event_handler) {
									event_handler(nullptr, EVT_SEND, { nullptr, len });
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

				if (net_factory)
				{
					conn = net_factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}
		 
				dlog("add remote connection {}", addrstr(pt));
				connections[addrstr(pt)] = conn;
				return conn;
			}

			void stop() {
				if (server_socket){
					server_socket->close();
				}
				
				connections.clear();
			}

		private:

			virtual bool handle_data(std::shared_ptr<T>, const std::string& msg) {

				return true;
			}
			virtual bool handle_event(std::shared_ptr<T>, NetEvent) {
				return true;
			}


			void do_receive() {

				server_socket->async_receive_from(asio::buffer(recv_buffer, max_length), remote_point,
					[this](std::error_code ec, std::size_t bytes_recvd) {
						if (!ec && bytes_recvd > 0) {

							dlog("get message from {}:{}", remote_point.address().to_string(),
								remote_point.port());
							auto conn = this->find_connection(remote_point);
							if (!conn) {
								conn = this->create_connection(remote_point);
								conn->init(server_socket, net_worker, this );
								//conn->udp_socket = server_socket;
								conn->remote_point = remote_point;
								// if (multi_host.empty()) {
								// 	conn->remote_point = remote_point;
								// } else {
								// 	conn->remote_point =
								// 		asio::ip::udp::endpoint(asio::ip::make_address(multi_host), udp_port);
								// }
							}
							recv_buffer[bytes_recvd] = 0;
							conn->handle_package(std::string((const char*)recv_buffer, bytes_recvd));

							if (event_handler) {
								event_handler(conn, EVT_RECV, { recv_buffer, bytes_recvd });
							}

						}
						else {
							elog("receive error");
						}
						do_receive();
					});
			}



			enum { max_length = 4096 };
			char recv_buffer[max_length];
			uint32_t udp_port;
			std::string multi_host;
			udp::endpoint remote_point;
			std::shared_ptr<udp::socket> server_socket;
			std::unordered_map<std::string, TPtr> connections;
			EventHandler event_handler;
			FactoryPtr net_factory = nullptr;
			WorkerPtr net_worker;

		};

	} // namespace udp
} // namespace knet
