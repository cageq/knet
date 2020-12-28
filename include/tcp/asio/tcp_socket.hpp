//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once


#include <memory>
#include <string>
#include <chrono>
 
 

#define WITH_PACKAGE_HANDLER 1

namespace knet {
namespace tcp {
 
using asio::ip::tcp;

template <class T>
class TcpSocket : public std::enable_shared_from_this<TcpSocket<T>>  {
public:
	enum class SocketStatus {
		SOCKET_IDLE,
		SOCKET_INIT = 1,
		SOCKET_OPEN,
		SOCKET_CLOSING,
		SOCKET_RECONNECT,
		SOCKET_CLOSED,
	};
	using TPtr = std::shared_ptr<T>; 
	TcpSocket(const std::thread::id& tid, asio::io_context& ctx, void * = nullptr)
		: io_context(ctx)
		, tcp_sock(ctx) {
		worker_tid = tid;
		m.status = SocketStatus::SOCKET_INIT;
		//m.send_buffer.reserve(4096);
	}

	~TcpSocket() {}

	template <class,class>
	friend class Connection;

	bool connect(const std::string& host, uint32_t port, const std::string & localAddr = "0.0.0.0", uint32_t localPort = 0 ) {
		tcp::resolver resolver(io_context);
		auto result = resolver.resolve(host, std::to_string(port));
		dlog("connect to server {}:{}", host.c_str(), port);
		auto self = this->shared_from_this();
		if (localPort > 0) {
			asio::ip::tcp::endpoint laddr(asio::ip::make_address(localAddr), localPort); 
			tcp_sock.bind(laddr); 
		}

		async_connect(tcp_sock, result,
			[self](asio::error_code ec, typename decltype(result)::endpoint_type endpoint) {
				if (!ec) {

					if (self->connection) {
						ilog("on connection connected");
						self->connection->process_event(EVT_CONNECT);
					} else {
						wlog("no connection");
					}
					self->do_read();
				} else {
					dlog("connect failed");
					self->tcp_sock.close();
				}
			});
		return true;
	}

	template <class F>
	void run_inloop(F fn) {
		asio::dispatch(io_context, fn);
	}

