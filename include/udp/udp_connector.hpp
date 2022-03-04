//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include "udp/udp_connection.hpp"
#include "knet_factory.hpp"
#include "knet_handler.hpp"
#include <cstddef>

using asio::ip::udp;

namespace knet
{
	namespace udp
	{

		template <typename T, class Factory = KNetFactory<T>, typename Worker = KNetWorker>
		class UdpConnector : public KNetHandler<T>
		{

		public:
			using TPtr = std::shared_ptr<T>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*; 

			UdpConnector(FactoryPtr fac = nullptr, WorkerPtr w = nullptr) {
				net_factory = fac;
				net_worker = w;
				if (fac != nullptr ){
					add_factory_event_handler(std::integral_constant<bool, std::is_base_of<KNetHandler<T>, Factory >::value>(), fac);
				} 
			}

			bool start(NetOptions opts = {},FactoryPtr fac = nullptr)
			{
				net_factory = fac; 
				net_options = opts; 
				if (net_worker == nullptr)
				{
					net_worker = std::make_shared<Worker>(nullptr , this );
					net_worker->start();
				}
				return true;
			}
			
			TPtr connect(const KNetUrl &urlInfo)
			{
				url_info = urlInfo; 
				if (!net_worker){
					elog("should init worker first"); 
					return nullptr; 
				}

				TPtr conn = nullptr;
					
				if (net_factory)
				{
					conn = net_factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}
				conn->init(nullptr, net_worker,this);
				asio::ip::address remoteAddr = asio::ip::make_address(url_info.host);
				asio::ip::udp::endpoint remotePoint(remoteAddr, url_info.port);

				std::string bindAddr = url_info.get("bind_addr"); 
				uint16_t bindPort = url_info.get<uint16_t>("bind_port",8888); 
				
				if (remoteAddr.is_multicast())
				{
					conn->connect(remotePoint, bindPort, bindAddr);
				}
				else
				{
					udp::resolver resolver(net_worker->context());
					udp::resolver::results_type endpoints =
						resolver.resolve(remotePoint.protocol(), url_info.host, std::to_string(url_info.port));

					if (!endpoints.empty())
					{
						conn->connect(  *endpoints.begin(), bindPort, bindAddr);
					}
				}

				connections[addrstr(remotePoint)] = conn;
				return conn;
			}

			TPtr connect(const std::string& host, uint16_t port, uint16_t localPort = 0,
				const std::string& localAddr = "0.0.0.0")
			{
				KNetUrl urlInfo {"udp",host,port}; 
				urlInfo.set("bind_port",std::to_string(localPort)); 
				urlInfo.set("bind_addr", localAddr); 
				 return this->connect(urlInfo); 
			}

			void stop() {
				for(auto & item : connections)
				{
					item.second->close();
				}
				connections.clear();  
				if (net_worker && net_worker->get_user_data() == this){ // stop my net worker
					net_worker->stop(); 
				} 
			}


			virtual bool handle_data(std::shared_ptr<T> conn, const std::string& msg) {
				return invoke_data_chain(conn, msg);
				
			}
			virtual bool handle_event(std::shared_ptr<T> conn, NetEvent evt) {
				bool ret = invoke_event_chain(conn, evt);
				if (evt == EVT_RELEASE)
				{
					this->release(conn);
				}
				return ret;
			}

			void add_event_handler(KNetHandler<T>* handler) {
				if (handler) {
					event_handler_chain.push_back(handler);
				}
			}
		private:
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


			bool invoke_data_chain(TPtr conn, const std::string& msg) {
				bool ret = true;
				for (auto & handler : event_handler_chain) {
					if (handler) {
						ret = handler->handle_data(conn, msg);
						if (!ret) {
							break;
						}
					}
				}
				return ret;
			}

			bool invoke_event_chain(TPtr conn, NetEvent evt) {
				bool ret = true;
				for (auto handler : event_handler_chain) {
					if (handler) {
						ret = handler->handle_event(conn, evt);
						if (!ret)
						{
							break;
						}
					}
				}
				return ret;
			}
			inline void add_factory_event_handler(std::true_type, FactoryPtr fac) {
				auto evtHandler = static_cast<KNetHandler<T> *>(fac);
				if (evtHandler) {
					add_event_handler(evtHandler);
				}
			}

			inline void add_factory_event_handler(std::false_type, FactoryPtr fac) {
			}
 

			FactoryPtr net_factory = nullptr;
			WorkerPtr net_worker;
			std::vector< KNetHandler<T>*>  event_handler_chain;
			std::unordered_map<std::string, TPtr> connections;
			KNetUrl url_info; 
			NetOptions net_options; 
		};

	} // namespace udp
} // namespace knet
