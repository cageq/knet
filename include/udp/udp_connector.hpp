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
				m.factory = fac;
				m.worker = w;

				add_factory_event_handler(std::integral_constant<bool, std::is_base_of<KNetHandler<T>, Factory >::value>(), fac);
			}


			bool start(FactoryPtr fac = nullptr)
			{
				m.factory = fac; 
				if (m.worker == nullptr)
				{
					m.worker = std::make_shared<Worker>();
					m.worker->start();
				}
				return true;
			}


			TPtr connect(const std::string& host, uint32_t port, uint32_t localPort = 0,
				const std::string& localAddr = "0.0.0.0")
			{
				if (!m.worker){
					elog("should init worker first"); 
					return nullptr; 
				}

				TPtr conn = nullptr;
					
				if (m.factory)
				{
					conn = m.factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}
				conn->init(nullptr, m.worker,this);

				asio::ip::address remoteAddr = asio::ip::make_address(host);
				asio::ip::udp::endpoint remotePoint(remoteAddr, port);
				
				if (remoteAddr.is_multicast())
				{
					conn->connect(  remotePoint, localPort, localAddr);
				}
				else
				{
					udp::resolver resolver(m.worker->context());
					udp::resolver::results_type endpoints =
						resolver.resolve(remotePoint.protocol(), host, std::to_string(port));

					if (!endpoints.empty())
					{
						conn->connect(  *endpoints.begin(), localPort);
					}
				}

				m.connections[addrstr(remotePoint)] = conn;
				return conn;
			}
			void stop() {
				m.connections.clear();
			}


			virtual bool handle_data(std::shared_ptr<T>, const std::string& msg) {

				return true;
			}
			virtual bool handle_event(std::shared_ptr<T>, NetEvent) {
				return true;
			}
			void add_event_handler(KNetHandler<T>* handler) {
				if (handler) {
					m.event_handler_chain.push_back(handler);
				}
			}
		private:
				bool  invoke_data_chain(TPtr conn, const std::string& msg) {
				bool ret = true;
				for (auto handler : m.event_handler_chain) {
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
				for (auto handler : m.event_handler_chain) {
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
			struct
			{
				FactoryPtr factory = nullptr;
				WorkerPtr worker;
				std::vector< KNetHandler<T>*>  event_handler_chain;
				std::unordered_map<std::string, TPtr> connections;
			} m;
		};

	} // namespace udp
} // namespace knet
