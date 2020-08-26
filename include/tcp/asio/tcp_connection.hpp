//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include <vector>
#include <unordered_map>
#include "tcp_socket.hpp"
#include "tcp_factory.hpp"
#include "event_worker.hpp"
using asio::ip::tcp;

namespace knet {
namespace tcp {

template <class T, class Sock = Socket<T>>
class TcpConnection : public std::enable_shared_from_this<T> {
public:
	// template <class>
	// friend class template.Sock;
	template <class, class, class, class...>
	friend class Listener;
	template <class, class, class>
	friend class Connector;
	using ConnSock = Sock;

	using NetEventHandler = std::function<void(std::shared_ptr<T>, NetEvent)>;
	using SelfNetEventHandler = void (T::*)(std::shared_ptr<T>, NetEvent);
	using DataHandler = std::function<uint32_t(const std::string&, MessageStatus)>;
	using SelfDataHandler = uint32_t (T::*)(const std::string&, MessageStatus);
	using SocketPtr = std::shared_ptr<Sock>;
	using ConnFactory = ConnectionFactory<T>;
	using FactoryPtr = ConnFactory*;

	using TimerHandler = std::function<void()>;
	struct TimerItem {
		TimerItem(asio::io_context& ctx)
			: timer(ctx) {}
		asio::steady_timer timer;
		TimerHandler handler;
		uint64_t interval;
		bool alive = false;
		uint64_t id;
		bool loop = true;
	};
	using TimerItemPtr = std::shared_ptr<TimerItem>;

	TcpConnection() {}


	TcpConnection(const std::string & host, uint32_t port ) {
		remote_host = host; 
		remote_port = port; 
	}

	virtual ~TcpConnection() {
		for (auto& item : timers) {
			item.second->alive = false;
			item.second->timer.cancel();
		}
		timers.clear();
	}

	void init(FactoryPtr fac = nullptr, SocketPtr sock = nullptr) {
		static uint64_t index = 1024;
		this->factory = fac;
		cid = ++index;
		std::shared_ptr<T> self = this->shared_from_this();
		socket = sock;
		socket->connection = self;
		handle_event(EVT_CREATE);
	}

	int send(const char* pData, uint32_t dataLen) { return msend(std::string(pData, dataLen)); }

	int send(const std::string& msg) { return msend(msg); }

 

	template <class P, class... Args>
	int msend(const P& first, const Args&... rest) {
		return socket->msend(first, rest...);
	}

	void close() {
		reconn_flag = false;
		socket->close();
	}
	void post(std::function<void()> handler) {
		if (socket) {
			socket->run_inloop(handler);
		} else {
			elog("socket is invalid");
		}
	}

	void bind_data_handler(DataHandler handler) { data_handler = handler; }
	void bind_data_handler(SelfDataHandler handler) {
		T* child = static_cast<T*>(this);
		data_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
	}

	void bind_event_handler(NetEventHandler handler) { event_handler = handler; }

	void bind_event_handler(SelfNetEventHandler handler) {
		T* child = static_cast<T*>(this);
		event_handler = std::bind(handler, child, std::placeholders::_1, std::placeholders::_2);
	}

	bool is_connected() { return socket && socket->is_open(); }

	bool connect(const std::string& host, uint32_t port) {
		dlog("start to connect {}:{}", host, port);
		this->remote_host = host;
		this->remote_port = port;
		return socket->connect(host, port);
	}

	bool vsend(const std::vector<asio::const_buffer>& bufs) { return socket->vsend(bufs); }

	bool connect() { return socket->connect(remote_host, remote_port); }

	utils::Endpoint local_endpoint() { return socket->local_endpoint(); }

	utils::Endpoint remote_endpoint() { return socket->remote_endpoint(); }

	std::string get_remote_ip() { return socket->remote_endpoint().host; }

	uint32_t get_remote_port() { return socket->remote_endpoint().port; }

	uint64_t get_cid() { return cid; }

	bool passive() { return is_passive; }

