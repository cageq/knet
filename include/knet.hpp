//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#ifndef __KNET_H__
#define __KNET_H__

#include <thread>
#include <functional>
#include "utils/knet_log.hpp"
#include "utils/knet_url.hpp"
#include "knet_handler.hpp"
namespace knet
{


	struct NetOptions
	{
        bool tcp_delay{true}; 
		bool sync {false};
		bool reuse { true} ;
		uint32_t backlogs = 512;								// listening queue size
		uint32_t threads = 1; // std::thread::hardware_concurrency()  ; // iocp/epoll worker threads number
		uint32_t send_buffer_size = 16 * 1024;
		uint32_t recv_buffer_size = 14 * 1024;
		uint32_t recv_size = 8 * 1024;
		std::string chain_file = "cert/server.pem";
		std::string dh_file = "cert/dh2048.pem";
	};

    inline NetOptions options_from_url(const utils::KNetUrl & urlInfo){
        NetOptions opts ;
        opts.tcp_delay = urlInfo.get<bool>("tcp_delay"); 
        opts.sync = urlInfo.get<bool>("sync"); 
        opts.reuse = urlInfo.get<bool>("reuse"); 
        opts.backlogs = urlInfo.get<uint32_t >("backlogs"); 
        opts.send_buffer_size = urlInfo.get<uint32_t >("sbuf_size"); 
        opts.recv_buffer_size = urlInfo.get<uint32_t >("rbuf_size"); 
        opts.chain_file = urlInfo.get("cert"); 
        opts.dh_file    = urlInfo.get("cert_key"); 
        return opts; 
    }

    inline NetOptions options_from_url(const std::string & url){
        utils::KNetUrl urlInfo(url); 
        return options_from_url(urlInfo); 
    }


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
