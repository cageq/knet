#include "klog.hpp"
#include "knet.hpp"
#include "http_request.hpp"
#include "http_factory.hpp"

namespace knet {
namespace http {
template <class Worker = knet::EventWorker, class Factory = HttpFactory< HttpConnection> >
class HttpServer  {
public:
	using WorkerPtr = std::shared_ptr<Worker>;
	HttpServer(Factory* fac = nullptr, WorkerPtr lisWorker = std::make_shared<Worker>(), uint32_t workerNum = 4 )
		: http_factory(fac == nullptr?&default_factory:fac)
		, http_listener(http_factory   , lisWorker) {

			if (lisWorker == nullptr) {
				lisWorker = std::make_shared<Worker>(); 
			}
			http_listener.add_worker(lisWorker); // also as default worker

		for (uint32_t i = 0;i < workerNum ; i++){
			add_worker(); 
		}
	}

	void add_worker(WorkerPtr worker = nullptr ) { 
		if (worker) {
			http_listener.add_worker(worker);
		} else {
			auto worker = std::make_shared<Worker>(); 
			worker->start(); 
			http_listener.add_worker(worker);
		}
   
	}

	bool start(uint32_t port = 8888, const std::string& host = "0.0.0.0") {

		dlog("start http server :{} , thread id {}", port, std::this_thread::get_id());
		http_listener.start(port, host);
		return true;
	}

	void register_router(const std::string& path, HttpHandler handler) {
		dlog("register path {}", path);
		if (http_factory) {
			http_factory->http_routers[path] = handler;
		}  
	}

private:
	// HttpRouteMap http_routers;
	Factory default_factory ; 
	Factory* http_factory = nullptr;

	TcpListener<HttpConnection, Factory, Worker> http_listener;
};

} // namespace http

} // namespace knet
