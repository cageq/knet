
# KNet 
Simple morden c++ network library wrapper based on asio standalone version , provide simple apis to write network applications. 


## Build 
it uses the std::string_view api for http parser, so we need c++17 to compile it. 

the library contains a simple log library to print debug log. using fmt library to accelerate the log output.

you can use "setup.sh" to install dependence lib into deps directory.   

the library is headonly library, basically you can copy to you project and use it.


## build samples
```shell

./setup.sh   # install asio fmt lib to deps directory, no need root permissions. 

cmake . 

make -j4 

```



## Tcp Server 

create a tcp server , you need define a tcp session type,  with the session type , you can create the listener. 
start it with the port. now you get a discard tcp server. 

```cpp

#include "knet.hpp"
class TcpSession : public TcpConnection<TcpSession> {
      public:

}; 

TcpListener<TcpSession> listener;
listener.start(8899); 


```


## Tcp Client 
It's almost the same with the server code, you need define a tcp session first , then connect to server will create a session for you . 

```cpp 
	class TcpSession : public TcpConnection<TcpSession> {
		public:

	}; 


	TcpConnector<TcpSession>  connector;
	connector.start(); 
	connector.add_connection("127.0.0.1", 8899);
``` 
	
## Bind the event handler 
there are two different handler , event handler and data handler, you can bind your handlers in your session . 
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
most of the time , we need a manager to manage all the sessions, so you need create a factory to handle all connections' events . 

if you use your own factory, you cann't get data or events in your own session . 

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
you can create one or multi EventWorker to process the connections, but we will keep one connecion's events will always  be in one thread  in its lifecycle.  
for example   it is safe to create one lua engine in your session , all net events will be called in the same thread.  








