//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#ifndef __KNET_H__
#define __KNET_H__

#include <thread>
#include "klog.hpp"
#include <functional>
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

	struct NetOptions
	{
		bool sync = false;
		bool reuse = true;
		uint16_t port = 9999;
		std::string host = "0.0.0.0"; 
	
		uint32_t backlogs = 512;								// listening queue size
		uint32_t threads = std::thread::hardware_concurrency(); // iocp/epoll worker threads number
		uint32_t send_buffer_size = 16 * 1024;
		uint32_t recv_buffer_size = 14 * 1024;
		uint32_t recv_size = 8 * 1024;
		std::string chain_file = "cert/server.pem";
		std::string dh_file = "cert/dh2048.pem";
	};

	enum class MessageStatus : uint32_t
	{
		MESSAGE_NONE = 0,
		MESSAGE_CHUNK = 1,
		MESSAGE_END = 2,
	};
} // namespace knet

#define USING_LOOP_BUFFER 0
#define USING_SINGLE_BUFFER 1
#define USING_DOUBLE_BUFFER 0

#include <asio.hpp>
#define USING_LIBGO_ENGINE 0
#define USING_ASIO_COROUTINE 0

#if __APPLE__

#if USING_ASIO_COROUTINE
#include "tcp/aco/tcp_connection.hpp"
#include "tcp/aco/tcp_listener.hpp"
#include "tcp/aco/tcp_connector.hpp"
#else
#include "tcp/asio/tcp_connection.hpp"
#include "tcp/asio/tcp_listener.hpp"
#include "tcp/asio/tcp_connector.hpp"
#endif

#else

#if USING_LIBGO_ENGINE
#include "tcp/co/tcp_connection.hpp"
#include "tcp/co/tcp_listener.hpp"
#include "tcp/co/tcp_connector.hpp"

#elif USING_ASIO_COROUTINE
#include "tcp/aco/tcp_connection.hpp"
#include "tcp/aco/tcp_listener.hpp"
#include "tcp/aco/tcp_connector.hpp"

#else
#include "tcp/asio/tcp_connection.hpp"
#include "tcp/asio/tcp_listener.hpp"
#include "tcp/asio/tcp_connector.hpp"
#endif

#endif

#endif
