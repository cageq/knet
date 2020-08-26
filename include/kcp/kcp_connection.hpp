//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include <asio.hpp>
#include "klog.hpp"
#include "utils.hpp"

#include "timer.hpp"

#include "ikcp.h"
#include "kcp_message.hpp"
#include "event_worker.hpp"

#include <string>
#include <memory>

namespace knet {
namespace kcp {

using asio::ip::udp;
using namespace knet::utils;
using namespace std::chrono;

using KcpSocketPtr = std::shared_ptr<udp::socket>;

inline std::string addrstr(udp::endpoint pt) {
	return pt.address().to_string() + ":" + std::to_string(pt.port());
}
 

template <typename T>
class KcpConnection : public std::enable_shared_from_this<T> {
public:
	enum ConnectionStatus { CONN_IDLE, CONN_OPEN, CONN_KCP_READY, CONN_CLOSING, CONN_CLOSED };
	using TPtr = std::shared_ptr<T>;
	using EventHandler = std::function<TPtr(TPtr, NetEvent, const  std::string&)>;

	
	KcpConnection(asio::io_context& ctx)
		: io_context(ctx)
		, timer(ctx) {
		shakehand_request_.cmd = KCP_SHAKEHAND_REQUEST;
		shakehand_response_.cmd = KCP_SHAKEHAND_RESPONSE;
		disconnect_message_.cmd = KCP_GOODBYE;
		heartbeat_message_.cmd = KCP_HEARTBEAT;

		status = CONN_IDLE;
	}


	enum PackageType {
		PACKAGE_PING,
		PACKAGE_PONG,
		PACKAGE_USER,
	};

	void init_kcp() { 
		dlog("create kcp cid is {}", cid);
		if (kcp != nullptr) {
			ikcp_release(kcp);
			kcp = nullptr;
		}

		if (kcp == nullptr) {
			kcp = ikcp_create(cid, this);
			kcp->output = [](const char* buf, int len, ikcpcb* kcp, void* user) {
				auto self = static_cast<KcpConnection<T>*>(user);
				self->do_send(buf, len);
				return 0;
			};
			ikcp_nodelay(kcp, 1, 10, 1, 1);
			// ikcp_nodelay(kcp, 0, 40, 0, 0);
			if (kcp_timerid == 0) {
				kcp_timerid = timer.start_timer(
					[this]() {
						if (status == CONN_KCP_READY && kcp) {
							auto timeNow = std::chrono::system_clock::now();
							auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
								timeNow.time_since_epoch());

							ikcp_update(kcp, duration.count());
						}
					},
					100);
			}
		}

		status = CONN_KCP_READY;
		dlog("init kcp success , change status  to ready {}", status);
	}



	static TPtr create(asio::io_context& ctx) { return std::make_shared<T>(ctx); }

	int32_t do_sync_send(const char* data, std::size_t len)
	{
		if (sock)
		{
				asio::error_code ec;
				dlog("send message {} to {}:{}", len, remote_point.address().to_string(),
				remote_point.port());
				int32_t ret = sock->send_to(asio::const_buffer(data, len), remote_point,0, ec); 
 
				if (!ec) {
					return ret;
				}
		}
		return -1; 
	}

	int32_t do_send(const char* data, std::size_t length) {
		if (sock) {
			dlog("send message {} to {}:{}", length, remote_point.address().to_string(),
				remote_point.port());

			//std::string msg(data,length); 
			auto buffer = std::make_shared<std::string>(data, length);
			sock->async_send_to(asio::buffer(*buffer), remote_point,
				[this, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						if (event_handler) {
							event_handler(this->shared_from_this(), EVT_SEND, {nullptr,len});
						}
					} else {
						dlog("sent message , error code {}, {}", ec.value(), ec.message());
					}
				 
				});

			return 0;
		}
		return -1;
	}

	int32_t send(const std::string& msg) {
		dlog("start to send user message on status {}", status);
		if (status == CONN_KCP_READY) {
			if (kcp) {
				return ikcp_send(kcp, msg.c_str(), msg.length());
			}
		} else {
			this->shakehand_request();
		}
		return -1;
	}
 

	virtual PackageType on_message(const char* data, uint32_t len) {

		dlog("on recv udp message {} , length is {}", data, len);
		return PACKAGE_USER;
	}
	int32_t disconnect() {

		if (sock && status < CONN_CLOSING) {
			status = CONN_CLOSING;
			disconnect_message_.conv = cid;
			auto self = this->shared_from_this(); 

			if (kcp_timerid != 0 ) {
				timer.stop_timer(kcp_timerid);
				kcp_timerid = 0; 
			}

			sock->async_send_to(
				asio::buffer((const char*)&disconnect_message_, sizeof(KcpShakeHandMsg)),
				remote_point, [=](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (ec) {						
						dlog("sent message , error code {}, {}", ec.value(), ec.message());
					}

					if (event_handler) {
						event_handler(self->shared_from_this(), EVT_DISCONNECT, {nullptr, 0});
					}
					self->release();
				}); 
		

			return 0;
		}
		return -1;
	} 
	uint32_t cid = 0;
	std::chrono::milliseconds last_msg_time;

//private:
	//template <class>
	//friend class KcpListener;
	//template <class>
	//friend class KcpConnector;

