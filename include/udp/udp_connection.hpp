
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

using namespace knet::utils;
using namespace std::chrono;

namespace knet
{
	namespace udp
	{
		using asio::ip::udp;

		using UdpSocketPtr = std::shared_ptr<udp::socket>;
		using SendBufferPtr = std::shared_ptr<std::string>;

	
		inline std::string addrstr(udp::endpoint pt)
		{
			return pt.address().to_string() + ":" + std::to_string(pt.port());
		}

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

			enum PackageType
			{
				PACKAGE_PING,
				PACKAGE_PONG,
				PACKAGE_USER,
			};

			UdpConnection() { net_status = CONN_IDLE; }

			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<bool(knet::NetEvent)>;
			using DataHandler = std::function<bool(const std::string &)>;



			void init(UdpSocketPtr socket = nullptr, KNetWorkerPtr worker = nullptr, KNetHandler<T> *evtHandler = nullptr)
			{
				udp_socket = socket;
				static uint64_t index = 1024;
				cid = ++index;
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
					//dlog("join multi address {}", multiHost);
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

			template <typename P>
			inline void write_data(SendBufferPtr &sndBuf, const P &data)
			{
				sndBuf->append(std::string_view((const char *)&data, sizeof(P)));
			}

			inline void write_data(SendBufferPtr &sndBuf, const std::string_view &data)
			{
				sndBuf->append(data);
			}

			inline void write_data(SendBufferPtr &sndBuf, const std::string &data)
			{
				sndBuf->append(data);
			}

			inline void write_data(SendBufferPtr &sndBuf, const char *data)
			{
				sndBuf->append(std::string(data));
			}

			template <class P, class... Args>
			int32_t msend(const P &first, const Args &...rest)
			{
				auto sendBuffer = std::make_shared<std::string>(); //calc all params size
				return mpush(sendBuffer, first, rest...);
			}

			template <typename F, typename... Args>
			int32_t mpush(SendBufferPtr &sndBuf, const F &data, Args... rest)
			{
				this->write_data(sndBuf, data);
				return mpush(sndBuf, rest...);
			}

			int32_t mpush(SendBufferPtr &sndBuf)
			{

				if (!udp_socket)
				{
					return -1;
				}

				if (!sndBuf->empty())
				{
					udp_socket->async_send_to(asio::const_buffer(sndBuf->data(), sndBuf->length()), 
											  remote_point, [this, sndBuf](std::error_code ec, std::size_t len /*bytes_sent*/)
											  {
												  if (!ec)
												  {
													  //dlog("sent out thread id is {}", std::this_thread::get_id());
												  }
												  else
												  {
													  elog("sent message error : {}, {}", ec.value(), ec.message());
												  }
											  });
					return sndBuf->length();
				}
				return 0;
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
				//dlog("handle event in connection {}", evt);
				return true;
			}

			virtual bool handle_data(const std::string &msg)
			{
				return true;
			}

			uint32_t cid = 0;
		private:
			bool process_data(const std::string &msg)
			{
				bool ret = true;

				if (ret && user_event_handler)
				{
					ret = user_event_handler->handle_data(this->shared_from_this(), msg);
				}
				if (ret)
				{
					ret = handle_data(msg);
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
					asio::ip::udp::endpoint lisPoint(asio::ip::make_address(localAddr), localPort);
					udp_socket->open(lisPoint.protocol());
					udp_socket->set_option(asio::ip::udp::socket::reuse_address(true));
					if (localPort > 0)
					{
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
					net_status = CONN_OPEN;

					if (remote_point.address().is_multicast())
					{
						join_group(remote_point.address().to_string());
					}

					do_receive();
				}
			}

			void do_receive()
			{
				udp_socket->async_receive_from(asio::buffer(recv_buffer, max_length), sender_point,
											   [this](std::error_code ec, std::size_t bytes_recvd)
											   {
												   if (!ec && bytes_recvd > 0)
												   {
													   last_msg_time = std::chrono::steady_clock::now();
													   //dlog("get message from {}:{}", sender_point.address().to_string(),
													   //	sender_point.port());
													   recv_buffer[bytes_recvd] = 0;
													   auto pkgType = this->handle_package(std::string((const char *)recv_buffer, bytes_recvd));
													   if (pkgType == PACKAGE_USER)
													   {
														   process_data(std::string((const char *)recv_buffer, bytes_recvd));
													   }

													   do_receive();
												   }
												   else
												   {
													   elog("async receive error {}, {}", ec.value(), ec.message());
												   }
											   });
			}
			void release()
			{
				process_event(EVT_RELEASE);
			}

			// std::string cid() const { return remote_host + std::to_string(remote_port); }
			enum
			{
				max_length = 4096
			};
			char recv_buffer[max_length];

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
			
		};

	} // namespace udp
} // namespace knet
