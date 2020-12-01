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

		template <class Worker = knet::EventWorker>
			class KPipe : public ConnectionFactory<PipeConnection> {
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

					void start(const std::string& host = "0.0.0.0", uint16_t port = 9999) {

						if ((pipe_mode & PIPE_SERVER_MODE) && port != 0) {
							dlog("start pipe on server mode {}", port);
							listener->start(port);
						}

						if (pipe_mode & PIPE_CLIENT_MODE) {
							dlog("start pipe on client mode {}", port);
							connector->start();
							connect();
						}
						is_started = true;
					}

					void attach(PipeSessionPtr pipe, const std::string& host = "", uint16_t port = 0) {
						if (pipe) {
							dlog("bind pipe host {} port {} pipe id {}", host, port, pipe->pipeid);
							pipe_factory.pipes[pipe->pipeid] = pipe;

							if (!host.empty()) {
								pipe->host = host;
							}
							if (port != 0) {
								pipe->port = port;

								if (is_started) {
									
									auto conn = connector->add_connection({pipe->host, pipe->port}, pipe->pipeid);
									conn->enable_reconnect();
								}
							}
						}
					}

					void stop() {}

					PipeSessionPtr find(const std::string& pid) { return pipe_factory.find(pid); }

					virtual void destroy(TPtr conn) {
						dlog("connection factory destroy connection in my factory ");
					}

					void broadcast(const char* pData, uint32_t len) { pipe_factory.broadcast(pData, len); }

					void broadcast(const std::string & msg) { pipe_factory.broadcast(msg.data(), msg.length()); }

				private:
					void connect() {
						dlog("start pipe client connection {}", pipe_factory.pipes.size()); 
						if (pipe_mode & PIPE_CLIENT_MODE) {
							for (auto& item : pipe_factory.pipes) {
								auto& pipe = item.second;
								if (pipe && pipe->port != 0 && !pipe->host.empty()) {
									dlog("start connect pipe to {}:{}", pipe->host, pipe->port);


									auto conn = connector->add_connection({pipe->host, pipe->port}, pipe->pipeid);
									conn->enable_reconnect();
								}
							}
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
