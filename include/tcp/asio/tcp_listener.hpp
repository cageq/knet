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
				using Socket = typename T::ConnSock; 
				using SocketPtr = std::shared_ptr<typename T::ConnSock>;

				TcpListener(FactoryPtr fac = nullptr, WorkerPtr lisWorker = nullptr)
				{
					if (lisWorker == nullptr){
						listen_worker = std::make_shared<Worker>(); 
						listen_worker->start();
					}else {
						listen_worker = lisWorker;
					}

					if (fac != nullptr) {					
						net_factory = fac;
						add_factory_event_handler(std::is_base_of<KNetHandler<T>, Factory>(), fac);
					}
					tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(listen_worker->context()); 
				}

				TcpListener(WorkerPtr lisWorker)
				{  
					if (lisWorker == nullptr){
						listen_worker = std::make_shared<Worker>(); 
						listen_worker->start();
					}else {
						listen_worker = lisWorker;
					} 
					tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(listen_worker->context()); 
				}

				void add_worker(WorkerPtr worker)
				{
					if (worker)
					{
						user_workers.push_back(worker);
					}
				}

				TcpListener(const TcpListener &) = delete;

				~TcpListener() {}


				bool start(KNetUrl url, NetOptions opt ={}, void *sslCtx = nullptr)
				{
					url_info = url; 
					net_options = opt;
					ssl_context = sslCtx;
					if (!is_running)
					{
						is_running = true;
                        if(net_options.workers > 0){
                            ilog("start user workers {}", net_options.workers); 
                            for(uint32_t i  = 0;i < net_options.workers; i ++){
                                auto worker = std::make_shared<Worker>(); 
                                worker->start(); 
                                user_workers.push_back(worker); 
                            }
                        }
                        ilog("start listen on {}:{} ", url_info.host, url_info.port);
						asio::ip::tcp::endpoint endpoint(asio::ip::make_address(url_info.host),url_info.port  );

						// this->tcp_acceptor.open(asio::ip::tcp::v4());
						this->tcp_acceptor->open(endpoint.protocol());
						if (tcp_acceptor->is_open())
						{
							this->tcp_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));;
							 
							asio::error_code ec;
							this->tcp_acceptor->bind(endpoint, ec);
							if (ec) {
								elog("bind address failed {}:{}", url_info.host, url_info.port);
								is_running = false;
								return false;
							}
							this->tcp_acceptor->listen(net_options.backlogs, ec);

							if (ec) {
								elog("start listen failed {}:{}", url_info.host, url_info.port);
								is_running = false;
								return false;
							}

					 		if (net_options.sync_accept_threads > 0 ){
								for(uint32_t i = 0; i < net_options.sync_accept_threads; i ++){
									accept_threads.emplace_back([this](){
										this->do_sync_accept(); 
									});
								}								
								return true; 
							}							
							return this->do_accept();
						}
						else
						{
							return false;
						}
					}
					return true;
				}

				bool start(uint16_t port, NetOptions opts ={}, void * ssl = nullptr){
					return start(KNetUrl{"tcp","0.0.0.0",port},opts, ssl ); 
				}
		 
				bool start(const std::string & url , NetOptions opts ={},void *ssl = nullptr) {					
					return start(KNetUrl{url}, opts,  ssl);
				}

				void stop() {
					dlog("stop listener thread");
					if (is_running)
					{
						is_running = false;
						tcp_acceptor->close();
						for(auto & worker : user_workers){
							worker->stop(); 
						}
						user_workers.clear();
						listen_worker->stop();
					}
				}

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

				void add_net_handler(KNetHandler<T> *handler)
				{
					if (handler)
					{
						net_handler_chain.push_back(handler);
					}
				}
				void push_front(KNetHandler<T> *handler){
					if (handler)
					{
						auto beg = net_handler_chain.begin(); 
						net_handler_chain.insert(beg, handler);
					}
				}

				void push_back(KNetHandler<T> *handler){
					if (handler)
					{
						net_handler_chain.push_back(handler);
					}
				}

                uint64_t start_timer(const knet::utils::Timer::TimerHandler &  handler, uint64_t interval, bool bLoop = true){
                    if (listen_worker){
                        return listen_worker->start_timer(handler, interval, bLoop); 
                    }
                    return 0; 
                }
			private:
				inline void add_factory_event_handler(std::true_type, FactoryPtr fac)
				{
					auto evtHandler = dynamic_cast<KNetHandler<T> *>(fac);
					if (evtHandler)
					{
						add_net_handler(evtHandler);
					}
				}

				inline void add_factory_event_handler(std::false_type, FactoryPtr fac)
				{
				}

				bool invoke_data_chain(const TPtr &  conn, char * data, uint32_t dataLen)
				{
					bool ret = true;
					for (auto &handler : net_handler_chain)
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
					for (auto &handler : net_handler_chain)
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
					if (net_factory != nullptr) {
						asio::post(listen_worker->context(), [this, conn]() {
                                    if (net_factory) {
                                        net_factory->release(conn);
                                    }
								});
					}
				}

				bool do_sync_accept(){ 

					while(is_running){  
							auto worker = this->get_worker();
							if (!worker) {
								elog("no event worker, can't start tcp listener");
								return false ;
							} 

							asio::error_code ec;
							asio::ip::tcp::endpoint remoteAddr  ;
							asio::ip::tcp::socket sock(tcp_acceptor->accept(worker->context(), remoteAddr, ec));
					 
							if (!ec) {
								auto socket = std::make_shared<Socket>(std::move(sock),   worker->thread_id(), worker, ssl_context);
							 
								pthread_t curTid = pthread_self();				
								ilog("accept new connection from {}:{}, tid {}", remoteAddr.address().to_string(), remoteAddr.port(), curTid);

								socket->socket().set_option( asio::ip::tcp::no_delay(true));
								asio::socket_base::send_buffer_size SNDBUF(net_options.send_buffer_size);
								socket->socket().set_option(SNDBUF);
								asio::socket_base::receive_buffer_size RCVBUF(net_options.recv_buffer_size);
								socket->socket().set_option(RCVBUF);

								this->init_conn(worker, socket);
							}else {											// An error occurred.
								elog("get remote address failed: {}:{}", ec.value(), ec.message());
								sock.close();
							}
						}
					return true; 
				}

				bool do_accept()
				{
					auto worker = this->get_worker();
					if (!worker) {
						elog("no event worker, can't start");
						return false ;
					}

					auto socket = std::make_shared<Socket>(worker->thread_id(), worker, ssl_context);
					tcp_acceptor->async_accept(socket->socket(), [this, socket, worker](std::error_code ec) {
                                if (!ec) {
                                    socket->socket().set_option(asio::ip::tcp::no_delay(true));
								asio::socket_base::send_buffer_size SNDBUF(net_options.send_buffer_size);
								socket->socket().set_option(SNDBUF);
								asio::socket_base::receive_buffer_size RCVBUF(net_options.recv_buffer_size);
								socket->socket().set_option(RCVBUF);

								asio::error_code ec;
								asio::ip::tcp::endpoint remoteAddr = socket->socket().remote_endpoint(ec);
                                if (!ec) {																
                                    ilog("accept new connection from {}:{} ", remoteAddr.address().to_string(), remoteAddr.port());
                                    this->init_conn(worker, socket);
                                }else {											// An error occurred.
                                    elog("get remote address failed: {}:{}", ec.value(), ec.message());
                                    socket->socket().close();
                                }
                                    do_accept();
                                } else {
								elog("accept error: {},{}",ec.value(), ec.message());
								if (ec.value() == 24 ){  //too many open files
									do_accept();
								}
                                }
							});
                    return true; 
				}

				WorkerPtr get_worker()
				{
					if (!user_workers.empty())
					{
                        auto workeIdx = (worker_index++) % user_workers.size(); 
						//knet_tlog("dispatch to worker {}/{}", workeIdx,user_workers.size());
						return user_workers[workeIdx];
					}
					else
					{
						//dlog("dispatch work to listen worker {}", std::this_thread::get_id());
						return listen_worker;
					}
				}

				void init_conn(WorkerPtr worker, SocketPtr socket)
				{
					if (worker)
					{
						asio::dispatch(worker->context(), [=]() {
								auto conn = create_connection(socket, worker);				 
                                    if (!this->net_options.sync ) {
                                        socket->init_read(true);
                                    }
								});
					}
				}

				TPtr create_connection(SocketPtr sock, WorkerPtr worker)
				{
					TPtr  conn = nullptr; 
					if (net_factory != nullptr) {
						conn = net_factory->create();						
					} else {
						conn = std::make_shared<T>(); 						
					}
					conn->init(net_options, sock, worker, this);
					return conn; 
				}

			
				uint32_t worker_index = 0;
				std::vector<WorkerPtr> user_workers;
				KNetUrl url_info; 
				NetOptions net_options;
				bool is_running = false;
				void *ssl_context = nullptr;
				FactoryPtr net_factory = nullptr;
				std::vector<KNetHandler<T> *> net_handler_chain;
				WorkerPtr listen_worker;	
				std::vector<std::thread> accept_threads; 
				std::shared_ptr<asio::ip::tcp::acceptor> tcp_acceptor;
				
		};

		template <typename T>
			using DefaultTcpListener = TcpListener<T, KNetFactory<T>, KNetWorker>;

	} // namespace tcp

} // namespace knet
