//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <unordered_map>
#include "tcp_connection.hpp"
#include "knet_factory.hpp"
#include "knet_worker.hpp"
#include <type_traits>

namespace knet
{
	namespace tcp
	{
		template <class T, class Factory = KNetFactory<T>, class Worker = KNetWorker>
		class TcpConnector : public KNetHandler<T>
		{
		public:
			using TPtr = std::shared_ptr<T>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory *;
            using Socket  = typename T::ConnSock; 
			using ConnectionMap =  std::unordered_map<uint64_t, TPtr>; 

			TcpConnector(FactoryPtr fac = nullptr, WorkerPtr worker = nullptr)
			{
				net_factory = fac;
				if (worker)
				{
					user_workers.emplace_back(worker);
				}
				else
				{
					 worker = std::make_shared<Worker>(nullptr , this );//use this as worker's owner
					 user_workers.emplace_back(worker);
					 worker->start();
				}
				if (net_factory)
				{
					add_factory_event_handler(std::integral_constant<bool, std::is_base_of<KNetHandler<T>, Factory>::value>(), fac);
				}				
			}


			TcpConnector(WorkerPtr worker)
			{  
				if (worker == nullptr){
					worker = std::make_shared<Worker>(nullptr , this );//use this as worker's owner 
					worker->start();
				} 
				user_workers.emplace_back(worker);
			}

			virtual ~TcpConnector() {}

			void add_worker(WorkerPtr worker)
			{
				if (worker)
				{
					user_workers.push_back(worker);
				}
			}

			bool start(FactoryPtr fac = nullptr, uint32_t threads = 0)
            {
                if (fac != nullptr)
				{
					net_factory = fac;
				}
                for (uint32_t i = 0; i < threads ; i++)
				{
					auto worker = std::make_shared<Worker>(nullptr , this );
					user_workers.emplace_back(worker);
					worker->start();
				}
                return true; 
            }

			void stop()
			{
			//	event_handler_chain.clear(); 
				for (auto &elem : connections)
				{
					elem.second->deinit(); 
					elem.second->close(true);
				}
				connections.clear(); 
				
				for(auto & worker: user_workers){
					if (worker->get_user_data() == this){//inner worker
					 	worker->stop(); 
					}
				}			
				user_workers.clear(); 
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

			bool add_connection(TPtr conn, const KNetUrl &urlInfo )
			{
				if (conn)
				{		 
					auto worker = this->get_worker();
					auto sock = std::make_shared<Socket>(worker->thread_id(), worker);
                    auto opts = options_from_url(urlInfo); 
					conn->init(opts, sock, worker, this);					 
					conn->connect(urlInfo );
					connections[conn->get_cid()] = conn;
					return true;
				}
				return false;
			}

			TPtr find_connection(uint64_t sid ){
				auto itr = connections.find(sid); 
				if (itr != connections.end()){
					return itr->second; 
				}
				return nullptr; 				
			}
			inline ConnectionMap get_connections() const {
				return connections; 
			}

			template <class... Args>
			TPtr add_connection(const KNetUrl &urlInfo, Args... args)
			{
				auto worker = this->get_worker();
				auto sock = std::make_shared<Socket>(worker->thread_id(), worker);
				TPtr conn = nullptr;
				if (net_factory)
				{
					conn = net_factory->create(args...);
				}
				else
				{
					conn = std::make_shared<T>(args...);
				}

                auto opts = options_from_url(urlInfo); 
				conn->init(opts, sock, worker, this);  //init add this to event handler
				conn->connect(urlInfo);
				connections[conn->get_cid()] = conn;
				return conn;
			}

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
					elog("no event worker, please create at least one worker");
					return nullptr;
				}
			}
			void add_event_handler(KNetHandler<T> *handler)
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
			inline void add_factory_event_handler(std::true_type, FactoryPtr fac)
			{
				auto evtHandler = static_cast<KNetHandler<T> *>(fac);
				if (evtHandler)
				{
					add_event_handler(evtHandler);
				}
			}

			inline void add_factory_event_handler(std::false_type, FactoryPtr fac)
			{
			}
			virtual bool handle_data(std::shared_ptr<T> conn, const std::string &msg)
			{

				return invoke_data_chain(conn, msg);
			}
			virtual bool handle_event(std::shared_ptr<T> conn, NetEvent evt)
			{

				bool ret = invoke_event_chain(conn, evt);
				if (evt == EVT_RELEASE)
				{
					conn->post([this, conn]() {
						conn->disable_reconnect();
						if (net_factory)
						{
							net_factory->release(conn);
						}
					});  
				}

				return ret;
			}

			bool invoke_data_chain(TPtr conn, const std::string &msg)
			{
				bool ret = true;
				for (auto handler : event_handler_chain)
				{
					if (handler)
					{
						ret = handler->handle_data(conn, msg);
						if (!ret)
						{
							break;
						}
					}
				}
				return ret;
			}

			bool invoke_event_chain(TPtr conn, NetEvent evt)
			{
				bool ret = true;
				for (auto handler : event_handler_chain)
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

			uint32_t worker_index = 0;
			std::vector<WorkerPtr> user_workers;
			std::unordered_map<uint64_t, TPtr> connections;
			FactoryPtr net_factory = nullptr;
			std::vector<KNetHandler<T> *> event_handler_chain;
		 
		};

	} // namespace tcp
} // namespace knet
