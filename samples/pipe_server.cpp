#include "pipe/kpipe.hpp"

using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){

		}
		virtual ~MyChannel(){}

		virtual bool handle_event(knet::NetEvent evt) { dlog("handle net event {}", evt);  return true; }
		virtual int32_t handle_message(const std::string& msg, uint64_t obdata = 0 ) {
			dlog("---------------{}----------------", msg.size()); 
			dlog("outbond data is {}", obdata); 
			dlog("{}",msg); 
			
			dlog("---------------------------------"); 

			//			this->transfer(std::string(msg.data(), msg.length())); 
			//
			return msg.size(); 
		} 

}; 


int main(int argc, char * argv[]){ 

	kLogIns.add_console(); 
	auto mySession = std::make_shared<MyChannel>("pipe1" ); 

	KPipe<>  spipe(PipeMode::PIPE_SERVER_MODE);  
	spipe.attach(mySession);  

	uint32_t port = 6688; 
	dlog("start server at {} ", port); 
	spipe.start("127.0.0.1",port);  

	while(1){
		std::this_thread::sleep_for(std::chrono::seconds(3));
 
		const char * pMsg = "hello world"; 
		//mySession->transfer(pMsg, strlen(pMsg)); 
		uint64_t obid = 199999; 
		mySession->send(obid, std::string( pMsg, strlen(pMsg)) ); 
		//spipe.broadcast("message from serever"); 
	}; 


	return 0; 
};
