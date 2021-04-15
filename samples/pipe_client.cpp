#include "pipe/kpipe.hpp"
#include <thread> 


using namespace knet::tcp; 
using namespace knet::pipe; 

class MyChannel : public PipeSession{

	public:
		MyChannel(const std::string & id):PipeSession(id){ 
		}

		virtual ~MyChannel(){}
		virtual bool handle_event(knet::NetEvent evt) { dlog("handle net event {}", evt);  return true; }
		virtual int32_t handle_message(const std::string& msg,uint64_t obdata = 0 ) {
			dlog("---------------{}----------------", msg.size()); 
			dlog("outband data is {}", obdata); 
			dlog("{}",msg); 
			dlog("---------------------------------"); 
//			this->transfer(std::string(msg.data(), msg.length())); 
//
			return msg.size(); 
		} 
}; 
 


int main(int argc, char * argv[]){ 

	KNetLogIns.add_console();
	auto mySession = std::make_shared<MyChannel>("pipe1"); 
	KPipe<> cpipe(PipeMode::PIPE_CLIENT_MODE); 

	uint32_t port = 6688; 
	cpipe.attach(mySession,"127.0.0.1",port); 
	cpipe.start();  

	uint64_t index = 0; 
		std::this_thread::sleep_for(std::chrono::seconds(1)); 
	while(1){
		//std::this_thread::sleep_for(std::chrono::microseconds(1)); 
		//std::this_thread::sleep_for(std::chrono::seconds(1)); 
		//mySession->transfer("welcome to 2020",15);  
//		cpipe.broadcast("welcome to 2020");  
     	std::string msg = fmt::format("index: {} , ",index ++); 
		//mySession->msend_with_obdata(index , msg, std::string("msg from client "),std::string("msend"));
		mySession->transfer(msg); 
	}; 
	return 0; 
};
