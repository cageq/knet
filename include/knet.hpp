//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#ifndef __KNET_H__
#define __KNET_H__

#include <thread>
#include "klog.hpp"
#include <functional>
#include "event_handler.hpp"
namespace knet
{


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
