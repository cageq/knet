//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <cstddef>
#include <tuple>
#include <memory>
#include <vector>
#include "tcp_connection.hpp"
#include "knet_factory.hpp"

namespace knet
{
	namespace tcp
	{

		using asio::ip::tcp;
		template <class T, class Factory = KNetFactory<T>, class Worker = KNetWorker>
			class TcpListener final : public KNetHandler<T>
		{
			public:
				using TPtr = std::shared_ptr<T>;
				using FactoryPtr = Factory *;
				using WorkerPtr = std::shared_ptr<Worker>;
				using SocketPtr = std::shared_ptr<TcpSocket<T>>;

				TcpListener(FactoryPtr fac = nullptr, WorkerPtr lisWorker = nullptr)
				{
					if (lisWorker == nullptr){
						m.listen_worker = std::make_shared<Worker>(); 
						m.listen_worker->start();
					}else {
						m.listen_worker = lisWorker;
					}

					m.factory = fac;
					if (fac != nullptr)
					{					
						add_factory_event_handler(std::is_base_of<KNetHandler<T>, Factory>(), fac);
					}

					tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m.listen_worker->context()); 
				}

				TcpListener(WorkerPtr lisWorker)
				{  
					if (lisWorker == nullptr){
						m.listen_worker = std::make_shared<Worker>(); 
						m.listen_worker->start();
					}else {
						m.listen_worker = lisWorker;
					} 
				 
					tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(m.listen_worker->context()); 
				}

				void add_worker(WorkerPtr worker)
				{
					if (worker)
					{
						m.user_workers.push_back(worker);
					}
				}

				TcpListener(const TcpListener &) = delete;

				~TcpListener() {}

				bool start(NetOptions opt, void *sslCtx = nullptr)
				{
					m.options = opt;
					m.ssl_context = sslCtx;
					if (!m.is_running)
					{
						m.is_running = true;
						asio::ip::tcp::endpoint endpoint(asio::ip::make_address(opt.host), opt.port);

						// this->tcp_acceptor.open(asio::ip::tcp::v4());
						this->tcp_acceptor->open(endpoint.protocol());
						if (tcp_acceptor->is_open())
						{
							this->tcp_acceptor->set_option(asio::socket_base::reuse_address(true));
							//	this->tcp_acceptor.set_option(asio::ip::tcp::no_delay(true));
							this->tcp_acceptor->non_blocking(true);

							asio::socket_base::send_buffer_size SNDBUF(m.options.send_buffer_size);
							this->tcp_acceptor->set_option(SNDBUF);
							asio::socket_base::receive_buffer_size RCVBUF(m.options.recv_buffer_size);
							this->tcp_acceptor->set_option(RCVBUF);

							asio::error_code ec;
							this->tcp_acceptor->bind(endpoint, ec);
							if (ec)
							{
								elog("bind address failed {}:{}", opt.host, opt.port);
								m.is_running = false;
								return false;
							}
							this->tcp_acceptor->listen(m.options.backlogs, ec);

							if (ec)
							{
								elog("start listen failed");
								m.is_running = false;
								return false;
							}
							this->do_accept();
						}
						else
						{
							return false;
						}
					}
					return true;
				}

				bool start(uint16_t port = 9999, const std::string &host = "0.0.0.0", void *ssl = nullptr)
				{
					m.options.host = host;
					m.options.port = port;
					return start(m.options, ssl);
				}

				void stop()
				{
					dlog("stop listener thread");
					if (m.is_running)
					{
						m.is_running = false;
						tcp_acceptor->close();
						for(auto & worker : m.user_workers){
							worker->stop(); 
						}
						m.user_workers.clear();

						m.listen_worker->stop();
					}
				}

				virtual bool handle_data(TPtr conn, const std::string &msg)
				{
					return invoke_data_chain(conn, msg);
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

				void add_event_handler(KNetHandler<T> *handler)
				{
					if (handler)
					{
						m.event_handler_chain.push_back(handler);
					}
				}
				void add_first(KNetHandler<T> *handler){
					if (handler)
					{
						auto beg = m.event_handler_chain.begin(); 
						m.event_handler_chain.insert(beg, handler);
					}
				}

				void add_last(KNetHandler<T> *handler){
					if (handler)
					{
						m.event_handler_chain.push_back(handler);
					}
				}

			private:
				inline void add_factory_event_handler(std::true_type, FactoryPtr fac)
				{
					auto evtHandler = dynamic_cast<KNetHandler<T> *>(fac);
					if (evtHandler)
					{
						add_event_handler(evtHandler);
					}
				}

				inline void add_factory_event_handler(std::false_type, FactoryPtr fac)
				{
				}

				bool invoke_data_chain(const TPtr &  conn, const std::string &msg)
				{
					bool ret = true;
					for (auto &handler : m.event_handler_chain)
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

				bool invoke_event_chain(const TPtr &  conn, NetEvent evt)
				{
					bool ret = true;
					for (auto &handler : m.event_handler_chain)
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

				void release(const TPtr &  conn)
				{
					asio::post(m.listen_worker->context(), [this, conn]() {
							if (m.factory)
							{
							m.factory->release(conn);
							}
							});
				}
				void do_accept()
				{
					// dlog("accept new connection ");
					auto worker = this->get_worker();
					if (!worker)
					{
						elog("no event worker, can't start");
						return;
					}

					auto socket = std::make_shared<TcpSocket<T>>(worker->thread_id(), worker->context(), m.ssl_context);
					tcp_acceptor->async_accept(socket->socket(), [this, socket, worker](std::error_code ec) {
							if (!ec)
							{
							dlog("accept new connection ");
							this->init_conn(worker, socket);
							do_accept();
							}
							else
							{
							elog("accept error");
							}
							});
				}

				WorkerPtr get_worker()
				{
					if (!m.user_workers.empty())
					{
						dlog("dispatch to worker {}", m.worker_index);
						return m.user_workers[m.worker_index++ % m.user_workers.size()];
					}
					else
					{
						//dlog("dispatch work to listen worker {}", std::this_thread::get_id());
						return m.listen_worker;
					}
				}

				void init_conn(WorkerPtr worker, SocketPtr socket)
				{
					if (worker)
					{
						asio::dispatch(worker->context(), [=]() {
								auto conn = create_connection(socket, worker);				 
								socket->init_read();
								});
					}
				}

				TPtr create_connection(SocketPtr sock, WorkerPtr worker)
				{
					auto conn = m.factory->create();
					conn->init(sock, worker, this);
					return conn;
				}

				struct
				{
					uint32_t worker_index = 0;
					std::vector<WorkerPtr> user_workers;
					NetOptions options;
					bool is_running = false;
					void *ssl_context = nullptr;
					FactoryPtr factory = nullptr;
					std::vector<KNetHandler<T> *> event_handler_chain;
					WorkerPtr listen_worker;
				} m;

				std::shared_ptr<asio::ip::tcp::acceptor> tcp_acceptor;
		};

		template <typename T>
			using DefaultTcpListener = TcpListener<T, KNetFactory<T>, KNetWorker>;

	} // namespace tcp

} // namespace knet
