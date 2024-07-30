
//***************************************************************
//    created:    2020/08/01
//    author:        arthur wong
//***************************************************************
#pragma once
#include <unordered_map>
#include "kcp/kcp_connection.hpp"

using asio::ip::udp;

namespace knet
{
    namespace kcp
    {
        template <typename T, class Factory = KNetFactory<T>, typename Worker = KNetWorker>
        class KcpConnector
        {

        public:
            using TPtr = std::shared_ptr<T>;
 
            using WorkerPtr = std::shared_ptr<Worker>;
            using FactoryPtr = Factory *;

            KcpConnector(WorkerPtr w = nullptr)
            {
                if (!w)
                {
                    net_worker = std::make_shared<Worker>();
                    net_worker->start();
                }
                else
                {
                    net_worker = w;
                }
            }
            bool start( FactoryPtr fac = nullptr)
            {
                // event_handler = evtHandler;
                net_factory = fac;
                return true;
            }

            template <class... Args>
            TPtr add_connection(const KNetUrl &urlInfo, Args... args)
            {
                TPtr conn = nullptr;
                if (net_factory)
                {
                    conn = net_factory->create(args...);
                }
                else
                {
                    conn = std::make_shared<T>(args...);
                }

                conn->init(net_worker);
                connections[conn->cid] = conn; 
                 
                udp::resolver resolver(net_worker->context());
                udp::resolver::results_type endpoints =
                    resolver.resolve(udp::v4(), urlInfo.host, std::to_string(urlInfo.port));
                if (!endpoints.empty())
                {
                    conn->connect(*endpoints.begin());
                }
                return conn;
            }
            TPtr connect(const std::string &host, uint16_t port, uint64_t id = 0, bool reconn = false )
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
                if (id > 0)
                {
                    conn->cid = id;
                }
                conn->init(net_worker);
                connections[conn->cid] = conn;

                // conn->event_handler = event_handler;
                udp::resolver resolver(net_worker->context());
                udp::resolver::results_type endpoints =
                    resolver.resolve(udp::v4(), host, std::to_string(port));

                if (!endpoints.empty())
                {
                    conn->connect(*endpoints.begin(), reconn);
                }
                return conn;
            }

            bool remove(TPtr conn)
            {
                auto itr = connections.find(conn->cid);
                if (itr != connections.end())
                {
                    connections.erase(itr);
                    return true;
                }
                return false;
            }

            void stop()
            {

                for (auto &conn : connections)
                {
                    conn->disconnect();
                }
                connections.clear();
            }

        private:
            void release(const TPtr &conn)
            {
                if (net_worker)
                {
                    net_worker->post([this, conn]()
                                     {
                        if (net_factory)
                        {
                            net_factory->release(conn);
                        } });
                }
            }
            FactoryPtr net_factory = nullptr;
            WorkerPtr net_worker;
            std::unordered_map<uint64_t, TPtr> connections;
     
        };

    } // namespace udp
} // namespace knet
