#include <stdio.h>
#include <iostream>
#include "knet.hpp"
#include "knet_handler.hpp"

using namespace knet::tcp;

class TcpSession : public TcpConnection<TcpSession>
{
public:
    typedef std::shared_ptr<TcpSession> TcpSessionPtr;
    TcpSession() {}

    virtual ~TcpSession()
    {
        knet_dlog("destroy tcp session");
    }
};

class MyFactory : public knet::KNetFactory<TcpSession>, public knet::KNetHandler<TcpSession>
{

public:
    virtual void destroy(ConnectionPtr conn)
    {
    }

    virtual void on_create(ConnectionPtr ptr)
    {

        knet_dlog("connection created event in my factory ");
    }

    virtual void on_release(ConnectionPtr ptr)
    {
        knet_dlog("connection release event in my factory ");
    }

    virtual bool handle_event(ConnectionPtr conn, knet::NetEvent evt)
    {
        knet_ilog("handle event in connection my factory {}", static_cast<uint32_t>(evt));
        return true;
    }

    virtual bool handle_data(ConnectionPtr conn, char *data, uint32_t dataLen) override
    {
        conn->send(data, dataLen);
        return dataLen;
    };
};

int main(int argc, char **argv)
{
    knet_add_console_sink();
    MyFactory factory;
    knet_dlog("start server");
    TcpListener<TcpSession, MyFactory> listener(&factory);
    int port = 8888;
    listener.start(port);
    //    listener->bind_event_handler( [](NetEvent evt, std::shared_ptr<TcpSession>) {
    //            switch (evt)
    //            {
    //            case EVT_CREATE:
    //            knet_dlog("on create connection");
    //            break;
    //            case EVT_CONNECT:
    //            knet_dlog("on connection established");
    //            break;
    //            case EVT_CONNECT_FAIL:
    //            break;
    //
    //            default:;
    //            }
    //
    //            return 0;
    //            });
    knet_dlog("start server on port {}", port);

    char c = getchar();
    while (c)
    {
        if (c == 'q')
        {
            printf("quiting ...\n");
            break;
        }
        c = getchar();
    }

    listener.stop();
    knet_dlog("quit server");
    return 0;
}
