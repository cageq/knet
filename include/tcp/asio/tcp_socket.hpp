//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once


#include <memory>
#include <string>
#include <chrono>
#include <string_view> 
#include "utils.hpp"
#include "loop_buffer.hpp"

#define WITH_PACKAGE_HANDLER 0
using namespace knet::utils;
namespace knet {
namespace tcp {

using asio::ip::tcp;
 

template <class T>
class Socket : public std::enable_shared_from_this<Socket<T>>  {
public:
	enum class SocketStatus {
		SOCKET_IDLE,
		SOCKET_INIT = 1,
		SOCKET_OPEN,
		SOCKET_CLOSING,
		SOCKET_RECONNECT,
		SOCKET_CLOSED,
	};
	Socket(const std::thread::id& tid, asio::io_context& ctx, void * = nullptr)
		: io_context(ctx)
		, sock(ctx) {
		worker_tid = tid;
		m_status = SocketStatus::SOCKET_INIT;
		//send_buffer.reserve(4096);
	}

	~Socket() {}

	template <class,class>
	friend class Connection;

	bool connect(const std::string& host, uint32_t port) {
		tcp::resolver resolver(io_context);
		auto result = resolver.resolve(host, std::to_string(port));
		dlog("connect to server {}:{}", host.c_str(), port);
		auto self = this->shared_from_this();
		async_connect(sock, result,
			[self](asio::error_code ec, typename decltype(result)::endpoint_type endpoint) {
				if (!ec) {

					if (self->connection) {
						ilog("on connection connected");
						self->connection->handle_event(EVT_CONNECT);
					} else {
						wlog("no connection");
					}
					self->do_read();
				} else {
					dlog("connect failed");
					self->sock.close();
				}
			});
		return true;
	}

	template <class F>
	void run_inloop(F fn) {
		asio::dispatch(io_context, fn);
	}

	void do_read() {
		if (sock.is_open()) {

			m_status = SocketStatus::SOCKET_OPEN;

			auto self = this->shared_from_this();
			auto buf = asio::buffer((char*)read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos);
			// dlog("left buffer size {}", kReadBufferSize - read_buffer_pos);
			sock.async_read_some(
				buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
					if (!ec) {
						// dlog("received data {}", bytes_transferred);
						process_data(bytes_transferred);
						//dlog("read data: {} length {}", read_buffer, bytes_transferred);
						self->do_read();
					} else {
						dlog("read error ,close connection {} ", ec.value());
						do_close();
					}
				});
		} else {
			dlog("socket is not open");
		}
	}

	int send_inloop(const char* pData, uint32_t dataLen) {

		asio::async_write(
			sock, asio::buffer(pData, dataLen), [&](std::error_code ec, std::size_t length) {
				if (!ec) {
					if (length < dataLen) {
						send_inloop(pData + length, dataLen - length);
					}
				} else {
					this->do_close();
				}
			});
		return 0;
	}
  

	int send(const char* pData, uint32_t dataLen) {

		if (is_inloop()) {
			return send_inloop(pData, dataLen);
		}
		return msend(std::string(pData, dataLen));
	}

	int send(const std::string& msg) {

		if (is_inloop()) {
			return send_inloop(msg.data(), msg.length());
		}

		return msend(std::string_view(msg.data(), msg.length()));
	}

	template <class P, class... Args>
	int msend(const P& first, const Args&... rest) {
		if (sock.is_open()) {
			if (!first.empty()) {
				m_mutex.lock();
				this->mpush(first, rest...);
				m_mutex.unlock();
			}
			return 0;
		}
		return -1;
	}

	template <class... Args>
	void mpush(const std::string& first, Args... rest) { 
		
		std::ostream outbuf (&send_buffer); 
		outbuf << first; 
		mpush(rest...);
	}

	template <class... Args>
	void mpush(const std::string_view& first, Args... rest) {
		std::ostream outbuf (&send_buffer); 
		outbuf << first; 
		mpush(rest...);
	}

	void mpush() {
		auto self = this->shared_from_this();
		asio::dispatch(io_context, [this, self]() {
			if (sock.is_open()) {							

				if (!is_writing) {
					self->do_async_write();
				}
			}
		});
	}

