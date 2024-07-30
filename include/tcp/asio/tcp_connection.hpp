//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <vector>
#include <list>
#include <unordered_map>
#include <cstdlib>
#include "tcp_socket.hpp"
#ifdef KNET_WITH_OPENSSL
#include "tls_socket.hpp"
#endif 
#include <thread> 
#include "knet_worker.hpp"
#include "knet_handler.hpp"
#include "utils/knet_log.hpp"
using asio::ip::tcp;
using namespace  knet::log; 

namespace knet
{
	namespace tcp
	{ 
		template <class T , class Sock = TcpSocket<T>>
		class TcpConnection : public std::enable_shared_from_this<T>
		{
		public:
			using ConnSock  = Sock; 
		    friend Sock; 

			using KNetEventHandler = std::function<bool( NetEvent)>;
			using SelfEventHandler = bool (T::*)( NetEvent);

			using PackageHandler = std::function<int32_t(const char * , uint32_t len  )>;
			using SelfPackageHandler = int32_t (T::*)(const char * , uint32_t len  );

			using KNetDataHandler = std::function<bool(char *, uint32_t)>;
			using SelfDataHandler = bool (T::*)(char * , uint32_t );

			using SocketPtr = std::shared_ptr<Sock >; 

			// for passive connection 
			template <class ... Args>
			TcpConnection(Args ... args){ 
				static  std::atomic_uint64_t  conn_index{1024};
				cid = ++conn_index; 
			}	 

			virtual ~TcpConnection()
			{
				//clear all conn_timers to keep safety				
			}

			void init(NetOptions opts,  SocketPtr sock = nullptr,const  KNetWorkerPtr  & worker = nullptr, KNetHandler<T> * evtHandler = nullptr)
			{
                
				event_worker = worker;			 
				tcp_socket   = sock;
				if (tcp_socket){
					asio::error_code ec;
					asio::ip::tcp::endpoint remoteAddr = tcp_socket->socket().remote_endpoint(ec);
					if (!ec){
						remote_host = remoteAddr.address().to_string(); 
						remote_port = remoteAddr.port(); 
					}  
				}
				user_event_handler = evtHandler;
				tcp_socket->init(this->shared_from_this() , opts ); 				
				handle_event(EVT_CREATE);//TODO process_event(EVT_CREATE);	
			}
			
