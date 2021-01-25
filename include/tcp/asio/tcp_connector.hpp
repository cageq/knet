//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include <unordered_map>
#include "tcp_connection.hpp"
#include "user_factory.hpp"
#include "event_worker.hpp"
#include  <type_traits>

namespace knet
{
	namespace tcp
	{ 
		template <class T, class Factory = UserFactory<T>, class Worker = EventWorker>
		class TcpConnector : public UserEventHandler<T>
		{
		public:
			using TPtr = std::shared_ptr<T>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*;

			TcpConnector(FactoryPtr fac = nullptr, WorkerPtr worker = nullptr)
			{
				m.factory = fac;

				if (worker)
				{
					user_workers.emplace_back(worker);
					// worker->start(); 
				}
				else
				{
					// worker = std::make_shared<Worker>();
					// user_workers.emplace_back(worker);
					// worker->start();
				}
				//factory_event_helper<std::is_base_of<UserEventHandler<T> , Factory  >::value>( fac); 		  

				add_factory_event_handler(std::integral_constant<bool, std::is_base_of<UserEventHandler<T> , Factory >::value>() , fac); 
			}



			inline void add_factory_event_handler(std::true_type , FactoryPtr fac){
				
					auto evtHandler = static_cast<UserEventHandler<T> *>(fac); 	
					if (evtHandler){
						add_event_handler(evtHandler); 
					}					
			}

			inline void add_factory_event_handler(std::false_type , FactoryPtr fac){
				
			}


		// template<bool flag>
		// using allow_if = typename std::enable_if<flag>::type;
		// 	template<bool S = true>
		// 		allow_if<S> factory_event_helper(FactoryPtr fac){
		// 			elog("add factory event helper {}", std::is_base_of<UserEventHandler<T> , Factory >::value ); 
		// 			auto evtHandler = static_cast<UserEventHandler<T> *>(fac); 	
		// 			if (evtHandler){
		// 				add_event_handler(evtHandler); 
		// 			}					
		// 		}

		// 	template<bool S = true>
		// 	allow_if<!S> factory_event_helper(FactoryPtr fac){
		// 		elog("not add factory event helper {}", std::is_base_of<UserEventHandler<T> , Factory >::value ); 
				 
		// 	}
	

			void add_worker(WorkerPtr worker)
			{
				if (worker)
				{
					user_workers.push_back(worker);
				}
			}

			bool start(uint32_t thrds = 1, FactoryPtr fac = nullptr)
			{
				if (fac != nullptr)
				{
					m.factory = fac;
				}
				// nothing to do
				for (uint32_t i = 0; i < thrds; i++)
				{
					auto worker = std::make_shared<Worker>();
					user_workers.emplace_back(worker);
					worker->start();
				}

				return true;
			}

			void stop() {
				for (auto& elem : connections) {
					elem.second->close();
				}
			}



			bool remove_connection(uint64_t cid)
			{
				auto itr = connections.find(cid);
				if (itr != connections.end())
				{
					connections.erase(itr);
					return true;
				}
				return false;
			}


			bool add_connection(TPtr conn, const ConnectionInfo& connInfo)
			{
				if (conn)
				{
					auto worker = this->get_worker();
					auto sock =
						std::make_shared<typename T::ConnSock>(worker->thread_id(), worker->context());
					conn->init( sock, worker, this );			 
					asio::ip::tcp::endpoint endpoint(asio::ip::make_address(connInfo.server_addr), connInfo.server_port);
					conn->connect(connInfo);
					connections[conn->get_cid()] = conn;
					return true;
				}
				return false;
			}

			template <class... Args>
			TPtr add_connection(const ConnectionInfo& connInfo, Args... args)
			{
				auto worker = this->get_worker();
				auto sock = std::make_shared<typename T::ConnSock>(worker->thread_id(), worker->context());
				TPtr conn = nullptr;
				if (m.factory)
				{
					conn = m.factory->create(args...);
				}
				else
				{
					conn = std::make_shared<T>(args...);
				}

				conn->init(sock, worker, this);
				conn->connect(connInfo);
				connections[conn->get_cid()] = conn;
				return conn;
			}

			// template <class... Args>
			// TPtr add_ssl_connection(
			// 	const std::string& host, uint16_t port, const std::string& caFile, Args... args)
			// {
			// 	auto worker = this->get_worker();
			// 	auto sock =
			// 		std::make_shared<typename T::ConnSock>(worker->thread_id(), worker->context(), caFile);
			// 	TPtr conn = nullptr;
			// 	if (factory)
			// 	{
			// 		conn = factory->create(args...);
			// 	}
			// 	else
			// 	{
			// 		conn = std::make_shared<T>(args...);
			// 	}

			// 	conn->init( sock, worker, this );
		 
			// 	conn->connect(host, port);
			// 	connections[conn->cid] = conn;

			// 	return conn;
			// }

			// template <class... Args>
			// TPtr add_wsconnection(const std::string& host, uint16_t port, Args... args)
			// {
			// 	auto worker = get_worker();
			// 	auto sock = std::make_shared<typename T::ConnSock>(worker->thread_id(), worker->context());
			// 	TPtr conn = nullptr;
			// 	if (factory)
			// 	{
			// 		conn = factory->create(args...);
			// 	}
			// 	else
			// 	{
			// 		conn = std::make_shared<T>(args...);
			// 	}
			// 	conn->init( sock, worker, this );
			// 	conn->connect(host, port);
			// 	connections[conn->cid] = conn;
			// 	return conn;
			// }

 

			WorkerPtr get_worker(int32_t idx = 0)
			{
				if (!user_workers.empty())
				{
					if (idx >= 0 && idx < (int32_t)user_workers.size())
					{
						return user_workers[idx];
					}
					else
					{
						worker_index = (worker_index + 1) % user_workers.size();
						return user_workers[worker_index];
					}
				}
				else
				{
					elog("not worker, please create worker");
					return nullptr;
				}
			}

		private:
			virtual bool handle_data(std::shared_ptr<T> conn, const std::string& msg ) {

					return invoke_data_chain(conn, msg  ); 		
			}
			virtual bool handle_event(std::shared_ptr<T> conn , NetEvent evt ) {

				bool ret = invoke_event_chain(conn,evt);  

					if (evt == EVT_RELEASE){

					asio::post(*conn->get_context(), [this, conn]() {

						conn->disable_reconnect();
						if (m.factory)
						{
							m.factory->release(conn);
						}
						});
					}
 
				return ret; 
			}


	 
			void add_event_handler(UserEventHandler<T> * handler){
				if (handler){
					m.event_handler_chain.push_back(handler); 
				}
			}

			bool  invoke_data_chain(TPtr conn, const std::string& msg  ){
				bool ret = true; 
				for(auto handler : m.event_handler_chain){
					if (handler ){
						ret = handler->handle_data(conn, msg); 
						if (!ret){
							break; 
						}
					}
				}
				return ret; 
			}

			bool invoke_event_chain(TPtr conn, NetEvent evt){
				bool ret = true; 
				for(auto handler : m.event_handler_chain){
					if (handler ){
						ret = handler->handle_event(conn, evt); 
						if (!ret)
						{
							break; 
						}
					}
				}
				return ret; 
			}


			uint32_t worker_index = 0;			
			std::vector<WorkerPtr> user_workers;
			std::unordered_map<uint64_t, TPtr> connections;
			struct {
				FactoryPtr factory = nullptr;
				std::vector< UserEventHandler<T> *>  event_handler_chain; 				
			} m; 

			
		};

	} // namespace tcp
} // namespace knet
