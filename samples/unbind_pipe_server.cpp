#include "pipe/kpipe.hpp"

using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){

		}
		virtual ~MyChannel(){}

		virtual bool handle_event(knet::NetEvent evt) { dlog("handle net event {}", evt); return true;  }
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
//	auto mySession = std::make_shared<MyChannel>("1" ); 

	KPipe<>  spipe(PipeMode::PIPE_SERVER_MODE);  
	//spipe.attach(mySession);  

	dlog("start server at {} ", 9999); 
	spipe.start("127.0.0.1",9999);  

	while(1){
		usleep(500000); 
		//const char * pMsg = "hello world"; 
		//mySession->transfer(pMsg, strlen(pMsg)); 
		spipe.broadcast("message from serever"); 
	}; 


	return 0; 
};
