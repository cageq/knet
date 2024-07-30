
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <asio.hpp>
#include "utils/knet_log.hpp"
#include "utils/timer.hpp"
#include "utils/knet_url.hpp"
#include "knet_worker.hpp"
#include "utils/loop_buffer.hpp"

using namespace knet::utils;
using namespace std::chrono;

namespace knet
{
	namespace udp
	{
		using asio::ip::udp;

		using UdpSocketPtr = std::shared_ptr<udp::socket>;
		using SendBufferPtr = std::shared_ptr<std::string>;
		enum { kMaxRecvBufferSize = 4096 };
	
		inline std::string addrstr(udp::endpoint pt)
		{
			return pt.address().to_string() + ":" + std::to_string(pt.port());
		}

		enum PackageType
		{
			PACKAGE_PING,
			PACKAGE_PONG,
			PACKAGE_USER,
		};


		template <typename T>
		class UdpConnection : public std::enable_shared_from_this<T>
		{
		public:
			enum ConnectionStatus
			{
				CONN_IDLE,
				CONN_OPEN,
				CONN_CLOSING,
				CONN_CLOSED
			};

	
			UdpConnection() {
				static uint64_t index = 1024;
		 		cid = ++index;
		   		net_status = CONN_IDLE;
		    }
			
			using EventHandler = std::function<bool(knet::NetEvent)>;
			using DataHandler = std::function<bool(const std::string &)>; 

			void init(UdpSocketPtr socket = nullptr, KNetWorkerPtr worker = nullptr, KNetHandler<T> *evtHandler = nullptr)
			{
				udp_socket = socket;
				event_worker = worker;
				user_event_handler = evtHandler;
				process_event(EVT_CREATE);
			}

			int32_t send(const char *data, std::size_t length)
			{
				return msend(std::string(data, length));
			}

			int32_t send(const std::string &msg)
			{
				return msend(msg);
			}

			bool join_group(const std::string &multiHost)
			{
				if (!multiHost.empty())
				{
					// Create the socket so that multiple may be bound to the same address.
					//knet_dlog("join multi address {}", multiHost);
					asio::ip::address multiAddr = asio::ip::make_address(multiHost);
					udp_socket->set_option(asio::ip::multicast::join_group(multiAddr));
				}
				return true; 
			}

			int32_t sync_send(const std::string &msg)
			{
				if (udp_socket)
				{
					asio::error_code ec;
					auto ret = udp_socket->send_to(asio::buffer(msg), remote_point, 0, ec);
					if (!ec)
					{
						return ret;
					}
				}
				return -1;
			}


			template <class P, class... Args>
			int32_t msend(const P &first, const Args &...rest)
			{
				{
				
					std::lock_guard<std::mutex> lock (write_mutex); 				
					mpush( first, rest...);
				}
				return do_send(); 
			}

			virtual PackageType handle_package(const std::string &msg)
			{
				return PACKAGE_USER;
			}

			bool ping()
			{
				return true;
			}
			bool pong()
			{
				return true;
			}

			void close()
			{
				if (udp_socket)
				{
					if (udp_socket->is_open())
					{
						udp_socket->close();
					}
					udp_socket.reset();
				}
			}

			virtual bool handle_event(NetEvent evt)
			{
				//knet_dlog("handle event in connection {}", evt);
				return true;
			}

			virtual bool handle_data(char * data, uint32_t dataLen)
			{
				return true;
			}

			uint32_t cid = 0;
		private:

		
				template <typename P >  
					inline uint32_t write_data( const P & data  ){
						return send_buffer.push((const char*)&data, sizeof(P));  
					} 


				inline uint32_t  write_data(const std::string_view &  data ){
					return send_buffer.push(data.data(), data.length());  
				}

				inline uint32_t write_data(const std::string &  data ){
					return send_buffer.push(data.data(), data.length()); 

				}

				inline uint32_t write_data(const char* data ){
					if (data != nullptr ){ 
						return send_buffer.push(data , strlen(data));  
					}
					return 0;                         
				}

				template <typename F, typename ... Args>
					int32_t mpush(const F &  data, Args... rest) { 					
						this->write_data(data  ) ;    
						return mpush(rest...);
					} 
 


			int32_t mpush()
			{
				return 0; 
			}
			
