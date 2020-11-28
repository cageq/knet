
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include <unordered_map>
#include "kcp/kcp_connection.hpp"


namespace knet {
	namespace kcp {

		template <typename T, typename Worker = EventWorker, typename ... Args>
		class KcpListener {
		public:
			using TPtr = std::shared_ptr<T>;
			using EventHandler = std::function<TPtr(TPtr, NetEvent, const std::string&)>;
			using WorkerPtr = std::shared_ptr<Worker>;
			KcpListener(WorkerPtr w = nullptr, Args ... args) :conn_args(args...) {
				auto worker = w ; 
				if (worker == nullptr) {
					m.default_worker = std::make_shared<Worker>();
					m.default_worker->start(); 
					worker = m.default_worker; 
				}  
				m.user_workers.emplace_back(worker); 
			}

			bool start(uint32_t port, EventHandler evtHandler = nullptr) {
				listen_port = port;
				event_handler = evtHandler;
				run();
				return true;
			}
			void add_worker(WorkerPtr w  ){
				if (w != nullptr)
				{
					m.user_workers.emplace_back(w); 
				} 
			}

			TPtr find_connection(udp::endpoint pt) {
				auto itr = connections.find(addrstr(pt));
				if (itr != connections.end()) {
					return itr->second;
				}
				return nullptr;
			}

			TPtr create_connection(udp::endpoint pt) {
				TPtr conn = nullptr;
				if (event_handler) {
					conn = event_handler(nullptr, EVT_CREATE, {});
				}
				if (!conn) {
					auto worker = this->get_worker(); 
					conn = std::apply(&KcpListener<T, Worker, Args...>::create_helper, conn_args);
					conn->init(worker);
				}
				conn->remote_point = pt;
				conn->event_handler = event_handler;
				connections[addrstr(pt)] = conn;
				return conn;
			}

			bool destroy_connection(TPtr conn) {
				auto itr = connections.find(conn->cid);
				if (itr != connections.end()) {
					connections.erase(itr);
					return true;
				}
				return false;
			}

			void stop() {
				if (server_socket)
				{
					server_socket->close();
				}
				m.user_workers.clear(); 
			}


		private:
			static TPtr create_helper(Args... args)
			{
				return  std::make_shared<T>(args...);
			}
			void do_receive() {
				server_socket->async_receive_from(asio::buffer(recv_buffer, max_length), remote_point,
					[this](std::error_code ec, std::size_t bytes_recvd) {
						if (!ec && bytes_recvd > 0) {
							dlog("received data {} from {}:{}", bytes_recvd,
								remote_point.address().to_string(), remote_point.port());

							auto conn = this->find_connection(this->remote_point);
							if (!conn) {
								conn = this->create_connection(remote_point);
								conn->listen(server_socket);
							}

							auto timeNow = std::chrono::system_clock::now();
							conn->last_msg_time = std::chrono::duration_cast<std::chrono::milliseconds>(
								timeNow.time_since_epoch());

							if (!conn->check_control_message((const char*)recv_buffer, bytes_recvd)) {
								conn->handle_receive((const char*)recv_buffer, bytes_recvd);
							}

							do_receive();
						}
						else {
							elog("receive error");
						}
					});
			}


			void run() {
				for(auto worker  :  m.user_workers){

					worker->post([this,worker]() {
					dlog("start kcp server @ {}", listen_port);
						server_socket = std::make_shared<udp::socket>(worker->context(), udp::endpoint(udp::v4(), listen_port));
						do_receive();
					});
				}
		
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
					dlog("dispatch work  to listen worker {}", std::this_thread::get_id());
					return m.default_worker;
				}
			}

			enum { max_length = 4096 };
			char recv_buffer[max_length];
		 
			uint32_t listen_port;
			udp::endpoint remote_point;

			std::shared_ptr<udp::socket> server_socket;
			std::unordered_map<std::string, TPtr> connections;
			EventHandler event_handler;
			std::tuple<Args...> conn_args; 

			struct
			{
				WorkerPtr default_worker; 
				uint32_t worker_index = 0;
				std::vector<WorkerPtr> user_workers;
			}m; 
 
		};

	} // namespace udp
} // namespace knet