	void do_read() {
		if (tcp_sock.is_open()) {
			m.status = SocketStatus::SOCKET_OPEN;
			auto self = this->shared_from_this();
			auto buf = asio::buffer((char*)m.read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos);
			// dlog("left buffer size {}", kReadBufferSize - read_buffer_pos);
			tcp_sock.async_read_some(
				buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
					if (!ec) {
						// dlog("received data {}", bytes_transferred);
						process_data(bytes_transferred);
						//dlog("read data: {} length {}", m.read_buffer, bytes_transferred);
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
			tcp_sock, asio::buffer(pData, dataLen), [&](std::error_code ec, std::size_t length) {
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

		return msend(std::string(msg.data(), msg.length()));
	}

	template <class P, class... Args>
	int msend(const P& first, const Args&... rest) {
		if (tcp_sock.is_open()) {
			if (!first.empty()) {
				m.mutex.lock();
				this->mpush(first, rest...);
				m.mutex.unlock();
			}
			return 0;
		}
		return -1;
	}

	template <class... Args>
	void mpush(const std::string& first, Args... rest) { 		
		std::ostream outbuf (&m.send_buffer); 
		outbuf << first; 
		mpush(rest...);
	}

	// template <class... Args>
	// void mpush(const std::string_view& first, Args... rest) {
	// 	std::ostream outbuf (&m.send_buffer); 
	// 	outbuf << first; 
	// 	mpush(rest...);
	// }

	void mpush() {
		auto self = this->shared_from_this();
		asio::dispatch(io_context, [this, self]() {
			if (tcp_sock.is_open()) {	
				if (!m.is_writing) {
					self->do_async_write();
				}
			}
		});
	}

	bool do_async_write() {
		if (m.send_buffer.size() > 0 ) {  
	 		m.is_writing = true; 
			ilog("send data length in single buffer {}", m.send_buffer.size());
			auto self = this->shared_from_this();			
			asio::async_write(tcp_sock, asio::buffer(m.send_buffer.data(), m.send_buffer.size()),
				[this, self](std::error_code ec, std::size_t length) {
					if (!ec && tcp_sock.is_open() && length > 0 ) {					
						connection->process_event(EVT_SEND);
						{
							ilog("send out length {}",length); 
							std::lock_guard<std::mutex>  guard(m.mutex); 
							m.send_buffer.consume(length); 					 	
							if (m.send_buffer.size() == 0){
								m.is_writing = false;
								return ; 
							}			 								
						}		
						self->do_async_write();					
					} else {
						self->do_close();
					}
				});
		
		} else {
			m.is_writing = false;
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
			tcp_sock, bufs, [this, self, totalSize](std::error_code ec, std::size_t length) {
				if (!ec && (tcp_sock.is_open())) {
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
		//		dlog("status is {}", static_cast<uint32_t>(m.status ));
		return tcp_sock.is_open() && m.status == SocketStatus::SOCKET_OPEN;
	}

#if WITH_PACKAGE_HANDLER
	void process_data(uint32_t nread) {
		if (!connection) {
			return;
		}

		this->connection->process_event(EVT_RECV);
		//	this->m.read_buffer.resize(m.read_buffer.size() + nread);
		read_buffer_pos += nread;
		uint32_t readPos = 0;
		int32_t pkgLen = this->connection->process_package(m.read_buffer, read_buffer_pos);
		dlog("process data  pkg size is {}", pkgLen);
		if (pkgLen > kReadBufferSize) {
			elog("single packet size ({}) error, close connection", pkgLen);
			connection->close();
			return;
		}

		while (pkgLen > 0) {
			dlog("read packet length {}", pkgLen);
			if (readPos + pkgLen <= read_buffer_pos) {
				char* pkgEnd = (char*)m.read_buffer + readPos + pkgLen + 1;
				char endChar = *pkgEnd;
				*pkgEnd = 0;
				this->connection->process_data(std::string ( (char*)m.read_buffer + readPos, pkgLen), MessageStatus::MESSAGE_NONE);
				*pkgEnd = endChar;
				readPos += pkgLen;
			}

			if (readPos < read_buffer_pos) {
				pkgLen = this->connection->process_package(
					(char*)m.read_buffer + readPos, read_buffer_pos - readPos);
				if (pkgLen <= 0) {
					elog("moving buffer to front {} ", read_buffer_pos - readPos);
					memmove(m.read_buffer, (char*)m.read_buffer + readPos, read_buffer_pos - readPos);
					read_buffer_pos -= readPos;
					// this->m.read_buffer.erase(m.read_buffer.begin(),m.read_buffer.begin()+ readPos);
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
		this->connection->process_event(EVT_RECV);

		read_buffer_pos += nread;
		dlog("read data {} total is {}", nread, read_buffer_pos);
		uint32_t readPos = 0;
		int32_t pkgLen = 0;

		do {

			uint32_t readLen = read_buffer_pos - readPos; 

			if (need_package_length > 0) {

				if (need_package_length <= readLen) {
					pkgLen = this->connection->process_data(std::string((char*)m.read_buffer + readPos,
						need_package_length), MessageStatus::MESSAGE_END);
					if (pkgLen < 0) {
						connection->close(); 
						return ; 
					}
					//dlog(" need length {} package len {} , data len is {} chunk", need_package_length, pkgLen, readLen);
					readPos += need_package_length;
					// if (readPos >= read_buffer_pos) {
					// 	read_buffer_pos = 0;
					// 	return;
					// }

					need_package_length = 0;

				} else {

					pkgLen = this->connection->process_data(
						std::string((char*)m.read_buffer + readPos, readLen), MessageStatus::MESSAGE_CHUNK);
					if (pkgLen < 0) {
						connection->close(); 
						return ; 
					}
					//dlog(" need length {} package len {} , data len is {} chunk", need_package_length, pkgLen, readLen);
					read_buffer_pos = 0;
					need_package_length -= readLen;
					return;
				}

			} else {
				pkgLen = this->connection->process_data(
					std::string((char*)m.read_buffer + readPos, readLen), MessageStatus::MESSAGE_NONE);
				//dlog(" need length {} package len {} , data len is {}", need_package_length, pkgLen, readLen);
				if (pkgLen > kMaxPackageLimit) {
					elog("max package limit error {}", pkgLen);
					this->close();
					return;
				}

				if (pkgLen <= 0) { // no enough data 

					if (read_buffer_pos > readPos && readPos > 0) {
						memmove(m.read_buffer, (char*)m.read_buffer + readPos, read_buffer_pos - readPos);
					}
					read_buffer_pos -= readPos;
					return;
				} else {

					if ( pkgLen > (int32_t)readLen) { 
						// exceed buffer size, but user has consume the buffer data
						need_package_length = pkgLen - readLen;
						//read_buffer_pos -= readLen;
						read_buffer_pos = 0; 
						return;
					} else {
						 // buffer contain one or more packets
						//dlog("process just one message {}", pkgLen);
						readPos += pkgLen;
					}
				}
			}
			dlog("current read pos is {}", readPos);

		} while (read_buffer_pos > readPos);

		if (read_buffer_pos > readPos && readPos > 0) {
			memmove(m.read_buffer, (char*)m.read_buffer + readPos, read_buffer_pos - readPos);
			read_buffer_pos -= readPos;
		} else {
			read_buffer_pos = 0;
		}

		dlog("last read buffer pos {}", read_buffer_pos);
	}

#endif

	void close() { do_close(true); }
	void do_close(bool force = false) {

		if (m.status == SocketStatus::SOCKET_CLOSING || m.status == SocketStatus::SOCKET_CLOSED) {
			dlog("already in closing status {}", m.status);
			return;
		}

		if (force) {
			m.status = SocketStatus::SOCKET_CLOSING;
		}

		auto self = this->shared_from_this();
		if (m.send_buffer.size() > 0  &&  !m.is_writing ){
			do_async_write(); //try last write
		}
		asio::post(io_context, [self]() {
			elog("try to close connection ..."); 

			if (self->connection) {
				self->connection->process_event(EVT_DISCONNECT);
				self->read_buffer_pos = 0;
				if (self->connection->need_reconnect()) {
					self->m.status = SocketStatus::SOCKET_RECONNECT;
				} else {
					self->m.status = SocketStatus::SOCKET_CLOSED;
					if (self->socket().is_open()) {
						self->socket().close();
					}
					self->connection->release();
					self->connection.reset();
				}

			} else {
				self->m.status = SocketStatus::SOCKET_CLOSED;
				if (self->socket().is_open()) {
					self->socket().close();
					self->read_buffer_pos = 0;
				}
			}
		});
	}

	tcp::endpoint local_endpoint() {
		 
		return tcp_sock.local_endpoint();
	
	}

	tcp::endpoint remote_endpoint() { 
		return tcp_sock.remote_endpoint(); 
	}

	inline tcp::socket& socket() { return tcp_sock; }

	TPtr connection;
	inline bool is_inloop() { return worker_tid == std::this_thread::get_id(); }

	inline asio::io_context& context() { return io_context; }

	static uint32_t max_package_size ; 

private:
	enum { kReadBufferSize = 1024*8, kMaxPackageLimit = 8*1024 * 1024 };

	asio::io_context& io_context; 
	tcp::socket tcp_sock; 
	struct {
		asio::streambuf send_buffer;  
		char read_buffer[kReadBufferSize];
		std::mutex mutex;
		SocketStatus status = SocketStatus::SOCKET_IDLE;
		bool is_writing = false;
	} m; 
 
	
	std::thread::id worker_tid;
	uint32_t read_buffer_pos = 0;
	uint32_t need_package_length = 0;
};
 

} // namespace tcp

}; // namespace knet
