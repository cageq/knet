
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <string>
#include <memory>
#include <asio.hpp>
#include "utils/knet_log.hpp"
#include "utils/timer.hpp"
#include "knet_worker.hpp"


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

			UdpConnection() { m.status = CONN_IDLE; }

			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<bool (knet::NetEvent)>;		
			using DataHandler = std::function<bool(const std::string & )>;	


			void init(UdpSocketPtr socket = nullptr, KNetWorkerPtr worker = nullptr, KNetHandler<T>* evtHandler = nullptr)
			{
				udp_socket = socket;
				static uint64_t index = 1024;				
				cid = ++index;
				m.event_worker = worker;
				m.user_event_handler = evtHandler;
				handle_event(EVT_CREATE);
			}


			int32_t send(const char* data, std::size_t length) {
				if (udp_socket) { 

					auto buffer = std::make_shared<std::string>(data, length);
					udp_socket->async_send_to(asio::buffer(*buffer), remote_point, [ buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
							if (!ec) {
								// if (event_handler) {
								// 	event_handler(this->shared_from_this(), EVT_SEND, { nullptr, len });
								// }
								//dlog("sent out thread id is {}", std::this_thread::get_id());
							}
							else {
								elog("sent message error : {}, {}", ec.value(), ec.message());
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
					//dlog("join multi address {}", multiHost);
					asio::ip::address multiAddr = asio::ip::make_address(multiHost);
					udp_socket->set_option(asio::ip::multicast::join_group(multiAddr));
				}
				return true;
			}

			int32_t send(const std::string& msg) {
				
				if (udp_socket) {
					auto buffer = std::make_shared<std::string>(std::move(msg));
					udp_socket->async_send_to(asio::buffer(*buffer), remote_point,
						[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
							if (!ec) {
								// if (event_handler) {
								// 	event_handler(this->shared_from_this(), EVT_SEND, { nullptr, len });
								// }
							}
							else {
								elog("sent message error : {}, {}", ec.value(), ec.message());
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

			
			inline void bind_data_handler(DataHandler handler) { data_handler = handler; }
		
			inline void bind_event_handler(EventHandler handler) { event_handler = handler; }
			
			virtual PackageType handle_package(const std::string& msg) {		
				return PACKAGE_USER;
			}

			bool ping() { return true; }
			bool pong() { return true; }
			uint32_t cid = 0;
			void close() {
				if (udp_socket ) {
					if (udp_socket->is_open()){
						udp_socket->close();
					}
					udp_socket.reset(); 
				}  
			}

			virtual bool handle_event(NetEvent evt)
			{
				//dlog("handle event in connection {}", evt);
				return true;
			}


			virtual bool handle_data(const std::string& msg)
			{
				return true;
			}


		private:


			bool process_data(const std::string &msg ){
				bool ret = true; 
				if (data_handler)
				{
					ret=  data_handler(msg);
				}

				if (ret && m.user_event_handler)
				{
					ret = m.user_event_handler->handle_data(this->shared_from_this(), msg );
				}		
				if (ret) {
					ret = handle_data(msg);
				}
		 		return ret; 				
			}

			bool process_event(NetEvent evt){
				bool ret = true; 
				if (event_handler)
				{
					ret =  event_handler(evt);
				}

				if (ret && m.user_event_handler)
				{
					 ret = m.user_event_handler->handle_event(this->shared_from_this(), evt);
				}	

				if (ret){
					return handle_event(evt); 				
				}	
				return ret; 				
			}


			template <class, class, class>
			friend class UdpListener;
			template <class, class, class>
			friend class UdpConnector;

			void connect(udp::endpoint pt, uint32_t localPort = 0,
				const std::string& localAddr = "0.0.0.0") {
				remote_point = pt;
				// this->udp_socket = std::make_shared<udp::socket>(ctx, udp::endpoint(udp::v4(), localPort));
				this->udp_socket = std::make_shared<udp::socket>(m.event_worker->context());
				if (udp_socket) {
					asio::ip::udp::endpoint lisPoint(asio::ip::make_address(localAddr), localPort);
					udp_socket->open(lisPoint.protocol());
					udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
					if (localPort > 0) {
						udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
						udp_socket->bind(lisPoint);
					}

					m.timer = std::unique_ptr<knet::utils::Timer>(new knet::utils::Timer(m.event_worker->context()));
					m.timer->start_timer(
						[this]() {
							std::chrono::steady_clock::time_point nowPoint =
								std::chrono::steady_clock::now();
							auto elapseTime = std::chrono::duration_cast<std::chrono::duration<double>>(
								nowPoint - last_msg_time);

							if (elapseTime.count() > 3) {
							 
								this->release(); 
							}			
							return true; 		
						},
						4000000);
					m.status = CONN_OPEN;

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
							//dlog("get message from {}:{}", sender_point.address().to_string(),
							//	sender_point.port());
							recv_buffer[bytes_recvd] = 0;
							auto pkgType = this->handle_package(std::string((const char*)recv_buffer, bytes_recvd));
							if (pkgType == PACKAGE_USER){
								process_data(std::string((const char*)recv_buffer, bytes_recvd)); 
							}						
					
							do_receive();
						}
						else {
							elog("async receive error {}, {}", ec.value(), ec.message());
						}
					});
			}
			void release(){
				process_event(EVT_RELEASE); 
			} 

			// std::string cid() const { return remote_host + std::to_string(remote_port); }
			enum { max_length = 4096 };
			char recv_buffer[max_length];
			
			udp::endpoint sender_point;
			udp::endpoint remote_point;
			udp::endpoint multicast_point;
			EventHandler event_handler = nullptr;
			DataHandler data_handler = nullptr;

			
		private:
			UdpSocketPtr udp_socket;
			std::chrono::steady_clock::time_point last_msg_time;

			struct {
				KNetHandler<T>* user_event_handler = nullptr;
				KNetWorkerPtr event_worker = nullptr;
				ConnectionStatus status;
				std::unique_ptr<knet::utils::Timer> timer = nullptr;
			} m;
		};

	} // namespace udp
} // namespace knet
