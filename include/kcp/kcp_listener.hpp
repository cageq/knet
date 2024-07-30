
//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include <unordered_map>
#include "kcp/kcp_connection.hpp"

namespace knet
{
	namespace kcp
	{

		template <typename T, class Factory = KNetFactory<T>, typename Worker = KNetWorker>
		class KcpListener
		{
		public:
			enum
			{
				MaxReceiveBufferSize = 4096
			};
			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<TPtr(TPtr, NetEvent, const std::string &)>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*;
			KcpListener(WorkerPtr w = nullptr) 
			{
				default_worker = w;
				if (default_worker == nullptr)
				{
					default_worker = std::make_shared<Worker>();
					default_worker->start();
				}
			}
			bool start(const KNetUrl &  url, EventHandler evtHandler = nullptr, FactoryPtr fac  = nullptr){
				knet_dlog("start kcp server at {}:{}",url.host,url.port); 
				net_factory = fac; 
				listen_port = url.port;  
				run();
				return true;
			}

			bool start(uint32_t port, EventHandler evtHandler = nullptr)
			{
				listen_port = port;
				event_handler = evtHandler;
				run();
				return true;
			}

			TPtr find_connection(udp::endpoint pt)
			{
				auto itr = connections.find(addrstr(pt));
				if (itr != connections.end())
				{
					return itr->second;
				}
				return nullptr;
			}

			TPtr create_connection(const udp::endpoint &  pt)
			{
				TPtr conn = nullptr; 

				if (net_factory)
				{
					conn = net_factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}
				auto worker = this->get_worker();				
				conn->init(worker);
			 
				conn->remote_point = pt;
				conn->event_handler = event_handler;
				connections[addrstr(pt)] = conn;
				return conn;
			}

			bool destroy_connection(TPtr conn)
			{
				auto itr = connections.find(conn->cid);
				if (itr != connections.end())
				{
					connections.erase(itr);
					return true;
				}
				return false;
			}
			void broadcast(const std::string_view & msg){

				for(auto &  conn: connections){
					conn.second->msend(msg);
				}

			}

			void stop()
			{
				if (server_socket)
				{
					server_socket->close();
				}
			}

		private:
			// static TPtr create_helper(Args... args)
			// {
			// 	return std::make_shared<T>(args...);
			// }
			void do_receive()
			{
				server_socket->async_receive_from(asio::buffer(recv_buffer, MaxReceiveBufferSize), remote_point,
												  [this](std::error_code ec, std::size_t bytes_recvd)
												  {
													  if (!ec && bytes_recvd > 0)
													  {
														  knet_dlog("received data {} from {}:{}", bytes_recvd,
															   remote_point.address().to_string(), remote_point.port());

														  auto conn = this->find_connection(this->remote_point);
														  if (!conn)
														  {
															  conn = this->create_connection(remote_point);
															  conn->listen(server_socket);
														  }

														  auto timeNow = std::chrono::system_clock::now();
														  conn->last_msg_time = std::chrono::duration_cast<std::chrono::milliseconds>(
															  timeNow.time_since_epoch());

														  if (!conn->check_control_message((const char *)recv_buffer, bytes_recvd))
														  {
															  conn->handle_receive((const char *)recv_buffer, bytes_recvd);
														  }

														  do_receive();
													  }
													  else
													  {
														  knet_elog("receive error");
													  }
												  });
			}

			void run()
			{

				default_worker->post([this]()
									 {
										 knet_dlog("start kcp server @ {}", listen_port);
										 server_socket = std::make_shared<udp::socket>(default_worker->context(), udp::endpoint(udp::v4(), listen_port));
										 do_receive();
				 });
			}

			inline WorkerPtr get_worker()
			{
				return default_worker;
			}

			char recv_buffer[MaxReceiveBufferSize];

			uint32_t listen_port;
			udp::endpoint remote_point;
			std::shared_ptr<udp::socket> server_socket;
			std::unordered_map<std::string, TPtr> connections;
			EventHandler event_handler;
			FactoryPtr net_factory = nullptr;
			//std::tuple<Args...> conn_args;
			WorkerPtr default_worker;
		};

	} // namespace udp
} // namespace knet
