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


			bool start(FactoryPtr fac = nullptr)
			{
				net_factory = fac; 
				if (net_worker == nullptr)
				{
					net_worker = std::make_shared<Worker>(nullptr , this );
					net_worker->start();
				}
				return true;
			}


			TPtr connect(const std::string& host, uint32_t port, uint32_t localPort = 0,
				const std::string& localAddr = "0.0.0.0")
			{
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

				asio::ip::address remoteAddr = asio::ip::make_address(host);
				asio::ip::udp::endpoint remotePoint(remoteAddr, port);
				
				if (remoteAddr.is_multicast())
				{
					conn->connect(remotePoint, localPort, localAddr);
				}
				else
				{
					udp::resolver resolver(net_worker->context());
					udp::resolver::results_type endpoints =
						resolver.resolve(remotePoint.protocol(), host, std::to_string(port));

					if (!endpoints.empty())
					{
						conn->connect(  *endpoints.begin(), localPort);
					}
				}

				connections[addrstr(remotePoint)] = conn;
				return conn;
			}
			void stop() {
				for(auto & item : connections)
				{
					item.second->close();
				}
				connections.clear();  
				if (net_worker && net_worker->get_user_data() == this){
					net_worker->stop(); 
				} 
			}


			virtual bool handle_data(std::shared_ptr<T>, const std::string& msg) {

				return true;
			}
			virtual bool handle_event(std::shared_ptr<T>, NetEvent) {
				return true;
			}
			void add_event_handler(KNetHandler<T>* handler) {
				if (handler) {
					event_handler_chain.push_back(handler);
				}
			}
		private:
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
		};

	} // namespace udp
} // namespace knet
