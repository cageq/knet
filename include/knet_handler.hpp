#pragma once 

#include <string_view> 
namespace knet
{
    enum NetEvent
    {
        EVT_THREAD_INIT,
        EVT_THREAD_EXIT,
        EVT_LISTEN_START,
        EVT_LISTEN_FAIL,
        EVT_LISTEN_STOP,
        EVT_CREATE,
        EVT_RELEASE,
        EVT_CONNECT,
        EVT_CONNECT_FAIL,
        EVT_RECV,
        EVT_SEND,
        EVT_DISCONNECT,
        EVT_RECYLE_TIMER,
        EVT_USER1,
        EVT_USER2,
        EVT_END
    };

	static const char *kEventMessage[] = {
		"Net Event: thread init",
		"Net Event: thread exit",
		"Net Event: listen start",
		"Net Event: listen failed",
		"Net Event: listen stop",
		"Net Event: connection create",
		"Net Event: connection release",
		"Net Event: connect",
		"Net Event: connect failed",
		"Net Event: data received",
		"Net Event: data sent",
		"Net Event: disconnect",
		"Net Event: recyle timer timeout"};
	inline const char *event_string(NetEvent evt)
	{
		return kEventMessage[evt];
	}


    template <class T>
    class KNetHandler {
    public: 
        virtual bool handle_data(std::shared_ptr<T>, const std::string_view& msg) = 0;
        virtual bool handle_event(std::shared_ptr<T>, NetEvent) = 0; 
    };


}