			int32_t do_send(){
			 
				if (!udp_socket)
				{
					return -1;
				}

				auto sentLen  = send_buffer.read([this](const char * data,uint32_t dataLen ){
					udp_socket->async_send_to(asio::const_buffer(data, dataLen), 
						remote_point, [this, dataLen](std::error_code ec, std::size_t len /*bytes_sent*/)
						{
							if (!ec)
							{
								send_buffer.commit(dataLen); 
								knet_dlog("send msg to {}  len {}",remote_point.address().to_string(), dataLen); 
							}
							else
							{
								knet_elog("sent message error : {}, {}, {}", ec.value(), ec.message(),dataLen);
							}
						});
						return dataLen; 
				});  
				return sentLen;
			}



			bool process_data(char * data, uint32_t dataLen)
			{
				bool ret = true;

				if (ret && user_event_handler)
				{
					ret = user_event_handler->handle_data(this->shared_from_this(), data, dataLen);
				}
				if (ret)
				{
					ret = handle_data(data, dataLen);
				}
				return ret;
			}

			bool process_event(NetEvent evt)
			{
				bool ret = true;

				if (ret && user_event_handler)
				{
					ret = user_event_handler->handle_event(this->shared_from_this(), evt);
				}

				if (ret)
				{
					return handle_event(evt);
				}
				return ret;
			}

			template <class, class, class>
			friend class UdpListener;
			template <class, class, class>
			friend class UdpConnector;

			void connect(udp::endpoint pt, uint32_t localPort = 0,
						 const std::string &localAddr = "0.0.0.0")
			{
				remote_point = pt;
				// this->udp_socket = std::make_shared<udp::socket>(ctx, udp::endpoint(udp::v4(), localPort));
				this->udp_socket = std::make_shared<udp::socket>(event_worker->context());
				if (udp_socket)
				{
					knet_dlog("local address is {}:{}", localAddr, localPort); 
					udp_socket->open(pt.protocol()); 

					if (remote_point.address().is_multicast())
					{				 
						knet_dlog("bind local multi address {}:{}", localAddr, remote_point.port());
						asio::ip::udp::endpoint bindAddr(asio::ip::make_address(localAddr.empty()?"0.0.0.0":localAddr), remote_point.port());				
				//		udp_socket->open(bindAddr.protocol());
						udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
						udp_socket->bind(bindAddr);  
				 
						join_group(remote_point.address().to_string() );
					}else {
						if (localPort > 0){
							asio::ip::udp::endpoint lisPoint(asio::ip::make_address(localAddr), localPort);
							udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
							udp_socket->bind(lisPoint);
						}
						net_timer = std::unique_ptr<knet::utils::Timer>(new knet::utils::Timer(event_worker->context()));
						net_timer->start_timer(
							[this]()
							{
								std::chrono::steady_clock::time_point nowPoint =
									std::chrono::steady_clock::now();
								auto elapseTime = std::chrono::duration_cast<std::chrono::duration<double>>(
									nowPoint - last_msg_time);

								if (elapseTime.count() > 3)
								{
									this->release();
								}
								return true;
							},
							4000000);

					}

					net_status = CONN_OPEN;
					do_receive();
				}
			}

			void do_receive()
			{
				udp_socket->async_receive_from(asio::buffer(recv_buffer, kMaxRecvBufferSize), sender_point,
						[this](std::error_code ec, std::size_t bytes_recvd)
						{
							if (!ec && bytes_recvd > 0)
							{
								last_msg_time = std::chrono::steady_clock::now();
								//knet_dlog("get message from {}:{}", sender_point.address().to_string(),
								//	sender_point.port());
								this->sender_point = sender_point; 
								recv_buffer[bytes_recvd] = 0;
								auto pkgType = this->handle_package(std::string((const char *)recv_buffer, bytes_recvd));
								if (pkgType == PACKAGE_USER)
								{
									process_data((char *)recv_buffer, bytes_recvd);
								}

								do_receive();
							}
							else
							{
								knet_elog("async receive error {}, {}", ec.value(), ec.message());
							}
						});
			}

 


			void release()
			{
				process_event(EVT_RELEASE);
			}

			// std::string cid() const { return remote_host + std::to_string(remote_port); }
 
			char recv_buffer[kMaxRecvBufferSize];
			udp::endpoint sender_point;
			udp::endpoint remote_point;
			udp::endpoint multicast_point;
		private:
			UdpSocketPtr udp_socket;
			std::chrono::steady_clock::time_point last_msg_time;	 
			KNetHandler<T> *user_event_handler = nullptr;
			KNetWorkerPtr event_worker = nullptr;
			ConnectionStatus net_status;
			std::unique_ptr<knet::utils::Timer> net_timer = nullptr;
			std::mutex write_mutex;  
            LoopBuffer<8192> send_buffer;  

			
		};

	} // namespace udp
} // namespace knet