			void deinit() {
				user_event_handler = nullptr; 
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

			bool vsend(const std::vector<asio::const_buffer> &bufs) { return tcp_socket->vsend(bufs); }

			void close(bool force = false )
			{
				if (force){
					//force stop reconnect 
                    disable_reconnect(); 
				}				
				if (tcp_socket)	{
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
					knet_elog("socket is invalid");
				}
				return false; 
			}
			
			inline void bind_package_handler(const PackageHandler & handler) { package_handler = handler; }
			void bind_package_handler(const SelfPackageHandler & handler)
			{
				T *child = static_cast<T *>(this);
				package_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
			}

			inline void bind_data_handler(const KNetDataHandler & handler) { data_handler = handler; }
			void bind_data_handler(const SelfDataHandler & handler)
			{
				T *child = static_cast<T *>(this);
				data_handler = std::bind(handler, child, std::placeholders::_1);
			}

			inline void bind_event_handler(const KNetEventHandler & handler) { event_handler = handler; }
			void bind_event_handler(const SelfEventHandler & handler)
			{
				T *child = static_cast<T *>(this);
				event_handler = std::bind(handler, child, std::placeholders::_1);
			}

			inline bool is_connected() { return tcp_socket && tcp_socket->is_open(); }
			inline bool is_connecting() { return tcp_socket && tcp_socket->is_connecting(); }

			bool connect(const KNetUrl & urlInfo    )
			{		 
				knet_dlog("start to connect {}:{}", urlInfo.get_host(), urlInfo.get_port()); 
				remote_host = urlInfo.get_host(); 
				remote_port = urlInfo.get_port(); 
				passive_mode = false; 
				return tcp_socket->connect(urlInfo);
			}		

			bool connect() { 
				passive_mode = false; 
                return tcp_socket->connect(); 
            }

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

			void set_remote_address(const KNetUrl &urlInfo ){
				if (tcp_socket){
					tcp_socket->set_remote_url(urlInfo); 
				}				
			}


			int32_t sync_sendv(const std::vector<std::string_view > & buffers){
				if (tcp_socket){
					return tcp_socket->sync_sendv(buffers); 
				 }
				 return 0; 
			}

			int32_t sync_sendv(const std::vector<std::string > & buffers){
				if (tcp_socket){
					return tcp_socket->sync_sendv(buffers); 
				 }
				 return 0; 
			}
			
			int32_t sync_send(const char* pData, uint32_t dataLen) {
				 if (tcp_socket){
					return tcp_socket->sync_send(pData, dataLen); 
				 }
				 return 0; 
			 }

			  template <class P, class... Args>
				int32_t sync_msend(const P& first, const Args&... rest)  {
					 if (tcp_socket){	
						return tcp_socket->mpush_sync(first, rest ...); 
					}
					return 0; 
				}
 
            int32_t sync_read(const std::function<uint32_t (const char *, uint32_t len )> & handler )    {
                if(tcp_socket){  
                    return this->tcp_socket->do_sync_read(handler); 
                }
                return 0; 
            }

			inline uint64_t get_cid() const { return cid; }

			inline bool is_passive() const  { return passive_mode; }

			void enable_reconnect(uint32_t interval = 1000000)
			{
				int randTime = (std::rand()%10 +1) * 100000; 
				if (reconn_interval == 0){
					reconn_interval = interval + randTime;
				}

				if (!reconn_flag){
					reconn_flag = true; 
					auto self = this->shared_from_this();
					this->reconn_timer = this->start_timer( [self]() {
							if (self->tcp_socket ){
								if ( !( self->tcp_socket->is_open() || self->tcp_socket->is_connecting() ))
								{
									self->connect();						 
								}								
							}
							return true; 
						}, reconn_interval);

					reconn_interval = reconn_interval * 1.5 + randTime; 
					if (reconn_interval > 30 * 1000000){
						reconn_interval = 30 * 1000000 + randTime; 
					}
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
 
			virtual int32_t handle_package(const char * data, uint32_t len ){
				return len  ; 
			}
 
			virtual bool handle_event(NetEvent evt) 
			{ 
				return true; 
			}

	
			virtual bool handle_data(char * data, uint32_t dataLen)
			{
				return true; 
			}

			uint64_t start_timer(const knet::utils::Timer::TimerHandler &  handler, uint64_t interval, bool bLoop = true)
			{
				if (event_worker)
				{
					uint64_t tid = event_worker->start_timer(handler, interval, bLoop);

					{
						std::lock_guard<std::mutex> guard(timer_mutex);
						conn_timers.push_back(tid);
					}
					  
					return tid;
				}
				return 0;
			}

			void stop_timer(uint64_t timerId)
			{
				if (event_worker)
				{
					event_worker->stop_timer(timerId);
					//needn't to remove the timerid  from conn_timers
				}
			}
			void clear_timers() {			
                std::lock_guard<std::mutex> guard(timer_mutex);
				//knet_tlog("release connection, timer size {} ", conn_timers.size()); 
                for(auto tid : conn_timers){
					stop_timer(tid); 
				}
                conn_timers.clear();
			}
	
			inline void release(){
                clear_timers(); 
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

			bool process_data(char * data, uint32_t dataLen ){
				bool ret = true; 
				if (data_handler)
				{
					ret = data_handler(data, dataLen);
				}

				if (ret && user_event_handler)
				{
					ret = user_event_handler->handle_data(this->shared_from_this(), data, dataLen );
				}		

				if (ret) {
					ret = handle_data(data, dataLen);
				}
		 		return ret; 				
			}

			void *user_data = nullptr;  
	private:
			bool process_event(NetEvent evt){
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
			uint32_t reconn_interval = 0; 

			bool passive_mode = true; 

			std::string remote_host; 
			uint16_t remote_port =0; 

			std::mutex  timer_mutex; 
			std::vector<uint64_t> conn_timers;  //timerid should not duplicated 
			SocketPtr tcp_socket = nullptr;
			KNetEventHandler    event_handler   = nullptr;
			KNetDataHandler     data_handler    = nullptr;
			PackageHandler  package_handler = nullptr;  
			KNetWorkerPtr   event_worker = nullptr;
			KNetHandler<T>* user_event_handler = nullptr;
	 
		};

	} // namespace tcp
} // namespace knet