	bool do_async_write() {
		if (send_buffer.size() > 0 ) {  
	 		is_writing = true; 
			ilog("send data length in single buffer {}", send_buffer.size());
			auto self = this->shared_from_this();			
			asio::async_write(sock, asio::buffer(send_buffer.data(), send_buffer.size()),
				[this, self](std::error_code ec, std::size_t length) {
					if (!ec && sock.is_open() && length > 0 ) {					
						connection->handle_event(EVT_SEND);
						{
							ilog("send out length {}",length); 
							std::lock_guard<std::mutex>  guard(m_mutex); 
							send_buffer.consume(length); 					 	
							if (send_buffer.size() == 0){
								is_writing = false;
								return ; 
							}			 								
						}		
						self->do_async_write();					
					} else {
						self->do_close();
					}
				});
		
		} else {
			is_writing = false;
		}
		return true;
	}
 

 
	bool vsend(const std::vector<asio::const_buffer>& bufs) {
		uint32_t totalSize = 0;
		for (auto& buf : bufs) {
			totalSize += buf.size();
		}
		auto self = this->shared_from_this();
		asio::async_write(
			sock, bufs, [this, self, totalSize](std::error_code ec, std::size_t length) {
				if (!ec && (sock.is_open())) {
					if (length < totalSize) {
						wlog("send data not finished,  total {}, sent {} ", totalSize, length);
						// self->batch_async_write();
					} else {
					}
				} else {
					self->do_close();
				}
			});

		return true;
	}
	
	bool is_open() {
		//		dlog("status is {}", static_cast<uint32_t>(m_status ));
		return sock.is_open() && m_status == SocketStatus::SOCKET_OPEN;
	}

#if (WITH_PACKAGE_HANDLER)
	void process_data(uint32_t nread) {
		if (!connection) {
			return;
		}

		this->connection->handle_event(EVT_RECV);
		//	this->read_buffer.resize(read_buffer.size() + nread);
		read_buffer_pos += nread;
		uint32_t readPos = 0;
		int32_t pkgLen = this->connection->handle_package(read_buffer, read_buffer_pos);
		dlog("process data  pkg size is {}", pkgLen);
		if (pkgLen < 0 || pkgLen > kReadBufferSize) {
			elog("single package size ({}) error, close connection", pkgLen);
			connection->close();
			return;
		}
		while (pkgLen > 0) {
			dlog("read package length {}", pkgLen);
			if (readPos + pkgLen <= read_buffer_pos) {
				char* pkgEnd = (char*)read_buffer + readPos + pkgLen + 1;
				char endChar = *pkgEnd;
				*pkgEnd = 0;
				this->connection->handle_data((char*)read_buffer + readPos, pkgLen);
				*pkgEnd = endChar;
				readPos += pkgLen;
			}

			if (readPos < read_buffer_pos) {
				pkgLen = this->connection->handle_package(
					(char*)read_buffer + readPos, read_buffer_pos - readPos);
				if (pkgLen <= 0) {
					elog("moving buffer to front {} ", read_buffer_pos - readPos);
					memmove(read_buffer, (char*)read_buffer + readPos, read_buffer_pos - readPos);
					read_buffer_pos -= readPos;
					// this->read_buffer.erase(read_buffer.begin(),read_buffer.begin()+ readPos);
					break;
				}
			} else {
				read_buffer_pos = 0;
				break;
			}
		}
	}

#else

