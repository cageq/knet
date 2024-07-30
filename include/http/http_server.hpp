#pragma once

#include "knet.hpp"
#include "http_request.hpp"
#include "http_factory.hpp"

namespace knet
{
	namespace http
	{
		template <class Worker = knet::KNetWorker, class Factory = HttpFactory<HttpConnection>>
		class HttpServer : public HttpFactory<HttpConnection>
		{
		public:
			using WorkerPtr = std::shared_ptr<Worker>;

			// it's easy to use raw pointer of Factory
			HttpServer(Factory *fac = nullptr, WorkerPtr lisWorker = nullptr, uint32_t workerNum = 4)
				: http_factory(fac == nullptr ? this : fac), http_listener(http_factory, lisWorker)
			{
				for (uint32_t i = 0; i < workerNum; i++)
				{
					add_worker();
				}
			}

			void add_worker(WorkerPtr worker = nullptr)
			{
				if (worker)
				{
					http_listener.add_worker(worker);
				}
				else
				{
					auto worker = std::make_shared<Worker>();
					worker->start();
					http_listener.add_worker(worker);
				}
			}

			bool start(uint16_t port = 8888, const std::string &host = "0.0.0.0", const NetOptions & netOpts = {})
			{
				knet_dlog("start http server {}:{}", host, port);
				http_listener.start({"tcp", host, port}, netOpts);
				return true;
			}
			void stop()
			{
				http_listener.stop();
			}

		private:
			Factory *http_factory = nullptr;
			TcpListener<HttpConnection, HttpFactory, Worker> http_listener;
		};

	} // namespace http

} // namespace knet
