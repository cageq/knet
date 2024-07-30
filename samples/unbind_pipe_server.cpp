#include "pipe/kpipe.hpp"

using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){

		}
		virtual ~MyChannel(){}

		virtual bool handle_event(knet::NetEvent evt) override{ knet_dlog("handle net event {}", static_cast<uint32_t>(evt)); return true;  }
		virtual int32_t handle_message(char * data, uint32_t dataLen, uint64_t ) override {
			knet_dlog("---------------{}----------------", dataLen); 
			knet_dlog("{}",data); 
			knet_dlog("---------------------------------"); 

			//			this->msend(std::string(msg.data(), msg.length())); 
			//
			return dataLen; 
		} 

}; 


int main(int argc, char * argv[]){ 

	knet_add_console_sink();
//	auto mySession = std::make_shared<MyChannel>("1" ); 

	KPipe<>  spipe(PipeMode::PIPE_SERVER_MODE);  
	//spipe.attach(mySession);  

	knet_dlog("start server at {} ", 9999); 
	spipe.start("127.0.0.1",9999);  

	while(1){
		usleep(500000); 
		//const char * pMsg = "hello world"; 
		//mySession->transfer(pMsg, strlen(pMsg)); 
		spipe.broadcast("message from serever"); 
	}; 


	return 0; 
};
