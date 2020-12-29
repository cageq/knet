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
				PipeConnection(const std::string& pid = "") { pipeid = pid; }

				virtual int32_t handle_package( const char* data, uint32_t len) {
					dlog("demarcate  pipe package {}", len); 
					PipeMsgHead* msg = (PipeMsgHead*)data;
					if (msg->length + sizeof(PipeMsgHead) > len) {			 
						elog("demarcate pipe message length {}  message head  {} ", len , msg->length);
						return 0;
					}
					return sizeof(PipeMsgHead) + msg->length;
				} 

				inline std::shared_ptr<PipeSession> get_session(){
					return session ; 
				}
				inline void set_session(std::shared_ptr<PipeSession> session){
					this->session = session; 
				}
				inline std::string get_pipeid()const {
					return pipeid; 
				}
			private: 
				std::shared_ptr<PipeSession> session;
				std::string pipeid;
		};

		using PipeConnectionPtr = std::shared_ptr<PipeConnection>; 
	}

}
