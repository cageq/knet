
#include "knet.hpp"

#include "websocket/wsock_ssl_client.hpp"
#include <iostream>

using namespace knet::tcp;
using namespace knet::websocket;


int main(int argc, char** argv)
{
	dlog("init websocket client "); 
	WSockSslClient  wsClient;  
	wsClient.start(); 




	WSockHandler<WSockSslConnection> wsHandler; 

	wsHandler.message = [](std::shared_ptr<WSockSslConnection> conn, const  std::string & msg , MessageStatus){

		wlog("on websocket message {}", msg); 
	}; 

	// wsconn->wsock_handler = wsHandler; 



	//auto wsconn =	wsClient.add_connection("ws://echo.websocket.org/echo" );
	auto wsconn = wsClient.add_connection("ws://127.0.0.1:8888/echo" , wsHandler);


	while(1){

		std::this_thread::sleep_for(std::chrono::milliseconds(5000)); 
		wsconn->send_text("hello world");  
		dlog("websocket client log "); 
	}

	//char c = getchar(); 
	//while (c)
	//{
	//	if (c == 'q')
	//	{
	//		break;
	//	}	
	//	c = getchar()	; 
	//}

	return 0;
}

