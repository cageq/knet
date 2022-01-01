

## KNet 
 
 简单的网络编程库封装，基于ASIO standalone版本封装，核心特点是设计了极简API，让网络编程变得非常简单。服务端使用了“1 => n”线程模型，监听一个现场，多个工作线程（n >= 0), 单个连接的所有事件和数据回调都在一个线程。尽可能提升服务器性能。API部分都是线程安全的，放心使用。支持headonly使用，包含头文件即可。 


## 编译示例代码

  库本身不需要编译，内置了一些示例代码，可以进行编译。 最新版本使用了C++17语言的一些特性，需要支持C++17的编译器和CMake工具。 由于国内访问github困难，内置了依赖的第三方包，如fmt，spdlog等,可以直接使用根目录下的./build.sh 可自动展开进行编译（需要先安装unzip等工具）。

```shell

git submodule init 

git submodule update --recursive 

cmake . 

make -j4 

# or 
./build.sh 

```


## 创建Tcp服务器

```cpp

#include "knet.hpp"
using namespace knet::tcp;
/*
//示例协议头
struct UserMsgHead {
    uint32_t length; //body length 
    uint32_t type;
    char     data[0];  
}; 
*/
class TcpSession : public TcpConnection<TcpSession> {
    public:
        //计算出一个包的整体长度，如果长度不够返回0或负数
        virtual int32_t handle_package(const char * data, uint32_t len ){
            /*
            if (len  < sizeof(UserMsgHead)){
                return 0; 
            }
            UserMsgHead * head = (UserMsgHead*) data;  
            return head->length + sizeof(UserMsgHead);   
            */ 
            return len  ; 
        }

        //处理收到的网络事件
        virtual bool handle_event(NetEvent evt) 
        { 
            return true; 
        }

        //处理一个网络协议包，通过handle_package 计算出来的整个包
        virtual bool handle_data(const std::string &msg )
        {
            return true; 
        }

}; 

/*
class MyFactory: public knet::KNetFactory<TcpSession>{
    public:
	    virtual void on_create(TPtr ptr) {

			dlog("connection created event in my factory "); 
		}

		virtual void on_release(TPtr ptr) { 
			dlog("connection release event in my factory "); 
		}


}; 
MyFactory factory; 

可以使用Factory工厂类进行连接实例的管理，从创建到释放整个生命周期。
TcpListener<TcpSession> listener(&factory); //这里使用原生指针主要是方便继承类
*/

TcpListener<TcpSession> listener;
listener.start(8899); 

```



## 创建Tcp客户端

```cpp

//使用和服务器相同的TcpSession 

TcpConnector<TcpSession>  connector;
connector.start(); 
connector.add_connection("127.0.0.1", 8899);


```



## UDP/KCP/HTTP/WEBSOCK

对UDP/KCP/HTTP/WebSocket 进行了简单的封装，可以初步使用。 

