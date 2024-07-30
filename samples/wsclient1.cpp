
#include "knet.hpp"

#include "websocket/wsock_connection.hpp"
#include <iostream>

using namespace knet::tcp;
using namespace knet::websocket;


int main(int argc, char** argv)
{
	knet_dlog("init client "); 
	Connector<WSockConnection>  connector;  
	connector.start(); 

	for (int i = 0; i < 1; i++)
	{  
        //connector.add_wsconnection("127.0.0.1", 5020);
        //connector.add_wsconnection("127.0.0.1", 3000);
		connector.add_wsconnection("echo.websocket.org", 80,"/echo");
		//connector.add_connection("10.246.34.55", 8855);
	}

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

