#include "websocket/wsock_ssl_server.hpp"



using namespace knet::websocket;

int main(int argc, char** argv)
{

	KNetLogIns.add_console(); 
 
	dlog("init websocket server "); 
	WSockSslServer<> wsServer; 

	WSockHandler<WSockSslConnection> wsHandler; 
	wsHandler.message = [](std::shared_ptr<WSockSslConnection> conn, const std::string_view &  msg){

		wlog("on websocket message:  {}", msg); 
	 	conn->send_text("hello client",OpCode::TEXT_FRAME,false); 
	}; 
	wsServer.register_router("/echo", wsHandler); 

	//wsServer.init_ssl("cert/server.pem", "cert/dh2048.pem"); 
	wsServer.init_ssl("ca/kstar.pem", "cert/dh2048.pem","Hello123"); 

	wsServer.start(9000,"0.0.0.0"); 
 
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