	void enable_reconnect(uint32_t interval = 1000000) {
		if (!reconn_flag) {
			reconn_flag = true;
			dlog("start reconnect timer ");
			auto self = this->shared_from_this();
			this->timerid = this->start_timer(
				[=]() {
					if (!is_connected()) {
						dlog("try to reconnect to server ");
						self->connect();
					}
				},
				interval);
		}
	}

	void disable_reconnect() {
		reconn_flag = false;
		if (this->timerid > 0) {
			this->stop_timer(this->timerid);
			this->timerid = 0;
		}
	}

	template <class FPtr>
	FPtr get_factory() {
		return std::static_pointer_cast<FPtr>(factory);
	}

	void set_worker(EventWorkerPtr w) { conn_worker = w; }

	EventWorkerPtr get_worker() { return conn_worker; }

	asio::io_context* get_context() {
		if (socket) {
			return &socket->context();
		}
		return nullptr;
	}
	void* user_data = nullptr;
	uint64_t cid = 0;

public:
	bool reconn_flag = false;
	uint64_t timerid = 0;

	void destroy() {
		if (destroyer) {
			destroyer(this->shared_from_this());
		}
	}

	std::function<void(std::shared_ptr<T>)> destroyer;
	virtual uint32_t handle_data(const std::string& msg, MessageStatus status)  {
		if (data_handler) {
			return data_handler(msg, status);
		}

		if (factory) {
			return factory->handle_data(this->shared_from_this(), msg, status);
		}
		return msg.length();
	}

	void handle_timeout(TimerItemPtr item)  { 
		 
		if (item->alive) {

			if (item->handler) {
				item->handler();
			}
			if (item->loop) {
				item->timer.expires_after(asio::chrono::microseconds(item->interval));
				item->timer.async_wait(std::bind(&TcpConnection<T>::handle_timeout, this, item));
			}

		} else {
			timers.erase(item->id);
		}
	}

	uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true) {

		static uint64_t timer_index = 1;
		if (socket) {
			TimerItemPtr item = std::make_shared<TimerItem>(socket->context());
			item->handler = handler;
			item->interval = interval;
			item->alive = true;
			item->id = timer_index++;
			item->loop = bLoop;

			asio::dispatch(socket->context(), [this, item]() { timers[item->id] = item; });
			item->timer.expires_after(asio::chrono::microseconds(interval));
			item->timer.async_wait(std::bind(&TcpConnection<T>::handle_timeout, this, item));
			dlog("start timer {} ", item->id);
			return item->id;
		}
		return 0;
	}

	bool stop_timer(uint64_t timerId) {
		dlog("stop timer {} ", timerId);
		if (socket) {
			asio::dispatch(socket->context(), [this, timerId]() {
				auto itr = timers.find(timerId);
				if (itr != timers.end()) {
					itr->second->alive = false;
					itr->second->timer.cancel();
				}
			});
		}

		return true;
	}

	virtual void handle_event(NetEvent evt)   {

		dlog("handle event in connection {}", evt);
		if (event_handler) {
			event_handler(this->shared_from_this(), evt);
		}

		if (factory) {
			factory->handle_event(this->shared_from_this(), evt);
		}
	}

	friend class ConnectionFactory<T>;

	static std::shared_ptr<T> create(SocketPtr sock) {
		auto self = std::make_shared<T>();
		self->init(self->factory, sock);
		return self;
	}
	void set_remote_addr(const std::string & host, uint32_t port ){
		remote_host = host; 
		remote_port = port; 
	}
	inline std::string  get_remote_host() const {
		return remote_host; 
	}
	inline uint32_t get_remote_port() const { 
		return remote_port; 
	}


	bool is_passive = true;
protected:
	std::unordered_map<uint64_t, TimerItemPtr> timers;
	FactoryPtr factory = nullptr;
	SocketPtr socket;
	NetEventHandler event_handler;
	DataHandler data_handler;

	std::string remote_host;
	uint32_t remote_port;
	EventWorkerPtr conn_worker;
};

} // namespace tcp
} // namespace knet