	bool passive = false; 
	void release() {

		if (status != CONN_CLOSED) {
			status = CONN_CLOSED;
			dlog("release all resources {}", this->cid);
			timer.stop_timer(heartbeat_timerid);
			timer.stop_timer(kcp_timerid);

			if (sock && !passive) {
				sock->close();
				sock = nullptr;
			}

			if (kcp) {
				ikcp_release(kcp);
				kcp = nullptr;
			}	
		}
	}

	void listen(KcpSocketPtr s) {
		sock = s;
		status = CONN_OPEN;
		passive = true;
		if (sock) {
			auto self = this->shared_from_this(); 
			heartbeat_timerid = timer.start_timer(
				[=]() {
					auto timeNow = std::chrono::system_clock::now();
					auto elapseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
						timeNow.time_since_epoch() - last_msg_time);

					if (elapseTime.count() > 3000) {
						wlog("check heartbeat failed, closing connection");

						if (event_handler) {
							event_handler(self->shared_from_this(), EVT_DISCONNECT, {});
						} 
						
					} else {
						self->send_heartbeat();
					}

					ilog("check heartbeat timer {}", elapseTime.count());
				},
				1000000);
		}
	}

	void connect(asio::io_context& ctx, udp::endpoint pt, bool reconn = true) {
		remote_point = pt;
		passive = false;
		reconnect = reconn;
		this->sock = std::make_shared<udp::socket>(ctx, udp::endpoint(udp::v4(), 0));
		if (sock) {
			auto self = this->shared_from_this(); 
			heartbeat_timerid = timer.start_timer(
				[=]() {
					auto timeNow = std::chrono::system_clock::now();
					auto elapseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
						timeNow.time_since_epoch() - last_msg_time);

					if (elapseTime.count() > 3000) {
						wlog("check heartbeat failed, closing connection");

						if (event_handler) {
							event_handler(this->shared_from_this(), EVT_DISCONNECT, {});
						}
						if(!reconnect)
						{
							self->release();
						}
					} else {
						this->send_heartbeat();
					}

					ilog("check heartbeat timer {}", elapseTime.count());
				},
				1000000);
			status = CONN_OPEN;
			shakehand_request();
			do_receive();
		}
	}

	void shakehand_request(uint32_t id = 0) {

		if (sock) {
			shakehand_request_.conv = id == 0 ? this->cid : id;
			dlog("send shakehand request to client {}", shakehand_request_.conv);
			sock->async_send_to(
				asio::buffer((const char*)&shakehand_request_, sizeof(KcpShakeHandMsg)),
				remote_point, [this](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						dlog("send shakehand request successful {}", shakehand_request_.conv);
					} else {
						dlog("sent message , error code {}, {}", ec.value(), ec.message());
					}
				});
		} else {
			elog("socket is not ready");
		}
	}

	void shakehand_response(uint32_t id = 0) {

		if (sock) {

			shakehand_response_.conv = id == 0 ? this->cid : id;
			sock->async_send_to(
				asio::buffer((const char*)&shakehand_response_, sizeof(KcpShakeHandMsg)),
				remote_point, [this](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						dlog("send shakehand response successful {}", shakehand_response_.conv);
					} else {
						dlog("sent message , error code {}, {}", ec.value(), ec.message());
					}
				});
		}
	}
	void send_heartbeat() {
		if (sock) {
			heartbeat_message_.conv = this->cid;
			sock->async_send_to(
				asio::buffer((const char*)&heartbeat_message_, sizeof(KcpHeartbeat)), remote_point,
				[this](std::error_code ec, std::size_t len /*bytes_sent*/) {
					if (!ec) {
						dlog("send heartbeat message successful {}", shakehand_response_.conv);
					} else {
						dlog("sent message , error code {}, {}", ec.value(), ec.message());
					}
				});
		}
	}
	bool check_control_message(const char* pData, uint32_t len) {
		static uint32_t server_conv_index = 0x1000;

		KcpMsgHead* head = (KcpMsgHead*)pData;

		dlog("check shakehand status is {}, type is {}", status, head->cmd);
		if (status == CONN_OPEN) {

			switch (head->cmd) {
			case KCP_SHAKEHAND_REQUEST: {
				KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
				if (passive && shakeMsg->conv == 0 && this->cid == 0) {
					this->cid = server_conv_index++;
					wlog("allocate cid to client {}", this->cid);
				}

				this->cid = shakeMsg->conv == 0 ? this->cid : shakeMsg->conv;
				dlog("get shakehand request, cid is {} ", shakeMsg->conv);
				if (this->cid != 0) {
					this->shakehand_response(this->cid);
					this->init_kcp();
					if (event_handler) {
						event_handler(this->shared_from_this(), EVT_CONNECT, {});
					}
				}

			} break;
			case KCP_SHAKEHAND_RESPONSE: {

				KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
				dlog("get shakehand response on open status, cid is {}", shakeMsg->conv);
				if (this->cid == 0) {
					wlog("update cid from other side {}", shakeMsg->conv);
					this->cid = shakeMsg->conv;
				}

				this->init_kcp();
				if (event_handler) {
					event_handler(this->shared_from_this(), EVT_CONNECT, {});
				}
			} break;
			case KCP_HEARTBEAT: {

				dlog("receive heartbeat , ignore it ");
			} break;

			default:
				wlog("on open state, need shakehand, send shakehand request ");
				this->shakehand_request(); // lost cid ,request cid again

				return true; // ignore message on open state , treat it as control message
			}

		} else if (status == CONN_KCP_READY) {
			switch (head->cmd) {
			case KCP_SHAKEHAND_REQUEST: {
				KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
				wlog("get shakehand request  , cid is {} ", shakeMsg->conv);
				if (shakeMsg->conv == 0) {
					wlog("request shakehand cid is 0 ,using my cid {}", this->cid);
				} else {
					if (this->cid == 0) {
						this->cid = shakeMsg->conv;
					}
				}
				status = CONN_OPEN;
				this->init_kcp();
				this->shakehand_response(this->cid); // report my cid again
			} break;
			case KCP_SHAKEHAND_RESPONSE: {
				KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
				wlog("get shakehand response on kcp ready status, cid is {} ", shakeMsg->conv);
				if (this->cid == 0) {
					this->cid = shakeMsg->conv;
				}
				this->init_kcp();
			} break;

			case KCP_HEARTBEAT: {
				dlog("receive heartbeat , ignore it on ready");
			} break;
			default:;
			}
		}
		return (head->cmd == KCP_SHAKEHAND_REQUEST || head->cmd == KCP_SHAKEHAND_RESPONSE ||
				head->cmd == KCP_HEARTBEAT);
	}

	void do_receive() {
		sock->async_receive_from(asio::buffer(recv_buffer, kMaxMessageLength), sender_point,
			[this](std::error_code ec, std::size_t bytes_recvd) {
				if (!ec && bytes_recvd > 0) {
					dlog("receive message length {}", bytes_recvd);
					recv_buffer[bytes_recvd] = 0;

					auto timeNow = std::chrono::system_clock::now();
					last_msg_time = std::chrono::duration_cast<std::chrono::milliseconds>(
						timeNow.time_since_epoch());

					if (!check_control_message((const char*)recv_buffer, bytes_recvd)) {
						handle_receive((const char*)recv_buffer, bytes_recvd);
					}
					do_receive();
				} else {
					elog("async receive error {}, {}", ec.value(), ec.message());
				}
			});
	}

	void handle_receive(const char* data, std::size_t dataLen) {

		dlog("handle receive data {}", dataLen);
		if (kcp) {
			int32_t cmd = ikcp_input(kcp, data, dataLen);
			dlog("kcp input command  {}", cmd);

			if (cmd == IKCP_CMD_ACK) {
				//ilog("it's ack command ,ignore it");
				return;
			}
			uint32_t recvLen = 0; 

			while (recvLen < dataLen)
			{
				char kcpBuf[kMaxMessageLength] = "";
				int32_t kcp_recvd_bytes = ikcp_recv(kcp, kcpBuf, sizeof(kcpBuf));
				if (kcp_recvd_bytes <= 0) {
					elog(" kcp received bytes error {}", kcp_recvd_bytes);
					// ikcp_log(kcp,"kcp input error"); 						
					break; 
				} else {
					recvLen += kcp_recvd_bytes; 
					dlog("received kcp message length {}", kcp_recvd_bytes);
					this->on_message(kcpBuf, kcp_recvd_bytes);

					if (event_handler) {
						event_handler(this->shared_from_this(), EVT_RECV, {data, dataLen});
					}
				}
			}; 
		}
	}

	KcpSocketPtr sock;
 
	enum { kMaxMessageLength = 4096 };
	char recv_buffer[kMaxMessageLength];

	ConnectionStatus status;

	udp::endpoint sender_point;
	udp::endpoint remote_point;
	EventHandler event_handler = nullptr;

	bool reconnect = false;

	KcpShakeHandMsg shakehand_request_;
	KcpShakeHandMsg shakehand_response_;
	KcpShakeHandMsg disconnect_message_;
	KcpHeartbeat heartbeat_message_;
 

	asio::io_context& io_context;
	Timer timer;
	ikcpcb* kcp = nullptr;
	uint64_t kcp_timerid = 0;
	uint64_t heartbeat_timerid = 0;
};

} // namespace udp
} // namespace gnet
