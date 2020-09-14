//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include <vector>
#include <set>
#include <unordered_map>

#include "tcp_socket.hpp"
#include "tcp_factory.hpp"
#include "event_worker.hpp"

using asio::ip::tcp;

namespace knet
{
	namespace tcp
	{
		struct ConnectionInfo {
			std::string server_addr; 
			uint32_t server_port; 
			std::string local_addr; 
			uint32_t local_port; 
			bool reuse =true; 
		}; 


		template <class T, class Sock = TcpSocket<T>>
		class TcpConnection : public std::enable_shared_from_this<T>
		{
		public:
			template <class, class, class, class...>
			friend class Listener;
			template <class, class, class>
			friend class Connector;

			using ConnSock = Sock; 
			using NetEventHandler = std::function<void(std::shared_ptr<T>, NetEvent)>;
			using SelfNetEventHandler = void (T::*)(std::shared_ptr<T>, NetEvent);
			using DataHandler = std::function<uint32_t(const std::string &, MessageStatus)>;
			using SelfDataHandler = uint32_t (T::*)(const std::string &, MessageStatus);
			using SocketPtr = std::shared_ptr<Sock>;
			using ConnFactory = ConnectionFactory<T>;
			using FactoryPtr = ConnFactory *;

			TcpConnection() {}

			TcpConnection(const std::string &host, uint32_t port)
			{
				remote_host = host;
				remote_port = port;
			}

			virtual ~TcpConnection()
			{
				//clear all timers to keep safety
				if (event_worker)
				{
					for (auto &tid : timers)
					{
						event_worker->stop_timer(tid);
					}
					timers.clear();
				}
			}

			void init(FactoryPtr fac = nullptr, SocketPtr sock = nullptr, EventWorkerPtr worker = nullptr)
			{
				static uint64_t index = 1024;
				event_worker = worker;
				this->factory = fac;
				cid = ++index;
				//std::shared_ptr<T> self = this->shared_from_this();
				socket = sock;
				socket->connection = this->shared_from_this();
				handle_event(EVT_CREATE);
			}

			int send(const char *pData, uint32_t dataLen) { return msend(std::string(pData, dataLen)); }

			int send(const std::string &msg) { return msend(msg); }

			template <class P, class... Args>
			int msend(const P &first, const Args &... rest)
			{
				return socket->msend(first, rest...);
			}

			void close()
			{
				reconn_flag = false;
				if (socket)
				{
					socket->close();
				}
			}
			void post(std::function<void()> handler)
			{
				if (socket)
				{
					socket->run_inloop(handler);
				}
				else
				{
					elog("socket is invalid");
				}
			}

			inline void bind_data_handler(DataHandler handler) { data_handler = handler; }
			void bind_data_handler(SelfDataHandler handler)
			{
				T *child = static_cast<T *>(this);
				data_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
			}

			inline void bind_event_handler(NetEventHandler handler) { event_handler = handler; }
			void bind_event_handler(SelfNetEventHandler handler)
			{
				T *child = static_cast<T *>(this);
				event_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
			}

			inline bool is_connected() { return socket && socket->is_open(); }

			bool connect(const ConnectionInfo & connInfo )
			{
				dlog("start to connect {}:{}", connInfo.server_addr, connInfo.server_port);
				this->remote_host = connInfo.server_addr;
				this->remote_port = connInfo.server_port;
				is_passive = false; 
				return socket->connect(connInfo.server_addr, connInfo.server_port, connInfo.local_addr, connInfo.local_port);
			}

			bool vsend(const std::vector<asio::const_buffer> &bufs) { return socket->vsend(bufs); }

			bool connect() { return socket->connect(remote_host, remote_port); }

			tcp::endpoint local_endpoint() { return socket->local_endpoint(); }

			tcp::endpoint remote_endpoint() { return socket->remote_endpoint(); }

			std::string get_remote_ip() { return socket->remote_endpoint().host; }

			uint32_t get_remote_port() { return socket->remote_endpoint().port; }

			inline uint64_t get_cid() const { return cid; }

			bool passive() { return is_passive; }

			void enable_reconnect(uint32_t interval = 1000000)
			{
				if (!reconn_flag)
				{
					reconn_flag = true;
					dlog("start reconnect timer ");
					auto self = this->shared_from_this();
					this->timerid = this->start_timer(
						[=]() {
							if (!is_connected())
							{
								dlog("try to reconnect to server ");
								self->connect();
							}
						},
						interval);
				}
			}

			void disable_reconnect()
			{
				reconn_flag = false;
				if (this->timerid > 0)
				{
					this->stop_timer(this->timerid);
					this->timerid = 0;
				}
			}

			template <class FPtr>
			FPtr get_factory()
			{
				return std::static_pointer_cast<FPtr>(factory);
			}

			inline EventWorkerPtr get_worker() { return event_worker; }

			asio::io_context *get_context()
			{
				if (socket)
				{
					return &socket->context();
				}
				return nullptr;
			}
			void *user_data = nullptr;
			uint64_t cid = 0;

			void destroy()
			{
				if (destroyer)
				{
					destroyer(this->shared_from_this());
				}
			}

			std::function<void(std::shared_ptr<T>)> destroyer;
			virtual uint32_t handle_data(const std::string &msg, MessageStatus status)
			{
				if (data_handler)
				{
					return data_handler(msg, status);
				}

				if (factory)
				{
					return factory->handle_data(this->shared_from_this(), msg, status);
				}
				return msg.length();
			}

			uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
			{
				if (event_worker)
				{
					uint64_t tid = event_worker->start_timer(handler, interval, bLoop);
					timers.insert(tid);
					return tid;
				}
				return 0;
			}

			void stop_timer(uint64_t timerId)
			{
				if (event_worker)
				{
					event_worker->stop_timer(timerId);
					timers.erase(timerId);
				}
			}

			virtual void handle_event(NetEvent evt)
			{
				dlog("handle event in connection {}", evt);
				if (event_handler)
				{
					event_handler(this->shared_from_this(), evt);
				}

				if (factory)
				{
					factory->handle_event(this->shared_from_this(), evt);
				}
			}

			//friend class ConnectionFactory<T>;

			// static std::shared_ptr<T> create(SocketPtr sock)
			// {
			// 	auto self = std::make_shared<T>();
			// 	self->init(self->factory, sock);
			// 	return self;
			// }

			void set_remote_addr(const std::string &host, uint32_t port)
			{
				remote_host = host;
				remote_port = port;
			}
			inline std::string get_remote_host() const
			{
				return remote_host;
			}
			inline uint32_t get_remote_port() const
			{
				return remote_port;
			}

		
			bool reconn_flag = false;

		private:
			uint64_t timerid = 0;
			bool is_passive = true;

			std::set<uint64_t> timers;
			FactoryPtr factory = nullptr;
			SocketPtr socket = nullptr;
			NetEventHandler event_handler;
			DataHandler data_handler;
			std::string remote_host;
			uint32_t remote_port;
			EventWorkerPtr event_worker;
		};

	} // namespace tcp
} // namespace knet
