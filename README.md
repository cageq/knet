
# KNet 
Simple morden c++ network library wrapper based on asio standalone version, provide very simple APIs to build your network applications. 


## Build 
It uses the some api like std::string_view for http parser, so we need c++17 to compile it. 

the library contains a simple colorful log library to print debug log and it depends fmt library to accelerate the log output.

you can use scripts "setup.sh" to install dependence libs into deps directory in library root directory, no root permissions  needed. 

it is a headonly library, basically you can copy all files to you project and use it. (except fmt lib, you can set LOG_LEVEL to 0 in klog.hpp to get rid of the dependence ) 


## build samples
```shell

./setup.sh   # copy asio files and fmt lib to deps directory, no need root permissions. 

cmake . 

make -j4 

```



## Tcp Server 

Create a tcp server , you need define a tcp session inherited  to TcpConnection class, with the session type , you can create the listener. 
start it with the port. now you get a discard tcp server. 

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
class TcpSession : public TcpConnection<TcpSession> {
	public:

}; 

TcpConnector<TcpSession>  connector;
connector.start(); 
connector.add_connection("127.0.0.1", 8899);
``` 
	
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

		uint32_t process_data(const std::string & msg , knet::MessageStatus status)
		{
			//    trace;
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


class MyFactory: public ConnectionFactory<TcpSession> { // TcpSession is your real session class  to process your session events and data 

	public:

		virtual void destroy(TPtr conn) {
			dlog("connection factory destroy connection in my factory "); 
		}	

		virtual void handle_event(TPtr conn, knet::NetEvent evt) {
			ilog("handle event in connection my factory"); 
		}

		virtual int32_t handle_data(TPtr conn, char * data, uint32_t len) { 
			conn->send(data,len); 
			return len ;
		}; 

}; 

	MyFactory factory; 
	dlog("start server");
	TcpListener<TcpSession,MyFactory> listener(&factory);// you need create a factory instance and pass it to listener.
	int port = 8899;
	listener.start(  port); 

```


## Thread mode 
	You can create one or multi EventWorker(s) to process the connections, but we will keep one connecion's events always be in one thread of its lifecycle.  
so it is safty to create a lua engine in your session, all net events will be called in the same thread.  


## Backends 
    There are serval backends implements including the raw epoll/kqueue/iocp api, the open source version is based on standalone asio version. The goal of this project is to provide simple api for user to build your network components.  Will it can help.

## Performance 
   I have not test the performance of this library, but it's a very thin wrapper over the asio, I think the performace will be close to asio. 

## github and gitee  address 
  [github]: https://github.com/cageq/knet "github main"
  [gitee]: https://gitee.com/fatihwk/knet "gitee mirror"


## Thanks . 
  Welcome all of you to make it better. 





