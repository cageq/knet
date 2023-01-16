#pragma once 
#include <string> 
#include <memory> 


namespace knet
{
    enum NetEvent
    {
        EVT_THREAD_INIT  = 0,
        EVT_THREAD_EXIT  = 1,
        EVT_LISTEN_START = 2,
        EVT_LISTEN_FAIL  = 3,
        EVT_LISTEN_STOP  = 4,
        EVT_CREATE       = 5,
        EVT_RELEASE      = 6 ,
        EVT_CONNECT      = 7 ,
        EVT_CONNECT_FAIL = 8 ,
        EVT_RECV         = 9 ,
        EVT_SEND         = 10,
        EVT_DISCONNECT   = 11,
        EVT_RECYLE_TIMER = 12,
        EVT_USER1        = 13,
        EVT_USER2        = 14,
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
        virtual bool handle_data(std::shared_ptr<T>, const std::string& msg) = 0;
        virtual bool handle_event(std::shared_ptr<T>, NetEvent) = 0; 
    };


}
