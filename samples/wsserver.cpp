#include "websocket/wsock_server.hpp"



using namespace knet::websocket;

int main(int argc, char** argv)
{
	dlog("init websocket server "); 
	WSockServer<> wsServer; 

	WSockHandler<WSockConnection> wsHandler; 
	wsHandler.message = [](std::shared_ptr<WSockConnection> conn, const std::string &  msg ){

		wlog("on websocket message {}", msg); 
		conn->send_text("hello client",OpCode::TEXT_FRAME,false); 
		return true; 
	}; 
	wsServer.register_router("/echo", wsHandler); 

	wsServer.start(8888, "0.0.0.0"); 
 
	char c = getchar(); 
	while (c)
	{
		if (c == 'q')
		{
			break;
		}	
		c = getchar()	; 
	}

	return 0; 

}
