//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include <vector>
#include <set>
#include <unordered_map>

#include "tcp_socket.hpp"
#include "event_worker.hpp"
 #include "event_handler.hpp"
using asio::ip::tcp;

namespace knet
{
	namespace tcp
	{
		struct ConnectionInfo {
			ConnectionInfo(const std::string & sAddr, uint16_t sPort, const std::string & dAddr = "127.0.0.1", uint16_t dPort =0){
					server_addr = sAddr; 
					server_port = sPort; 
					local_addr = dAddr; 
					local_port = dPort; 
			}
 
			std::string server_addr; 
			uint16_t server_port; 
			std::string local_addr; 
			uint16_t local_port; 
			bool reuse =true;  
		}; 


		template <class T, class Sock = TcpSocket<T>>
		class TcpConnection : public std::enable_shared_from_this<T>
		{
		public:
			template <class, class, class>
			friend class Listener;
			template <class, class, class>
			friend class Connector;

			using ConnSock = Sock; 
			using EventHandler = std::function<void(std::shared_ptr<T>, NetEvent)>;
			using SelfEventHandler = void (T::*)(std::shared_ptr<T>, NetEvent);
			using DataHandler = std::function<int32_t(const std::string &, MessageStatus)>;
			using SelfDataHandler = int32_t (T::*)(const std::string &, MessageStatus);
			using SocketPtr = std::shared_ptr<Sock>;
 

			// for passive connection 
			template <class ... Args>
			TcpConnection(Args ... args){ 
			}
		

			TcpConnection(const std::string &host, uint16_t port)
			{
				remote_host = host;
				remote_port = port;
			}

			virtual ~TcpConnection()
			{
				//clear all conn_timers to keep safety
				if (event_worker)
				{
					for (auto &tid : conn_timers)
					{
						event_worker->stop_timer(tid);
					}
					conn_timers.clear();
				}
			}

			void init(  SocketPtr sock = nullptr, EventWorkerPtr worker = nullptr, NetEventHandler<T> * evtHandler = nullptr)
			{
				static uint64_t index = 1024;
				event_worker = worker;
			 
				cid = ++index; 
				tcp_socket = sock;
				tcp_socket->connection = this->shared_from_this();
				event_handler = evtHandler; 
				handle_event(EVT_CREATE);
			}

			int send(const char *pData, uint32_t dataLen) { 	return tcp_socket->send(pData, dataLen); }

			int send(const std::string &msg) { return msend(msg); }

			template <class P, class... Args>
			int msend(const P &first, const Args &... rest)
			{
				return tcp_socket->msend(first, rest...);
			}

			void close()
			{
				reconn_flag = false;
				if (tcp_socket)	
				{
					tcp_socket->close();
				}
			}
			void post(std::function<void()> handler)
			{
				if (tcp_socket)
				{
					tcp_socket->run_inloop(handler);
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

			inline void bind_event_handler(EventHandler handler) { event_handler = handler; }
			void bind_event_handler(SelfEventHandler handler)
			{
				T *child = static_cast<T *>(this);
				event_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
			}

			inline bool is_connected() { return tcp_socket && tcp_socket->is_open(); }

			bool connect(const ConnectionInfo & connInfo )
			{
				dlog("start to connect {}:{}", connInfo.server_addr, connInfo.server_port);
				this->remote_host = connInfo.server_addr;
				this->remote_port = connInfo.server_port;
				is_passive = false; 
				return tcp_socket->connect(connInfo.server_addr, connInfo.server_port, connInfo.local_addr, connInfo.local_port);
			}

			bool vsend(const std::vector<asio::const_buffer> &bufs) { return tcp_socket->vsend(bufs); }

			bool connect() { return tcp_socket->connect(remote_host, remote_port); }

			tcp::endpoint local_endpoint() { return tcp_socket->local_endpoint(); }

			tcp::endpoint remote_endpoint() { return tcp_socket->remote_endpoint(); }

			std::string get_remote_ip() { return tcp_socket->remote_endpoint().host; }

			uint16_t get_remote_port() { return tcp_socket->remote_endpoint().port; }

			inline uint64_t get_cid() const { return cid; }

			inline bool passive() const  { return is_passive; }

			void enable_reconnect(uint32_t interval = 1000000)
			{
				if (!reconn_flag)
				{
					reconn_flag = true;
					dlog("start reconnect timer ");
					auto self = this->shared_from_this();
					this->reconn_timer = this->start_timer(
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
				if (this->reconn_timer > 0)
				{
					this->stop_timer(this->reconn_timer);
					this->reconn_timer = 0;
				}
			}

		 

			inline EventWorkerPtr get_worker() { return event_worker; }

			asio::io_context *get_context()
			{
				if (tcp_socket)
				{
					return &tcp_socket->context();
				}
				return nullptr;
			}
			void *user_data = nullptr;
		

			void destroy()
			{
				if (destroyer)
				{
					destroyer(this->shared_from_this());
				}
			}

#if WITH_PACKAGE_HANDLER
			virtual uint32_t handle_package(const char * data, uint32_t len ){
				return len  ; 
			}
#endif 

			std::function<void(std::shared_ptr<T>)> destroyer;
			virtual uint32_t handle_data(const std::string &msg, MessageStatus status)
			{
				if (data_handler)
				{
					return data_handler(msg, status);
				}
		 
				return msg.length();
			}

			uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
			{
				if (event_worker)
				{
					uint64_t tid = event_worker->start_timer(handler, interval, bLoop);
					conn_timers.insert(tid);
					return tid;
				}
				return 0;
			}

			void stop_timer(uint64_t timerId)
			{
				if (event_worker)
				{
					event_worker->stop_timer(timerId);
					conn_timers.erase(timerId);
				}
			}

			virtual void handle_event(NetEvent evt) 
			{
				dlog("handle event in connection {}", evt);
				if (event_handler)
				{
					event_handler->handle_event(this->shared_from_this(), evt);
				}

		 
			}
 
 

			void set_remote_addr(const std::string &host, uint32_t port)
			{
				remote_host = host;
				remote_port = port;
			}

			inline std::string get_remote_host() const
			{
				return remote_host;
			}
			inline uint16_t get_remote_port() const
			{
				return remote_port;
			} 
			bool reconn_flag = false;
		private:
			uint64_t cid = 0;
			uint64_t reconn_timer = 0;
			bool is_passive = true;

			std::set<uint64_t> conn_timers;
 
			SocketPtr tcp_socket = nullptr;
			EventHandler event_handler;
			DataHandler data_handler;
			std::string remote_host;
			uint16_t remote_port;
			EventWorkerPtr event_worker;

			NetEventHandler<T>* event_handler = nullptr;
		};

	} // namespace tcp
} // namespace knet
