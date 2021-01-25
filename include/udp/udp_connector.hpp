//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include "udp/udp_connection.hpp"
#include "user_factory.hpp"
#include "user_event_handler.hpp"

using asio::ip::udp;

namespace knet
{
	namespace udp
	{

		template <typename T, class Factory = UserFactory<T>, typename Worker = EventWorker>
		class UdpConnector : public UserEventHandler<T>
		{

		public:
			using TPtr = std::shared_ptr<T>;
			using WorkerPtr = std::shared_ptr<Worker>;
			using FactoryPtr = Factory*;

	 

			UdpConnector(FactoryPtr f = nullptr, WorkerPtr w = nullptr) {
				m.factory = f;
				m.worker = w;
			}

		//	using EventHandler = std::function<TPtr(TPtr, NetEvent, std::string_view)>;
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

				TPtr conn = nullptr;
					
				if (m.factory)
				{
					conn = m.factory->create();
				}
				else
				{
					conn = std::make_shared<T>();
				}

				asio::ip::address remoteAddr = asio::ip::make_address(host);
				asio::ip::udp::endpoint remotePoint(remoteAddr, port);
				
				if (remoteAddr.is_multicast())
				{
					conn->connect(m.worker->context(), remotePoint, localPort, localAddr);
				}
				else
				{
					udp::resolver resolver(m.worker->context());
					udp::resolver::results_type endpoints =
						resolver.resolve(remotePoint.protocol(), host, std::to_string(port));

					if (!endpoints.empty())
					{
						conn->connect(m.worker->context(), *endpoints.begin(), localPort);
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

		private:

			struct
			{
				FactoryPtr factory = nullptr;
				WorkerPtr worker;
				
				std::unordered_map<std::string, TPtr> connections;
			} m;
		};

	} // namespace udp
} // namespace knet
