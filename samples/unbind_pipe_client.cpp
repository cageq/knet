#include "pipe/kpipe.hpp"
#include <thread> 


using namespace knet::tcp; 
using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){ 
		}

		virtual ~MyChannel(){}
		virtual bool handle_event(knet::NetEvent evt) { 
			
			dlog("handle net event {}", evt);
			
			if (evt == knet::NetEvent::EVT_CONNECT){

				std::string testMsg = "hello world from client";
				this->msend(testMsg); 
			}
			return true; 
			
		 }
		virtual int32_t handle_message(const std::string& msg, uint64_t ) {
			dlog("---------------{}----------------", msg.size()); 
			dlog("{}",msg); 
			dlog("---------------------------------"); 
//			this->msend(std::string(msg.data(), msg.length())); 
//
			return msg.size(); 
		} 
}; 
 


int main(int argc, char * argv[]){ 
 	KNetLogIns.add_console();
	KPipe<> cpipe(PipeMode::PIPE_CLIENT_MODE); 
	cpipe.attach("127.0.0.1",9999); 
	cpipe.start("127.0.0.1",9999);  

	while(1){
		std::this_thread::sleep_for(std::chrono::seconds(3)); 
		//mySession->transfer("welcome to 2020",15);  
		dlog("try to broadcast message"); 
		cpipe.broadcast("welcome to 2020");  
	}; 
	return 0; 
};