	/*----------------------------------------------------------------
	enough     |  consume
	not enough |  consume
	not enough |  not consume
	----------------------------------------------------------------*/
	void process_data(uint32_t nread) {
		if (!connection) {
			return;
		}
		this->connection->handle_event(EVT_RECV);

		read_buffer_pos += nread;
		dlog("read data {} total is {}", nread, read_buffer_pos);
		uint32_t readPos = 0;
		uint32_t pkgLen = 0;

		do {

			uint32_t readLen = read_buffer_pos - readPos;
			dlog("need more data :{}", need_package_length);

			if (need_package_length > 0) {

				if (need_package_length <= readLen) {
					pkgLen = this->connection->handle_data(std::string((char*)read_buffer + readPos,
						need_package_length), MessageStatus::MESSAGE_END);
					dlog(" need length {} package len {} , data len is {} chunk", need_package_length, pkgLen, readLen);
					readPos += need_package_length;
					// if (readPos >= read_buffer_pos) {
					// 	read_buffer_pos = 0;
					// 	return;
					// }

					need_package_length = 0;

				} else {

					pkgLen = this->connection->handle_data(
						std::string((char*)read_buffer + readPos, readLen), MessageStatus::MESSAGE_CHUNK);

					dlog(" need length {} package len {} , data len is {} chunk",
						need_package_length, pkgLen, readLen);
					read_buffer_pos = 0;
					need_package_length -= readLen;
					return;
				}

			} else {
				pkgLen = this->connection->handle_data(
					std::string((char*)read_buffer + readPos, readLen), MessageStatus::MESSAGE_NONE);
				dlog(" need length {} package len {} , data len is {}", need_package_length, pkgLen,
					readLen);
				if (pkgLen > kMaxPackageLimit) {
					elog("max package limit error {}", pkgLen);
					this->close();
					return;
				}
				if (pkgLen == 0) { // no enough data 

					if (read_buffer_pos > readPos && readPos > 0) {
						memmove(read_buffer, (char*)read_buffer + readPos, read_buffer_pos - readPos);
					}
					read_buffer_pos -= readPos;
					return;
				} else {

					if ( pkgLen > readLen) { // exceed buffer size, but user has eat the buffer data
						need_package_length = pkgLen - readLen;
						//read_buffer_pos -= readLen;
						read_buffer_pos = 0; 
						return;
					} else { // buffer hold one or more package

						dlog("process just one message {}", pkgLen);
						readPos += pkgLen;
					}
				}
			}
			dlog("current read pos is {}", readPos);

		} while (read_buffer_pos > readPos);

		if (read_buffer_pos > readPos && readPos > 0) {
			memmove(read_buffer, (char*)read_buffer + readPos, read_buffer_pos - readPos);
			read_buffer_pos -= readPos;
		} else {
			read_buffer_pos = 0;
		}

		dlog("last read buffer pos {}", read_buffer_pos);
	}

#endif

	void close() { do_close(true); }
	void do_close(bool force = false) {

		if (m_status == SocketStatus::SOCKET_CLOSING || m_status == SocketStatus::SOCKET_CLOSED) {
			dlog("already in closing status {}", m_status);
			return;
		}

		if (force) {
			m_status = SocketStatus::SOCKET_CLOSING;
		}

		auto self = this->shared_from_this();
		if (send_buffer.size() > 0  &&  !is_writing ){
			do_async_write(); //try last write
		}
		asio::post(io_context, [self]() {
			elog("try to do close connection ...");


			if (self->connection) {
				self->connection->handle_event(EVT_DISCONNECT);
				self->read_buffer_pos = 0;
				if (self->connection->reconn_flag) {
					self->m_status = SocketStatus::SOCKET_RECONNECT;
				} else {
					self->m_status = SocketStatus::SOCKET_CLOSED;
					if (self->socket().is_open()) {
						self->socket().close();
					}
					self->connection->destroy();
					self->connection.reset();
				}

			} else {
				self->m_status = SocketStatus::SOCKET_CLOSED;
				if (self->socket().is_open()) {
					self->socket().close();
					self->read_buffer_pos = 0;
				}
			}
		});
	}

	utils::Endpoint local_endpoint() {
		utils::Endpoint endpoint;
		auto ep = sock.local_endpoint();
		endpoint.host = ep.address().to_string();
		endpoint.port = ep.port();
		return endpoint;
	}

	utils::Endpoint remote_endpoint() {
		utils::Endpoint endpoint;
		auto ep = sock.remote_endpoint();
		endpoint.host = ep.address().to_string();
		endpoint.port = ep.port();
		return endpoint;
	}
	inline tcp::socket& socket() { return sock; }

	std::shared_ptr<T> connection;
	bool is_inloop() { return worker_tid == std::this_thread::get_id(); }

	inline asio::io_context& context() { return io_context; }

private:
	enum { kReadBufferSize = 1024*8, kMaxPackageLimit = 1024 * 1024 };
	asio::io_context& io_context;
 
 
	bool is_writing = false;
	asio::streambuf send_buffer;  
 
	std::mutex m_mutex;

	tcp::socket sock;

	SocketStatus m_status = SocketStatus::SOCKET_IDLE;

	char read_buffer[kReadBufferSize];
	uint32_t read_buffer_pos = 0;
	std::thread::id worker_tid;
	uint32_t need_package_length = 0;
};

} // namespace tcp

}; // namespace knet
