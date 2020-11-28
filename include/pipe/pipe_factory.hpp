#pragma once
#include "kpipe.hpp"
#include "pipe_connection.hpp"

namespace knet{
	namespace pipe{ 

		enum PipeMode { PIPE_SERVER_MODE = 1, PIPE_CLIENT_MODE = 2, PIPE_DUET_MODE = 3 };
		class PipeFactory : public ConnectionFactory<PipeConnection> {
			public:
				PipeFactory(PipeMode mode = PipeMode::PIPE_SERVER_MODE)
					: pipe_mode(mode) {}

				virtual void handle_event(TPtr conn, NetEvent evt) {

					ilog("factory event {} {}", event_string(evt), std::this_thread::get_id());

					switch (evt) {
						case EVT_CONNECT: 
							{

								if (!conn->passive()) {
									send_shakehand(conn, conn->pipeid);
								}
							}
						case EVT_DISCONNECT: 
							{
								if (conn->session) {
									conn->session->handle_event(NetEvent::EVT_DISCONNECT);
									conn->session->unbind();
								}
							}

							break;
						default:;
					}

					if (conn->session) {
						dlog("handle session event {}", evt);
						conn->session->handle_event(evt);
					}
				}
				// virtual int32_t handle_package(TPtr conn, const char* data, uint32_t len) {

				// 	PipeMessageS* msg = (PipeMessageS*)data;
				// 	if (msg->head.length + sizeof(PipeMsgHead) > len) {
				// 		elog("received data {}", data); 
				// 		elog("data {} not enough on package {} ", len , msg->head.length);
				// 		return 0;
				// 	}

				// 	return sizeof(PipeMsgHead) + msg->head.length;
				// }

				virtual uint32_t handle_data(TPtr conn, const std::string& buf, MessageStatus status) {
					dlog("factory handle data  length {}", buf.length());

					PipeMessageS* msg = (PipeMessageS*)buf.data();
					if (msg->head.length + sizeof(PipeMsgHead) > buf.length()) {
						elog("data not enough, need length {}", msg->head.length + sizeof(PipeMsgHead));
						return 0;
					}

					if (msg->head.type == PIPE_MSG_SHAKE_HAND) {
						std::string pipeId = std::string(msg->data, msg->head.length);
						dlog("get handshake ,  pipeid is {}", pipeId);

						auto itr = pipes.find(pipeId);
						if (itr != pipes.end()) {
							auto session = itr->second;
							session->bind(conn);
							conn->session = session;
							dlog("bind session success");
							if (conn->passive()) {
								PipeMessage<64> shakeMsg;
								shakeMsg.fill(PIPE_MSG_SHAKE_HAND, pipeId);
								conn->send(shakeMsg.begin(), shakeMsg.length());
								session->handle_event(NetEvent::EVT_CONNECT);
							} else {
								session->handle_event(NetEvent::EVT_CONNECT);
							}
						} else {
							wlog("pipe id not found {}", pipeId);
							conn->close();
						}
						return sizeof(PipeMsgHead) + msg->head.length;
					} else {
						wlog("message type {}", msg->head.type); 
					}

					if (conn->session) {
						conn->session->handle_message(std::string_view(buf.data() + sizeof(PipeMsgHead), msg->head.length) );
					} else {
						elog("connection has no session");
					}
					return sizeof(PipeMsgHead) + msg->head.length;
				}

				void send_shakehand(TPtr conn, const std::string& pipeId) {
					dlog("shakehande request pipe id {}", pipeId);
					PipeMessage<64> shakeMsg;
					shakeMsg.fill(PIPE_MSG_SHAKE_HAND, pipeId);
					conn->send(shakeMsg.begin(), shakeMsg.length());
				}

				void broadcast(const char* pData, uint32_t len) {
					for (auto& item : pipes) {
						if (item.second) {
							item.second->transfer(pData, len);
						}
					}
				}

				PipeSessionPtr find(const std::string& pid) {
					auto itr = this->pipes.find(pid);
					if (itr != this->pipes.end()) {
						return itr->second;
					}
					return nullptr;
				}

				std::unordered_map<std::string, PipeSessionPtr> pipes;

			private:
				PipeMode pipe_mode;
		};

		using PipeFactoryPtr = std::shared_ptr<PipeFactory>;

	}

}
