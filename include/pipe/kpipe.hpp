#pragma once
#include <unordered_map>
#include <memory>

#include "knet.hpp"
#include "pipe_connection.hpp"
#include "pipe_session.hpp"
#include "pipe_factory.hpp"

using namespace knet::tcp;
namespace knet {
	namespace pipe{

		template <class Worker = knet::KNetWorker>
			class KPipe : public KNetFactory<PipeConnection> {
				public:
					using PipeListener = TcpListener<PipeConnection, PipeFactory, Worker>;
					using PipeConnector = TcpConnector<PipeConnection, PipeFactory, Worker>;
					using WorkerPtr = std::shared_ptr<Worker>;
					KPipe(PipeMode mode = PipeMode::PIPE_DUET_MODE, WorkerPtr worker = std::make_shared<Worker>())
						: pipe_mode(mode),pipe_factory(mode) {

							if (!worker) {
								worker = std::make_shared<Worker>();
							}
							pipe_worker = worker;
							pipe_worker->start(); 

							if (pipe_mode & PIPE_SERVER_MODE) {
								listener = std::make_shared<PipeListener>(&pipe_factory, worker);						
							}

							if (pipe_mode & PIPE_CLIENT_MODE) {
								connector = std::make_shared<PipeConnector>(&pipe_factory, worker);
							}
						}

			virtual ~KPipe() {}

					bool start(const std::string& host = "0.0.0.0", uint16_t port = 9999) {

						if ((pipe_mode & PIPE_SERVER_MODE) && port > 0) {
							knet_dlog("start pipe on server mode {}", port);
							bool ret = listener->start(port);
							if (!ret)
							{
								knet_elog("start kpipe listener failed");
								return false;
							}
						}

						if (pipe_mode & PIPE_CLIENT_MODE) {
							knet_dlog("start pipe on client mode {}", port);
							bool ret = connector->start();
							if (!ret)
							{
								knet_elog("start kpipe connector failed");
								return false;
							}
							connect();
						}
						is_started = true;
						return is_started; 
					}

					void attach(PipeSessionPtr pipe, const std::string& host = "", uint16_t port = 0) {
						if (pipe) {
							knet_dlog("bind pipe host {} port {} pipe id {}", host, port, pipe->get_pipeid());
							pipe_factory.register_pipe(pipe);

							if (!host.empty()) {
								pipe->set_host(host);  
							}
							if (port != 0) { 
								pipe->set_port(port);  
								if (is_started) { 
									if (pipe_mode & PIPE_CLIENT_MODE) {
										auto conn = connector->add_connection({"tcp",pipe->get_host(), pipe->get_port()}, pipe->get_pipeid());
										conn->enable_reconnect();
									}									
								}
							}
							pipe_worker->post([=](){								 
								pipe->handle_event(NetEvent::EVT_THREAD_INIT); 
							}); 
						}
					}

					PipeSessionPtr attach(const std::string& host = "", uint16_t port = 0) { 
						auto pipe = std::make_shared<PipeSession>(); 
						auto conn = connector->add_connection({"tcp",host, port});
						conn->enable_reconnect(); 
						knet_dlog("register unbind pipe {}",conn->get_cid()); 
						pipe_factory.register_pipe(pipe,conn->get_cid());
						pipe_worker->post([=](){								 
								pipe->handle_event(NetEvent::EVT_THREAD_INIT); 
						}); 
						return pipe; 
					}

					void stop() {
					
						if (pipe_worker) {
							pipe_worker->stop(); 
						}

						if (listener ) {
							listener->stop(); 
						}
						if (connector) {
							connector->stop(); 
						}
					}

					PipeSessionPtr find(const std::string& pid) { return pipe_factory.find_bind_pipe(pid); }

					virtual void destroy(TPtr conn) {
						knet_dlog("connection factory destroy connection in my factory ");
					}


					template <class P, class... Args>
					inline int32_t broadcast(const P &first, const Args &...rest){
						 return pipe_factory.broadcast(first, rest...);
					}

					
					template <class P, class... Args>
					inline int32_t send_to(const std::string & pipeId, const P &first, const Args &...rest){
						return pipe_factory.send_to(pipeId, first, rest...); 
					} 

				private:
					void connect() {
						knet_dlog("start pipe client connection " ); 
						if (pipe_mode & PIPE_CLIENT_MODE) {

							pipe_factory.start_clients([this](PipeSessionPtr pipe){ 
								auto conn = connector->add_connection({"tcp",pipe->get_host(), pipe->get_port()}, pipe->get_pipeid());
								conn->enable_reconnect();
							});  
				 
						}
					}


					PipeMode pipe_mode;
					PipeFactory pipe_factory;
					KPipe::WorkerPtr pipe_worker;

					bool is_started = false;
					std::shared_ptr<PipeListener> listener;
					std::shared_ptr<PipeConnector> connector;
			};


	}
}
