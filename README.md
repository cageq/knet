![C/C++ CI](https://github.com/cageq/knet/workflows/C/C++%20CI/badge.svg)

# KNet 
Simple morden c++ network library wrapper based on asio standalone version, provide very simple APIs to build your network applications. 


## Build 
need c++11 to compile it. 

It is a headonly library, basically you can copy all files to you project and use it. (except fmt lib, you can alse set LOG_LEVEL to 0 in klog.hpp to get rid of the dependence ) 


## build samples
```shell

git submodule init 

git submodule update --recursive 

cmake . 

make -j4 

```



## Tcp Server 

Create a tcp server , you need define a tcp session inherited  to TcpConnection class, with the session type , you can create the listener. 
Start it with the port. now you get a discard tcp server. 

```cpp

#include "knet.hpp"
using namespace knet::tcp; 
class TcpSession : public TcpConnection<TcpSession> {
      public:

}; 

TcpListener<TcpSession> listener;
listener.start(8899); 

```


## Tcp Client 
It's almost the same code with the server, you need define a tcp session , then connect to server will create a session for you. 

```cpp 
#include "knet.hpp"
using namespace knet::tcp; 
class TcpSession : public TcpConnection<TcpSession> {
	public:

}; 

TcpConnector<TcpSession>  connector;
connector.start(); 
connector.add_connection("127.0.0.1", 8899);
```



## UDP/KCP/HTTP/WEBSOCK

​	It  provides the  "connection-base" UDP/KCP protocol implements, but also has  the same apis with tcp. you can peek the  code in samples directory. 

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
As a server, most of the time , we need a manager to manage all the incoming sessions, so you need create a factory to handle all sessions' events . 

you can create a factory, then handle all sessions' event in the factory instance. if you have used factory ,  you cann't get data or events in your own session again. 

```cpp 

class MyFactory: public ConnFactory<TcpSession> { 
// TcpSession is your real session class  to process your session events and data 
	public:
		virtual void destroy(TPtr conn) {
			dlog("connection factory destroy connection in my factory "); 
		}	

		virtual void handle_event(TPtr conn, knet::NetEvent evt) {
			ilog("handle event in connection my factory"); 
		}

		virtual bool handle_data(TPtr conn, char * data, uint32_t len) { 
			conn->send(data,len); 
			return true ;
		}; 

}; 

	MyFactory factory; 
	dlog("start server");
  // you need create a factory instance and pass it to listener.
	TcpListener<TcpSession,MyFactory> listener(&factory);
	int port = 8899;
	listener.start(  port); 

```



## Thread mode 

​	You can create one or multi EventWorker(s) to process the connections, but we will keep one connecion's events always be in one thread of its lifecycle.  so it is safty to create a lua engine in your session, all net events will be called in the same thread.  



## Backends 

​	There are serval backends implements including the raw epoll/kqueue/iocp api, the open source version is based on standalone asio version. The goal of this project is to provide simple api for user to build your network components.  Will it can help.



## Performance 

   I have not test the performance of this library, but it's a very thin wrapper over the asio, I think the performace will be close to asio. 



## github & gitee   

https://github.com/cageq/knet 

https://gitee.com/cageq/knet  



## welcome to contribute

  If you think it's a little useful,  welcome all of you to make it better. 



## Thinking about coding style

​	I prefer good coding style , but not must-be coding style. code is also a language , it expresses your ideas, your thinking about it work for. basically I follow these rules to make code clear. 

1. class name will be camel name , with the first capitalize letter  : 
	TcpConnection , TcpServer , TcpListener
2. method name will lower case with "_" connects all words together  
3. class members are hard to name, google add postfix "_" , some add prefix "m_" to the word . I don't like them all.  



​	All members are part of the class, inner the class, it also a world to express something, I wish it be naturally.  In this library, I didn't add any prefix or postfix to members, sometimes it mix with the methods' name or other names, It's not a good naming style. 

​	I start a new branch to put some members into a "m" struct of the class.  so all access to the members should using "m." , it looks like a namespace, but namespace can't be used inner class. 
It works and looks better, but there are insufficient that can't init members in class initialization list, and the performace will be lower? I have not  test it. If you have better ideas for naming, we can have a discuss. 









