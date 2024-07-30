#include "pipe/kpipe.hpp"

using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){

		}
		virtual ~MyChannel(){}

		virtual bool handle_event(knet::NetEvent evt) override { knet_dlog("handle net event {}", static_cast<uint32_t>(evt));  return true; }
		virtual int32_t handle_message(char * data, uint32_t dataLen, uint64_t obdata = 0 ) override{
			knet_ilog("---------------{}----------------", dataLen); 
			knet_ilog("outband data is {}", obdata); 
			knet_ilog("{}",data); 
			knet_ilog("---------------------------------"); 

			//			this->msend(std::string(msg.data(), msg.length())); 
			//
			return dataLen; 
		} 

}; 


int main(int argc, char * argv[]){ 

	  
	knet_add_console_sink();  
	auto mySession = std::make_shared<MyChannel>("pipe1" ); 

	KPipe<>  spipe(PipeMode::PIPE_SERVER_MODE);  
	spipe.attach(mySession);  

	uint32_t port = 6688; 
	knet_dlog("start server at {} ", port); 
	spipe.start("127.0.0.1",port);  
	uint64_t obid = 10000; 

	while(1){
		std::this_thread::sleep_for(std::chrono::seconds(3));
 
		std::string msg = fmt::format("msg from server {}" , obid++); 
		//mySession->transfer(pMsg, strlen(pMsg)); 
		mySession->send( msg ,obid); 
		//spipe.broadcast("message from serever"); 
	}; 


	return 0; 
};
