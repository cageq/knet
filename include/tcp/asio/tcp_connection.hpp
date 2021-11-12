//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <vector>
#include <set>
#include <unordered_map>

#include "tcp_socket.hpp"
#include "knet_worker.hpp"
#include "knet_handler.hpp"
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
			bool reuse = true;  
		}; 


		template <class T>
		class TcpConnection : public std::enable_shared_from_this<T>
		{
		public:
			friend class TcpSocket<T> ; 
			using EventHandler = std::function<bool( NetEvent)>;
			using SelfEventHandler = bool (T::*)( NetEvent);

			using PackageHandler = std::function<int32_t(const char *  &, uint32_t len  )>;
			using SelfPackageHandler = int32_t (T::*)(const char *  &, uint32_t len  );

			using DataHandler = std::function<bool(const std::string & )>;
			using SelfDataHandler = bool (T::*)(const std::string & );

			using SocketPtr = std::shared_ptr<TcpSocket<T> >; 

			// for passive connection 
			template <class ... Args>
			TcpConnection(Args ... args){ 
				static uint64_t index = 1024;
				cid = ++index; 
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

			void init( SocketPtr sock = nullptr,const  KNetWorkerPtr  & worker = nullptr, KNetHandler<T> * evtHandler = nullptr)
			{
				event_worker = worker;			 
				tcp_socket = sock;
				tcp_socket->init(this->shared_from_this()); 
				user_event_handler = evtHandler; 
				handle_event(EVT_CREATE);
			}

			inline int32_t send(const char *pData, uint32_t dataLen) { 	return tcp_socket->send(pData, dataLen); }
			inline int32_t send(const std::string &msg) { return msend(msg); }
			inline int32_t send(const std::string_view &msg) { return msend(msg); }

			template <class P, class... Args>
			int32_t msend(const P &first, const Args &... rest)
			{
				if (tcp_socket)	{
					return tcp_socket->msend(first, rest...);
				}
				return -1;  
			}

			void close()
			{
				reconn_flag = false;
				if (tcp_socket)	
				{
					tcp_socket->close();
				}
			}

			bool post(const std::function<void()> & handler)
			{
				if (tcp_socket)
				{
					tcp_socket->run_inloop(handler);
					return true; 
				}
				else
				{
					elog("socket is invalid");
				}
				return false; 
			}

			
			inline void bind_package_handler(const PackageHandler & handler) { package_handler = handler; }
			void bind_package_handler(const SelfPackageHandler & handler)
			{
				T *child = static_cast<T *>(this);
				package_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
			}


			inline void bind_data_handler(const DataHandler & handler) { data_handler = handler; }
			void bind_data_handler(const SelfDataHandler & handler)
			{
				T *child = static_cast<T *>(this);
				data_handler = std::bind(handler, child, std::placeholders::_1);
			}

			inline void bind_event_handler(const EventHandler & handler) { event_handler = handler; }
			void bind_event_handler(const SelfEventHandler & handler)
			{
				T *child = static_cast<T *>(this);
				event_handler = std::bind(handler, child, std::placeholders::_1);
			}

			inline bool is_connected() { return tcp_socket && tcp_socket->is_open(); }
			inline bool is_connecting() { return tcp_socket && tcp_socket->is_connecting(); }

			bool connect(const ConnectionInfo & connInfo )
			{
				dlog("start to connect {}:{}", connInfo.server_addr, connInfo.server_port);
				this->remote_host = connInfo.server_addr;
				this->remote_port = connInfo.server_port;
				passive_mode = false; 
				return tcp_socket->connect(connInfo.server_addr, connInfo.server_port, connInfo.local_addr, connInfo.local_port);
			}

			bool vsend(const std::vector<asio::const_buffer> &bufs) { return tcp_socket->vsend(bufs); }

			bool connect() { return tcp_socket->connect(remote_host, remote_port); }

			inline tcp::endpoint local_endpoint() const{ return tcp_socket->local_endpoint(); }
			inline tcp::endpoint remote_endpoint() const { return tcp_socket->remote_endpoint(); }
  
			inline std::string get_remote_ip() const{ 
				if (tcp_socket){
					return tcp_socket->remote_endpoint().host; 
				}
				return "";
			}

			inline uint16_t get_remote_port() const{ 
				if (tcp_socket){
					return tcp_socket->remote_endpoint().port; 
				}
				return 0; 				
			}

			inline uint64_t get_cid() const { return cid; }

			inline bool is_passive() const  { return passive_mode; }

			void enable_reconnect(uint32_t interval = 1000000)
			{
				if (!reconn_flag)
				{
					reconn_flag = true; 
					auto self = this->shared_from_this();
					this->reconn_timer = this->start_timer(
						[self]() {
							if (self->tcp_socket ){
								if ( !( self->tcp_socket->is_open() || self->tcp_socket->is_connecting() ))
								{
									self->connect();						 
								}								
							}
							return true; 
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

			inline KNetWorkerPtr get_worker() { return event_worker; }
 
			void *user_data = nullptr;  
 
			virtual int32_t handle_package(const char * data, uint32_t len ){
				return len  ; 
			}
 
			virtual bool handle_event(NetEvent evt) 
			{ 
				return true; 
			}

	
			virtual bool handle_data(const std::string &msg )
			{
				return true; 
			}

			uint64_t start_timer(const knet::utils::Timer::TimerHandler &  handler, uint64_t interval, bool bLoop = true)
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
	
			inline void release(){
				process_event(EVT_RELEASE); 
			} 

	
			inline bool need_reconnect() const {
				return reconn_flag && !passive_mode; 
			}
		 
		 
		 	int32_t process_package(const char * data , uint32_t len){
				 if (package_handler){
					 return package_handler(data , len); 
				 } 			 	 
				 return handle_package(data, len); 
			}

			bool process_data(const std::string &msg ){
				bool ret = true; 
				if (data_handler)
				{
					ret=  data_handler(msg);
				}

				if (ret && user_event_handler)
				{
					ret = user_event_handler->handle_data(this->shared_from_this(), msg );
				}		
				if (ret) {
					ret = handle_data(msg);
				}
		 		return ret; 				
			}
	private:
			bool process_event(NetEvent evt){

				//dlog("process event {}", evt);
				bool ret = true; 
				if (event_handler)
				{
					ret =  event_handler(evt);
				}

				if (ret && user_event_handler)
				{
					 ret = user_event_handler->handle_event(this->shared_from_this(), evt);
				}	

				if (ret){
					return handle_event(evt); 				
				}	
				return ret; 				
			}

			bool reconn_flag = false;
			uint64_t cid = 0;
			uint64_t reconn_timer = 0;
			bool passive_mode = true;
			std::set<uint64_t> conn_timers; 
			SocketPtr tcp_socket = nullptr;

			EventHandler   event_handler   = nullptr;
			DataHandler    data_handler    = nullptr;
			PackageHandler package_handler = nullptr; 

			std::string remote_host ;
			uint16_t remote_port; 
			KNetWorkerPtr event_worker = nullptr;
			KNetHandler<T>* user_event_handler = nullptr;
		};

	} // namespace tcp
} // namespace knet
