//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include "tcp_connection.hpp"
#include <tuple>
#include <memory>

namespace knet {
namespace tcp {
using asio::ip::tcp;
template <class T, class F = ConnectionFactory<T>, class Worker = EventWorker, class... Args>
class TcpListener final {
public:
	using TPtr = std::shared_ptr<T>;
	using Factory = F;
	using FactoryPtr = F*;
	using WorkerPtr = std::shared_ptr<Worker>;
	using SocketPtr = std::shared_ptr<typename T::ConnSock>;

	TcpListener(FactoryPtr fac, uint32_t num, WorkerPtr lisWorker = std::make_shared<Worker>())
		: m_factory(fac)
		, listen_worker(lisWorker) {
		if (!listen_worker) {
			elog("can't live without listen worker, fail to start ");
			return;
		}
		listen_worker->start();

		tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(lisWorker->context());
		for (uint32_t i = 0; i < num; i++) {
			user_workers.push_back(std::make_shared<Worker>());
		}
	}

	TcpListener(FactoryPtr fac, std::vector<WorkerPtr> workers,
		WorkerPtr lisWorker = std::make_shared<Worker>())
		: m_factory(fac)
		, listen_worker(lisWorker) {
		if (!listen_worker) {
			elog("can't live without listen worker");
		}
		listen_worker->start();
		dlog("start listener in one worker");
		tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(lisWorker->context());
		if (!workers.empty()) {
			for (auto worker : workers) {
				user_workers.push_back(worker);
			}
		}
	}

	TcpListener(
		FactoryPtr fac = nullptr, WorkerPtr lisWorker = std::make_shared<Worker>(), Args... args)
		: m_factory(fac)
		, listen_worker(lisWorker)
		, conn_args(args...) {
		if (listen_worker) {
			listen_worker->start();
			tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(listen_worker->context());
		} else {
			elog("can't live without listen worker");
		}
	}

	TcpListener(WorkerPtr lisWorker, Args... args)
		: m_factory(nullptr)
		, listen_worker(lisWorker)
		, conn_args(args...) {
		if (listen_worker) {
			listen_worker->start();
			tcp_acceptor = std::make_shared<asio::ip::tcp::acceptor>(listen_worker->context());
		} else {
			elog("can't live without listen worker");
		}
	}

	void add_worker(WorkerPtr worker) {
		if (worker)
		{
			user_workers.push_back(worker); 
		} 
	}

	TcpListener(const TcpListener&) = delete;

	~TcpListener() {}

	bool start(NetOptions opt, void* sslCtx = nullptr) {
		m_options = opt;
		ssl_context = sslCtx;
		if (!is_running) {
			is_running = true;

			asio::ip::tcp::endpoint endpoint(asio::ip::make_address(opt.host), opt.port);
 
			// this->tcp_acceptor.open(asio::ip::tcp::v4());
			this->tcp_acceptor->open(endpoint.protocol());
			if (tcp_acceptor->is_open()) {
				this->tcp_acceptor->set_option(asio::socket_base::reuse_address(true));
				//	this->tcp_acceptor.set_option(asio::ip::tcp::no_delay(true));
				this->tcp_acceptor->non_blocking(true);

				asio::socket_base::send_buffer_size SNDBUF(m_options.send_buffer_size);
				this->tcp_acceptor->set_option(SNDBUF);
				asio::socket_base::receive_buffer_size RCVBUF(m_options.recv_buffer_size);
				this->tcp_acceptor->set_option(RCVBUF);

				asio::error_code ec;
				this->tcp_acceptor->bind(endpoint, ec);
				if (ec) {
					elog("bind address failed {}:{}", opt.host, opt.port);
					is_running = false;
					return false;
				}
				this->tcp_acceptor->listen(m_options.backlogs, ec);

				if (ec) {
					elog("start listen failed");
					is_running = false;
					return false;
				}
				this->do_accept();

			} else {
				return false;
			}
		}
		return true;
	}

	bool start(uint32_t port = 9999, const std::string& host = "0.0.0.0", void* ssl = nullptr) {
		m_options.host = host;
		m_options.port = port;
		return start(m_options, ssl);
	}

	void stop() {
		dlog("stop listener thread");
		if (is_running) {
			is_running = false;
			tcp_acceptor->close();
		}
	}

	void destroy(std::shared_ptr<T> conn) {
		asio::post(listen_worker->context(), [this, conn]() {
			if (m_factory) {
				m_factory->destroy(conn);
			}
		});
	}

private:
	void do_accept() {
		// dlog("accept new connection ");
		auto worker = this->get_worker();
		if (!worker) {
			elog("no event worker, can't start");
			return;
		}

		auto socket = std::make_shared<typename T::ConnSock>(
			worker->thread_id(), worker->context(), ssl_context);
		tcp_acceptor->async_accept(socket->socket(), [this, socket, worker](std::error_code ec) {
			if (!ec) {
				dlog("accept new connection ");
				this->init_conn(worker, socket);
				do_accept();
			} else {
				elog("accept error");
			}
		});
	}

	WorkerPtr get_worker() {
		if (!user_workers.empty()) {
			dlog("dispatch to worker {}", worker_index);
			return user_workers[worker_index++ % user_workers.size()];
		} else {
			dlog("dispatch work  to listen worker {}", std::this_thread::get_id());
			return listen_worker;
		}
	}

	void init_conn(WorkerPtr worker, std::shared_ptr<typename T::ConnSock> socket) {

		if (worker) {
			asio::dispatch(worker->context(), [=]() {
				auto conn = create_connection(socket);
				conn->set_worker(worker);
				conn->destroyer = std::bind(
					&TcpListener<T, F, Worker, Args...>::destroy, this, std::placeholders::_1);

				conn->handle_event(EVT_CONNECT);
				socket->do_read();
			});
		}
	}

	static TPtr factory_create_helper(FactoryPtr fac, Args... args) {
		if (fac) {
			wlog("create connection by factory");
			return fac->create(args...);
		} else {
			wlog("create connection by default");
			return std::make_shared<T>(args...);
		}
	}

	TPtr create_connection(SocketPtr sock) {

		auto conn = std::apply(&TcpListener<T, F, Worker, Args...>::factory_create_helper,
			std::tuple_cat(std::make_tuple(m_factory), conn_args));

		conn->init(m_factory, sock);
		return conn;
	}

	uint32_t worker_index = 0;
	std::vector<WorkerPtr> user_workers;
	FactoryPtr m_factory = nullptr;

	WorkerPtr listen_worker;
	NetOptions m_options;
	bool is_running = false;
	std::shared_ptr<asio::ip::tcp::acceptor> tcp_acceptor;
	std::tuple<Args...> conn_args;
	void* ssl_context = nullptr;
};

} // namespace tcp

} // namespace gnet
