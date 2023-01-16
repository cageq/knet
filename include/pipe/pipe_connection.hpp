#pragma once

#include "knet.hpp"
#include "pipe_proto.hpp"
using namespace knet::tcp;

namespace knet{
	namespace pipe{
 

		class PipeSession;

		class PipeConnection : public TcpConnection<PipeConnection> {
			public:

				friend class PipeSession; 
				using PipeSessionPtr = std::shared_ptr<PipeSession>; 
				PipeConnection(const std::string& pid = "") { pipeid = pid; }

				virtual int32_t handle_package( const char* data, uint32_t len) {
					if(len < sizeof(PipeMsgHead)){
						return 0; 
					}
		
					PipeMsgHead* msg = (PipeMsgHead*)data;
					if (msg->length + sizeof(PipeMsgHead) > len) {			 
						//wlog("demarcate message length {}  message head length {} ", len , msg->length);
						return 0;
					}
					return sizeof(PipeMsgHead) + msg->length;
				} 

				
				void send_shakehand(const std::string &pipeId)
				{
					dlog("shakehande request pipe id {}  ", pipeId);
					PipeMsgHead shakeMsg{ static_cast<uint32_t>(pipeId.length()), PIPE_MSG_SHAKE_HAND,0};
					this->msend(shakeMsg, pipeId);
					// if (!pipeId.empty()) {
					// conn->send(shakeMsg.begin(), shakeMsg.length());
					// }
						
				}

				int32_t send_heartbeat(const std::string &msg= "")
				{
					PipeMsgHead head{ static_cast<uint32_t>(msg.length()), PIPE_MSG_HEART_BEAT, 0 };			 
					return msend(head, msg);		  
				}

				inline PipeSessionPtr get_session(){
					return session ; 
				}

				inline void set_session( PipeSessionPtr session){
					this->session = session; 
				}

				inline std::string get_pipeid()const {
					return pipeid; 
				}
			private: 
				PipeSessionPtr session;
				std::string pipeid;
		};

		using PipeConnectionPtr = std::shared_ptr<PipeConnection>; 
	}

}