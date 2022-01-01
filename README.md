![C/C++ CI](https://github.com/cageq/knet/workflows/C/C++%20CI/badge.svg)

# KNet 
Simple morden c++ network library wrapper based on asio standalone version, provide simple APIs to build your network applications. 


## Build 
need c++17 supported compiler to compile it. 

It is a headonly library, basically you can copy all files to you project and use it. 


## build samples
```shell

git submodule init 

git submodule update --recursive 

cmake . 

make -j4 

# or 
./build.sh 

```

## Tcp Server 

Create a tcp server, you need define a tcp session inherited to TcpConnection class, with the session type , you can create the listener. 
Start it with the port. now you get a discard tcp server. 

```cpp

/*
//Sample protocol head 
struct UserMsgHead {
    uint32_t length; //body length 
    uint32_t type;
    char     data[0];  
}; 
*/

#include "knet.hpp"
using namespace knet::tcp; 
class TcpSession : public TcpConnection<TcpSession> {
      public:
		virtual int32_t handle_package(const char * data, uint32_t len ){
            /*
			//sample use message head 
            if (len  < sizeof(UserMsgHead)){
                return 0; 
            }
            UserMsgHead * head = (UserMsgHead*) data;  
            return head->length + sizeof(UserMsgHead);   
            */ 
            return len  ; 
        }

        //all net events
        virtual bool handle_event(NetEvent evt) 
        { 
            return true; 
        }

        //one whole message, divided by handle_package  
        virtual bool handle_data(const std::string &msg )
        {
            return true; 
        }
}; 

TcpListener<TcpSession> listener;
listener.start(8899); 

```


## Tcp Client 
It's almost the same code with the server, you need define a tcp session , then connect to server will create a session for you. or use add_connection() 

```cpp 
#include "knet.hpp"
using namespace knet::tcp; 

// TcpSession same with the server 

TcpConnector<TcpSession>  connector;
connector.start(); 
connector.add_connection("127.0.0.1", 8899);

```



## UDP/KCP/HTTP/WEBSOCK

​	It  provides the  "connection-base" UDP/KCP protocol implements, but also has the same apis with tcp.  
​	It have basic http/websocket protocol server-side implements,  and need more work to make it complete.



## Bind the event handler 

There are two different handlers , Event handler and Data handler, you can bind your handlers in your session . 
using the bind_event_handler and bind_data_handler to get data or connection events. 

```cpp 
class TcpSession : public TcpConnection<TcpSession > 
{
	public:
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 

		TcpSession() 
		{
			bind_data_handler(&TcpSession::process_data); 
			bind_event_handler([this](  TcpSessionPtr,knet::NetEvent ){

			std::string msg("hello world"); 
			this->send(msg.c_str(),msg.length()); 
					return 0; 
					} ); 
	
		}

		bool process_data(const std::string & msg )
		{
			dlog("received data {} ",msg); 
			this->send(msg.data(),msg.length());   
			return msg.length(); 
		}
}; 

```


## Session Factory 
As a server, we need a manager to control all the incoming sessions's lifetime, you can create a factory to handle all sessions' events . 

```cpp 

class MyFactory: public KNetFactory<TcpSession> { 
// TcpSession is your real session class  to process your session events and data 
	public:
		virtual void on_create(TPtr ptr) {

			dlog("connection created event in my factory "); 
		}

		virtual void on_release(TPtr ptr) { 
			dlog("connection release event in my factory "); 
		} 
		 
}; 

	MyFactory factory; 
	dlog("start server");
  	// create a factory instance and pass it to listener.
	TcpListener<TcpSession,MyFactory> listener(&factory);
	int port = 8899;
	listener.start(port); 

```



## Thread mode 

​	use one listener thread and zero or more worker threads.  we will keep one connecion's events always be in same thread. so it is safety to create a lua engine in your session, all net events will be called in the same thread.  

  

## github & gitee   

https://github.com/cageq/knet 

https://gitee.com/cageq/knet  


 