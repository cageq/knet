
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
