#pragma once
#include <memory>
#include "pipe_proto.hpp"

namespace knet {

	namespace pipe{


		enum PipeEvent {

			PIPE_CONNECT, 
			PIPE_DISCONNECT,
			PIPE_HEARTBEAT_REQUEST,
			PIPE_HEARTBEAT_RESPONSE 

		};

		class PipeSession {
			public:
				PipeSession(const std::string& pid = "", const std::string& h = "", uint16_t p = 0)
					: host(h)
					  , port(p) {
						  pipeid = pid;
					  }

				virtual void handle_event(NetEvent evt) { dlog("handle net event {}", evt); }
				virtual int32_t handle_message(const fmt::string_view & msg) {

					dlog("handle pipe message : {}", msg);

					return 0;
				}

				int32_t transfer(const std::string & msg) {

					return transfer(msg.data(), msg.length()); 
				}

				int32_t transfer(const char* pData, uint32_t len) {
					if (connection) {
						dlog("transfer data  to pipe {} , length {}", pipeid, len);

						PipeMsgHead head(PIPE_MSG_DATA, len);
						dlog("send head length {}, type is {}", head.length, head.type); 
						return connection->msend(
								std::string((const char*)&head, sizeof(PipeMsgHead)), std::string(pData, len));
					} else {

						wlog("pipe session has no connection {}", pipeid);
					}
					return -1;
				}

				int32_t send(const std::string& msg) {
					if (connection) {

						return connection->send(msg);
					}
					return -1;
				}
				bool is_ready() {
					if (connection) {
						return connection->is_connected();
					}
					return false;
				}

				int32_t send(const char* pData, uint32_t len) {
					if (connection) { 
						return connection->send(pData, len);
					}
					return -1;
				}
				void send_heartbeat(const std::string & msg){
					PipeMsg  hbMsg;
					hbMsg.fill(PIPE_MSG_HEART_BEAT, msg);  
					transfer(hbMsg.data(), hbMsg.length()); 
				}

				void bind(PipeConnectionPtr conn) {
					connection = conn;
					conn->pipeid = pipeid;
				}
				void unbind() { connection.reset(); }

				std::string pipeid;
				std::string host;
				uint16_t port;

			private:
				PipeConnectionPtr connection;
		};

		using PipeSessionPtr = std::shared_ptr<PipeSession>;


	}

}
