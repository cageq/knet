
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
			enum { kMaxRecvBufferSize = 4096 };
	

			UdpListener(WorkerPtr w = nullptr)
				: net_worker(w) {
				if (net_worker == nullptr) {
					net_worker = std::make_shared<Worker>();
					net_worker->start();
				}
				
			}

			bool start(uint16_t port, NetOptions opt = {}, FactoryPtr fac  = nullptr ){
				return start({"udp","0.0.0.0",port},opt, fac); 
			}

			bool start(KNetUrl url, NetOptions opt = {}, FactoryPtr fac  = nullptr ){
				url_info = url; 
				net_factory = fac; 
				asio::ip::address lisAddr = asio::ip::make_address(url_info.host);
				server_socket = std::make_shared<udp::socket>(net_worker->context());
				
				asio::ip::udp::endpoint lisPoint(lisAddr, url_info.port);
				server_socket->open(lisPoint.protocol());
				server_socket->set_option(asio::ip::udp::socket::reuse_address(opt.reuse));
				server_socket->bind(lisPoint);

				std::string multiHost = url.get("multi"); 				
				if (!multiHost.empty()) {
					multi_host = multiHost;
					// Create the socket so that multiple may be bound to the same address.
					knet_dlog("join multi address {}", multiHost);
					asio::ip::address multiAddr = asio::ip::make_address(multiHost);
					server_socket->set_option(asio::ip::multicast::join_group(multiAddr));
				}
 
				knet_dlog("start udp server {}:{}", url_info.host, url_info.port);
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
					asio::ip::udp::endpoint multiPoint(multiAddr, listen_port);
					knet_dlog("broadcast message to {}:{}", multiPoint.address().to_string(), multiPoint.port());
					server_socket->async_send_to(asio::buffer(*buffer), multiPoint,
						[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
							if (!ec) {						 
								this->handle_event(nullptr, EVT_SEND); 
							}
							else {
								knet_dlog("sent message error : {}, {}", ec.value(), ec.message());
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
		 
				knet_dlog("add remote connection {}", addrstr(pt));
				connections[addrstr(pt)] = conn;
				return conn;
			}

			void stop() {
				if (server_socket){
					server_socket->close();
				}				
				connections.clear();
			}

			void add_net_handler(KNetHandler<T> *handler)
			{
				if (handler)
				{
					event_handler_chain.push_back(handler);
				}
			}
			void push_front(KNetHandler<T> *handler){
				if (handler)
				{
					auto beg = event_handler_chain.begin(); 
					event_handler_chain.insert(beg, handler);
				}
			}

			void push_back(KNetHandler<T> *handler){
				if (handler)
				{
					event_handler_chain.push_back(handler);
				}
			}

		private:			 
			virtual bool handle_data(TPtr conn, char * data, uint32_t dataLen)
			{						
				return invoke_data_chain(conn, data, dataLen);
			}

			virtual bool handle_event(TPtr conn, NetEvent evt)
			{				
				bool ret = invoke_event_chain(conn, evt);
				if (evt == EVT_RELEASE)
				{
					this->release(conn);
				}
				return ret;
			} 

			void release(const TPtr &  conn)
			{
				if (net_worker) 
				{
					net_worker->post([this, conn]() {
							if (net_factory)
							{
								net_factory->release(conn);
							}
							}); 
				}
			}

			bool invoke_data_chain(const TPtr &  conn, char * data, uint32_t dataLen)
			{
				bool ret = true;
				for (auto &handler : event_handler_chain)
				{
					if (handler)
					{
						ret = handler->handle_data(conn, data, dataLen);
						if (!ret)
						{
							break;
						}
					}
				}
				return ret;
			}

			bool invoke_event_chain(const TPtr &  conn, NetEvent evt)
			{
				bool ret = true;
				for (auto &handler : event_handler_chain)
				{
					if (handler)
					{
						ret = handler->handle_event(conn, evt);
						if (!ret)
						{
							break;
						}
					}
				}
				return ret;
			}

 


			void do_receive() {
				server_socket->async_receive_from(asio::buffer(recv_buffer, kMaxRecvBufferSize), remote_point,
					[this](std::error_code ec, std::size_t bytes_recvd) {
						auto conn = this->find_connection(remote_point);
						if (!ec && bytes_recvd > 0) {
							knet_dlog("get message from {}:{}", remote_point.address().to_string(), remote_point.port());						
							if (!conn) {
								conn = this->create_connection(remote_point);
								conn->init(server_socket, net_worker, static_cast<KNetHandler<T>*>(this)  );							
								conn->remote_point = remote_point;
								// if (multi_host.empty()) {
								// 	conn->remote_point = remote_point;
								// } else {
								// 	conn->remote_point =
								// 		asio::ip::udp::endpoint(asio::ip::make_address(multi_host), listen_port);
								// }

								bool ret = conn->handle_event(EVT_CONNECT);  
								if (ret){
									this->handle_event(conn, EVT_CONNECT);  
								}								

							}
							auto pkgType = conn->handle_package(std::string((const char *)recv_buffer, bytes_recvd));
							if (pkgType == PACKAGE_USER)
							{								
								recv_buffer[bytes_recvd] = 0;				 
								bool ret = conn->handle_data((char*)recv_buffer, bytes_recvd); 
								if (ret){
									this->handle_data(conn, (char*)recv_buffer, bytes_recvd); 
								}
								
								ret = conn->handle_event(EVT_RECV);  
								if (ret){
									this->handle_event(conn, EVT_RECV);  
								}								
							}

						}else {
							knet_elog("receive error from {}:{}", remote_point.address().to_string(), remote_point.port());							
							if(conn){
								bool ret = conn->handle_event(EVT_DISCONNECT);  
								if (ret){
									this->handle_event(conn, EVT_DISCONNECT);  
								}						
							}						
							
							return ; 
						}
						do_receive();
					});
			}
 
	
			char recv_buffer[kMaxRecvBufferSize];
			uint32_t listen_port;
			std::string multi_host;
			udp::endpoint remote_point;
			std::shared_ptr<udp::socket> server_socket;
			std::unordered_map<std::string, TPtr> connections;
 
			FactoryPtr net_factory = nullptr;
			WorkerPtr net_worker;
			std::vector<KNetHandler<T> *> event_handler_chain;
			KNetUrl url_info; 		
			std::shared_ptr<UdpConnection<T>>   server_connection; 

		};

	} // namespace udp
} // namespace knet
