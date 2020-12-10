#pragma once
#include <functional>
#include "kpipe.hpp"
#include "pipe_connection.hpp"

namespace knet {
	namespace pipe {

		enum PipeMode { PIPE_SERVER_MODE = 1, PIPE_CLIENT_MODE = 2, PIPE_DUET_MODE = 3 };
		class PipeFactory : public ConnectionFactory<PipeConnection> {
		public:
			PipeFactory(PipeMode mode = PipeMode::PIPE_SERVER_MODE)
				: pipe_mode(mode) {}

			virtual void handle_event(TPtr conn, NetEvent evt) {

				ilog("factory event {} {} {}",evt,  event_string(evt), std::this_thread::get_id());

				switch (evt) {
				case EVT_CONNECT:
				{

					if (!conn->passive()) {
						send_shakehand(conn, conn->pipeid);
					}
				}
				break; 
				case EVT_DISCONNECT:
				{
					if (conn->session) {
						conn->session->handle_event(NetEvent::EVT_DISCONNECT);
						conn->session->unbind();
					}

					{

						std::lock_guard<std::mutex> guard(unbind_mutex); 	
						auto itr = unbind_pipes.find(conn->get_cid());
						if (itr != unbind_pipes.end()) {
							dlog("erase pipe {} ", conn->get_cid()); 
							unbind_pipes.erase(itr);
						}

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

			std::string generate_id() {
				static 	uint64_t pid_index = 0;
				return  fmt::format("pid{}", pid_index++);
			}

			void process_server_handshake(TPtr conn, PipeMessageS* msg) {
				if (msg->head.length > 0) {
					std::string pipeId = std::string(msg->data, msg->head.length);
					dlog("get handshake from client,  pipeid is {}", pipeId);
					auto itr = pipes.find(pipeId);
					if (itr != pipes.end()) {
						auto session = itr->second;
						session->bind(conn);
						conn->session = session;
						dlog("bind session success");
						PipeMessage<64> shakeMsg;
						shakeMsg.fill(PIPE_MSG_SHAKE_HAND, pipeId);
						conn->send(shakeMsg.begin(), shakeMsg.length());
						session->handle_event(NetEvent::EVT_CONNECT);
					}
					else {
						wlog("pipe id not found {}", pipeId);
						conn->close();
					}
				}
				else {
					//create a pipeid to client 
					auto pipeId = this->generate_id();
					auto itr = unbind_pipes.find(conn->get_cid());
					PipeSessionPtr session;
					if (itr == unbind_pipes.end()) {
						dlog("create and bind session success {}", pipeId );
						session = std::make_shared<PipeSession>();
						session->bind(conn);
						conn->session = session;
						unbind_pipes[conn->get_cid()] = session;
					}
					else {
						session = itr->second;
						if (session->get_pipeid().empty()) {
							session->set_pipeid(pipeId);
						}
					}

					if (session) {
						PipeMessage<64> shakeMsg;
						shakeMsg.fill(PIPE_MSG_SHAKE_HAND, pipeId);
						conn->send(shakeMsg.begin(), shakeMsg.length());
						session->handle_event(NetEvent::EVT_CONNECT);
					}
				}
			}


			void process_client_handshake(TPtr conn, PipeMessageS* msg) {
				if (msg->head.length > 0) {
					std::string pipeId = std::string(msg->data, msg->head.length);
					dlog("get handshake from server,  pipeid is {}", pipeId);
					auto itr = pipes.find(pipeId);
					if (itr != pipes.end()) {
						auto session = itr->second;
						session->bind(conn);
						if (session->get_pipeid().empty()) {
							//update local session pipeid 
							session->set_pipeid(pipeId);
						}
						conn->session = session;
						dlog("bind session success");
						session->handle_event(NetEvent::EVT_CONNECT);
					}
					else {
						dlog("try to find pipe by cid {} size is {}", conn->get_cid(), unbind_pipes.size());
						auto connItr = unbind_pipes.find(conn->get_cid());
						if (connItr != unbind_pipes.end()) {
							auto session = connItr->second;
							session->bind(conn);
							if (session->get_pipeid().empty()) {
								//update unbind session pipeid 
								session->set_pipeid(pipeId);
							}
							conn->session = session;
							dlog("bind session success");
							session->handle_event(NetEvent::EVT_CONNECT);
							
							{

								std::lock_guard<std::mutex> guard(unbind_mutex);  
								pipes[pipeId] = session;
								unbind_pipes.erase(connItr); 
							}
						
						}
						else {
							wlog("pipe id not found {} cid is {}", pipeId, conn->get_cid());
							conn->close();
						}
					}
				}
				else {
					elog("handshake from server is empty");

				}
			}


			virtual uint32_t handle_data(TPtr conn, const std::string& buf, MessageStatus status) {

				PipeMessageS* msg = (PipeMessageS*)buf.data();
				if (msg->head.length + sizeof(PipeMsgHead) > buf.length()) {
					elog("data not enough, need length {}", msg->head.length + sizeof(PipeMsgHead));
					return 0;
				}

				if (msg->head.type == PIPE_MSG_SHAKE_HAND) {
					dlog("handle pipe shake hande message ");
					if (conn->passive()) { //server side  
						process_server_handshake(conn, msg);
					}
					else {
						process_client_handshake(conn, msg);
					}

					return sizeof(PipeMsgHead) + msg->head.length;
				}
				else {
					wlog("message type {}", msg->head.type);
				}

				if (conn->session) {
					conn->session->handle_message(std::string_view(buf.data() + sizeof(PipeMsgHead), msg->head.length));
				}
				else {
					elog("connection has no session");
				}
				return sizeof(PipeMsgHead) + msg->head.length;
			}

			void send_shakehand(TPtr conn, const std::string& pipeId) {
				dlog("shakehande request pipe id {}", pipeId);
				PipeMessage<64> shakeMsg ;
				shakeMsg.fill(PIPE_MSG_SHAKE_HAND, pipeId);
				// if (!pipeId.empty()) {
				conn->send(shakeMsg.begin(), shakeMsg.length());
		 
				// }
			}

			void broadcast(const char* pData, uint32_t len) {
				for (auto& item : pipes) {
					if (item.second) {
						item.second->transfer(pData, len);
					}
				}

				// for (auto& item : unbind_pipes) {
				// 	if (item.second) {
				// 		item.second->transfer(pData, len);
				// 	}
				// }
			}
			void register_pipe(PipeSessionPtr pipe, uint64_t cid = 0) {
				if (pipe) {
					if (pipe->get_pipeid().empty())
					{
						if (cid != 0) {
							dlog("add unbind pipe {}", cid);
							std::lock_guard<std::mutex> guard(unbind_mutex); 				
							unbind_pipes[cid] = pipe;
						}

					}					
					else {
						dlog("add normal pipe {}", pipe->get_pipeid());
						pipes[pipe->get_pipeid()] = pipe;
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

			void start_clients(std::function<void(PipeSessionPtr)> handler) {

				for (auto& item : pipes) {
					auto& pipe = item.second;
					if (pipe && pipe->get_port() != 0 && !pipe->get_host().empty()) {
						dlog("start connect pipe to {}:{}", pipe->get_host(), pipe->get_port());
						if (handler) {
							handler(pipe);
						}
					}
				}

				for (auto& item : unbind_pipes) {
					auto& pipe = item.second;
					if (pipe && pipe->get_port() != 0 && !pipe->get_host().empty()) {
						dlog("start connect pipe to {}:{}", pipe->get_host(), pipe->get_port());
						if (handler) {
							handler(pipe);
						}
					}
				}
			}


		private:
			std::unordered_map<std::string, PipeSessionPtr> pipes;

			std::mutex  unbind_mutex; 
			std::unordered_map<uint64_t, PipeSessionPtr> unbind_pipes;
			PipeMode pipe_mode;
		};

		using PipeFactoryPtr = std::shared_ptr<PipeFactory>;

	}

}
