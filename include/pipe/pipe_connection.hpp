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
					dlog("demarcate  pipe package {}", len); 
					PipeMsgHead* msg = (PipeMsgHead*)data;
					if (msg->length + sizeof(PipeMsgHead) > len) {			 
						wlog("demarcate pipe message length {}  message head  {} ", len , msg->length);
						return 0;
					}
					return sizeof(PipeMsgHead) + msg->length;
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
