//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once


#include <memory>
#include <string>
#include <chrono>
#include <type_traits>
#include <string_view> 
#include "utils/knet_url.hpp"
#include "knet_worker.hpp"
#include <asio/use_future.hpp>

using namespace knet::utils; 

namespace knet {
    namespace tcp {

        using asio::ip::tcp;
        template <class T>
            class TcpSocket : public std::enable_shared_from_this<TcpSocket<T>> {
                public:
                    enum class SocketStatus {
                        SOCKET_IDLE,
                        SOCKET_INIT = 1,
                        SOCKET_CONNECTING, 
                        SOCKET_OPEN,
                        SOCKET_CLOSING,
                        SOCKET_RECONNECT,
                        SOCKET_CLOSED,
                    };

                    enum { kReadBufferSize = 1024* 16, kMaxPackageLimit = 16 * 1024  };
                    using TPtr = std::shared_ptr<T>;//NOTICY not weak_ptr 
                    TcpSocket(const std::thread::id& tid, knet::KNetWorkerPtr worker , void * = nullptr): io_worker(worker), 
                          tcp_sock(worker->context()) {
                              worker_tid = tid;
                              socket_status = SocketStatus::SOCKET_INIT; 
                              send_buffer.reserve(1024*4); 
                              cache_buffer.reserve(1024*4); 
                    }

                    TcpSocket(tcp::socket sock, const std::thread::id& tid, knet::KNetWorkerPtr worker , void * = nullptr): 
                         io_worker(worker),  tcp_sock(std::move(sock)) {
                              worker_tid = tid;
                              socket_status = SocketStatus::SOCKET_INIT; 
                              send_buffer.reserve(1024*4); 
                              cache_buffer.reserve(1024*4); 
                    }

                    inline void init(TPtr conn, NetOptions opts) {
                        net_options = opts; 
                        connection = conn;
                    }

                    inline bool connect(){
                        return connect(remote_url ); 
                    }

                    bool connect(const KNetUrl & urlInfo  ) {                       
                        remote_url = urlInfo; 
                        tcp::resolver resolver(io_worker->context()); 
                        tcp::resolver::results_type addrResult ; 
                        try {
                            addrResult = resolver.resolve(urlInfo.host, std::to_string(urlInfo.port));
                            if (addrResult.empty()){
                                wlog("resolve address {} failed", urlInfo.host); 
                                return false; 
                            }
                            dlog("resolve result :"); 
                            for(auto &e : addrResult){
                                dlog("  ip {}", e.endpoint().address().to_string()); 
                            }
                        }catch(... ){
                            wlog("resolve address {}:{} failed", urlInfo.host, urlInfo.port); 
                            return false; 
                        }
                        dlog("connect to server {}:{}", urlInfo.host, urlInfo.port);
                        
                        tcp_sock.close(); 
                        tcp_sock = tcp::socket(io_worker->context()); 
                        if ( urlInfo.has("bind_port")) {
                            std::string bindAddr = urlInfo.get("bind_addr"); 
                            std::string bindPort = urlInfo.get("bind_port"); 
                            asio::ip::tcp::endpoint laddr(asio::ip::make_address(bindAddr), std::stoi(bindPort) );
                            tcp_sock.bind(laddr);
                        }   
                     
                        auto self = this->shared_from_this();
                        socket_status = SocketStatus::SOCKET_CONNECTING;  
                        async_connect(tcp_sock, addrResult,
                            [self, this ](asio::error_code ec, typename decltype(addrResult)::endpoint_type endpoint) {
                            if (!ec) {
	                                if (!self->net_options.tcp_delay)
	                                {
	                                    self->tcp_sock.set_option(asio::ip::tcp::no_delay(true));        
	                                }

                                    if (net_options.send_buffer_size > 0){
                                        asio::socket_base::send_buffer_size SNDBUF(net_options.send_buffer_size);
                                        tcp_sock.set_option(SNDBUF);
                                    }
                                    if (net_options.recv_buffer_size > 0){
                                        asio::socket_base::receive_buffer_size RCVBUF(net_options.recv_buffer_size);
                                        tcp_sock.set_option(RCVBUF);
                                    }
                                    //self->tcp_sock.set_option(asio::socket_base::keep_alive(true));
                                
                                dlog("connect to {}:{} success {}",remote_url.host,remote_url.port, self->tcp_sock.is_open());       
                                self->init_read(false); 
                                                                    
                            }else {
                                ilog("connect to {}:{} failed, error : {}", remote_url.host, remote_url.port, ec.message() );
                                self->tcp_sock.close();
                                self->socket_status = SocketStatus::SOCKET_CLOSED; 
                            }
                        }); 

                        return true; 
                    
                    }

                    template <class F>
                        void run_inloop(const F & fn) {
                            asio::dispatch(io_worker->context(), fn);
                        }

                    void init_read(bool isPassive){
                        if (connection) {
                            socket_status = SocketStatus::SOCKET_OPEN;
                            connection->process_event(EVT_CONNECT);
                            do_read(); 
                        }				
                    }
 

                    void do_read() {
                        if (tcp_sock.is_open() ) {					                           
                            auto self = this->shared_from_this();
                            auto buf = asio::buffer( &read_buffer[read_buffer_pos], kReadBufferSize - read_buffer_pos);
                            if (kReadBufferSize > read_buffer_pos){
                                tcp_sock.async_read_some(buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
                                    if (!ec) {                                        				 
                                        process_data(bytes_transferred);
                                        self->do_read();
                                    }else {
                                        ilog("read error, close connection {} , reason : {} ", ec.value(), ec.message() );
                                        cache_buffer.clear(); //drop cache buffer
                                        do_close();
                                    }
                                });

                            }else {						
                                
                                process_data(0);
                                if (read_buffer_pos >= kReadBufferSize ){
                                    wlog("read buffer {} is full, check package size, read pos is {}", static_cast<uint32_t>(kReadBufferSize), read_buffer_pos); 
                                    //packet size exceed the limit, so we close it. 
                                    this->do_close();                                                 
                                }						
                            }	
                        } else {
                            ilog("socket is not open");
                        }
                    }

                    int32_t sync_send(const char* pData, uint32_t dataLen) {
                        if (is_open() ) {					
                           
                            try {
                                auto ret = asio::write(tcp_sock, asio::const_buffer(pData, dataLen));
                                if (ret >0  && connection){
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                elog("sync_send write failed {}", ex.what()); 
                            }
                        }
                        return -1; 
                    }

                    
                    int32_t sync_sendv(const std::vector<std::string_view > & buffers) {
                        if (is_open() ) {					                       
                            std::vector<asio::const_buffer> asioBuffers( buffers.size());                   
			                for(const auto & buf : buffers){
			                    asioBuffers.push_back(asio::const_buffer(buf.data(), buf.length())); 
			                }
                            try {
                                auto ret = asio::write(tcp_sock, asioBuffers);
                                if (ret >0  && connection){
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                elog("sync_sendv write failed {}", ex.what()); 
                            }
                        }
                        return -1; 
                    }


 
                    int32_t sync_sendv(const std::vector<std::string > & buffers) {
                        if (is_open() ) {					                       
                            std::vector<asio::const_buffer> asioBuffers( buffers.size());                   
			                for(const auto & buf : buffers){
			                    asioBuffers.push_back(asio::const_buffer(buf.data(), buf.length())); 
			                }
                            try {
                                auto ret =  asio::write(tcp_sock, asioBuffers);
                                if (ret > 0 &&  connection)
                                {
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                elog("sync_sendv write failed {}", ex.what()); 
                            }
                        }
                        return -1; 
                    }


                    template <class P, class... Args>
                        int32_t sync_msend(const P& first, const Args&... rest)  {
                            if (is_open() ) {	
                                return mpush_sync(first, rest ...); 
                            }
                            return 0; 
                        }

                     template <typename F, typename ... Args>
                        int32_t mpush_sync(const F &  data, Args... rest) { 					
                            this->write_data(data  ) ;                            
                            return   mpush_sync(rest...);
                        }

                     int32_t mpush_sync() {                        
                         try {
                             auto ret =  asio::write(tcp_sock, asio::const_buffer(send_buffer.data(), send_buffer.length()));                        
                             send_buffer.clear(); 
                             if (ret > 0 && connection)
                             {
                                 connection->process_event(EVT_SEND);
                             }

                             return ret; 
                        }  catch(const  asio::system_error &ex ){
                            elog("mpush_sync failed {}", ex.what()); 
                        }
                        return -1; 
                     }



                    inline int32_t send(const char* pData, uint32_t dataLen) { 
                        return msend(std::string_view(pData, dataLen));
                    }

                    inline int32_t send(const std::string_view& msg) {
                        return msend(msg);
                    }

                    template <class P, class... Args>
                        int32_t msend(const P& first, Args&& ... rest) {
                            if (is_open()){ 
								 
                                int32_t sent = 0; 
                                {
                                    std::lock_guard<std::mutex> lk(write_mutex);  
                                    sent = this->mpush(first, rest...);
                                }  
                                do_send(); 
                                return sent; 
                                
                            }
                            return -1;                         
                        }

                    template <typename P >  
                        inline uint32_t write_data( const P & data  ){
                            send_buffer.append(std::string_view((const char*)&data, sizeof(P)));
                            return send_buffer.size(); 
                        } 


                    inline uint32_t  write_data(const std::string_view &  data ){
                        send_buffer.append(data ); 
                        return send_buffer.size(); 
                    }

                    inline uint32_t write_data(const std::string &  data ){
                        send_buffer.append(data ); 
                        return send_buffer.size(); 
                    }

                    inline uint32_t write_data(const char* data ){
                        if (data != nullptr ){
                            send_buffer.append(std::string_view(data) ); 
                            return send_buffer.size(); 
                        }
                        return 0;                         
                    }  

                    template <typename F, typename ... Args>
                        int32_t mpush(const F &  data, Args && ... rest) { 					
                           auto len = this->write_data(data  ) ;                            
                           return len > 0?len + mpush(std::forward<Args&&>(rest)...):0;
                        }
                    
                    int32_t mpush(){
                        return 0; 
                    }

                    bool  is_writting {false}; 
                    int32_t do_send() {  			 
						/*
						auto self = this->shared_from_this();
                        asio::post(io_worker->context(), [self,this]() { 
                                if (!is_writting){
                                self->do_async_write(); 
                                }
                            }); 					 
							*/
						if (!is_writting){
							this->do_async_write(); 
						}

                        return 0; 
                    }
 

                    int32_t do_async_write() { 

                         {
							std::lock_guard<std::mutex> lk(write_mutex); 
							if(!send_buffer.empty()){
								if (cache_buffer.empty() ){
									send_buffer.swap(cache_buffer);   
								}
							}
						}
                        if (cache_buffer.empty()){
                            is_writting = false; 
							return 0; 
						} 
        
                        auto self = this->shared_from_this();
                        asio::const_buffer wBuf(cache_buffer.data(), cache_buffer.size()); 
                        is_writting = true; 
                        asio::async_write(tcp_sock,wBuf, [this, self](std::error_code ec, std::size_t length) {
                                if (!ec ) {
                                    if (connection)
                                    {
                                        connection->process_event(EVT_SEND);
                                	}
                                    cache_buffer.clear();                                 
                                    self->do_async_write();                          
                                }else {
                                    ilog("write error, status is {}", static_cast<uint32_t>(self->socket_status)); 
                                    cache_buffer.clear(); //drop cache buffer
                                    self->do_close();
                                }
                            });
                        return 0;  
                    }
 

                    inline bool is_open() {
                        return tcp_sock.is_open() && socket_status == SocketStatus::SOCKET_OPEN;
                    }
                    inline bool is_connecting() {
                        return  socket_status == SocketStatus::SOCKET_CONNECTING;
                    } 


                    bool process_data(uint32_t nread) {
                        read_buffer_pos += nread; 

                        int32_t readPos = 0; 
                        while(readPos < read_buffer_pos  && connection){   
                            int32_t pkgLen = this->connection->process_package(&read_buffer[readPos], read_buffer_pos);  
                            if ( pkgLen <= 0 || pkgLen >  read_buffer_pos - readPos ){
                                if (pkgLen > kReadBufferSize) {
                                //   wlog("single package size {} exceeds max buffer size ({}) , check package size", pkgLen, kReadBufferSize);
                                    this->do_close(); 
                                    return false;
                                }                                
                                break; 
                            } 
                            connection->process_data(&read_buffer[readPos], pkgLen);
                            readPos += pkgLen; 
                        }

                        if (readPos < read_buffer_pos ) {                            
                            memmove(read_buffer, &read_buffer[readPos ] , read_buffer_pos - readPos);    
                        } 

                        read_buffer_pos -= readPos;                        
                        return true;              
                    } 

                    void close() { 
                        if (socket_status == SocketStatus::SOCKET_CLOSING || socket_status == SocketStatus::SOCKET_CLOSED) {
                            wlog("close, status is {}", static_cast<uint32_t>(socket_status));
                            return;
                        }

                        auto self = this->shared_from_this();
                        asio::post(io_worker->context(), [this, self]() { 
                                self->socket_status = SocketStatus::SOCKET_CLOSING;
                                auto & conn = self->connection; 
                                if (conn) {
                                    if (!send_buffer.empty()) {
                                        do_async_write(); //try last write
                                    }
                                    if (self->socket_status <=  SocketStatus::SOCKET_CLOSING ){
                                        conn->process_event(EVT_DISCONNECT); 
                                    }
                                    
                                    self->socket_status = SocketStatus::SOCKET_CLOSED;
                                    if (tcp_sock.is_open()) {
                                        tcp_sock.close();
                                    }
                        
                                    if (conn->need_reconnect()) {
                                        socket_status = SocketStatus::SOCKET_RECONNECT;
                                    }else {
                                        conn->release();
                                        conn.reset(); 
                                    }
                                } else {
                                    self->socket_status = SocketStatus::SOCKET_CLOSED;
                                    if (tcp_sock.is_open()) {
                                        tcp_sock.close();							
                                    } 
                                }
                                self->read_buffer_pos = 0;
                        });

                    }

                    void do_close( ) {			 
                        if (socket_status == SocketStatus::SOCKET_CLOSING || socket_status == SocketStatus::SOCKET_CLOSED) {
                            wlog("do close, in status {}", static_cast<uint32_t>(socket_status) );
                            return;
                        }  
                        if (socket_status < SocketStatus::SOCKET_CLOSING){
                            socket_status = SocketStatus::SOCKET_CLOSING;   
                        }  
 
                        if (connection) {
							send_buffer.clear();
                            cache_buffer.clear(); //drop cache data
                            if (socket_status <=  SocketStatus::SOCKET_CLOSING ){
                                connection->process_event(EVT_DISCONNECT); 
                            }			
                            if (connection->need_reconnect()) {
                                socket_status = SocketStatus::SOCKET_RECONNECT;
                            }else {
                                socket_status = SocketStatus::SOCKET_CLOSED;
                                if (tcp_sock.is_open()) {
                                    tcp_sock.close();
                                }
                                connection->release();
                                connection.reset();
                            }
                        }else {
                            socket_status = SocketStatus::SOCKET_CLOSED;
                            if (tcp_sock.is_open()) {
                                tcp_sock.close();							
                            } 
                        } 

                        read_buffer_pos = 0;
                    }

                    inline tcp::endpoint local_endpoint() {
                        return tcp_sock.local_endpoint();
                    }

                    inline tcp::endpoint remote_endpoint() {
                        return tcp_sock.remote_endpoint();
                    }

                    inline tcp::socket& socket() { return tcp_sock; }

                    inline bool is_inloop() const  {
						thread_local auto tid =  std::this_thread::get_id(); 
                        return worker_tid == tid ;
                    }
                    void set_remote_url(const KNetUrl & urlInfo){
                        remote_url = urlInfo; 
                    }
                private:
                    knet::KNetWorkerPtr  io_worker;
                    tcp::socket tcp_sock;		
                    TPtr connection;		
                    char read_buffer[kReadBufferSize+4];
                    int32_t read_buffer_pos = 0;
                    std::mutex write_mutex; 
                    std::string send_buffer; 
                    std::string cache_buffer; 
                    SocketStatus socket_status = SocketStatus::SOCKET_IDLE;
                    std::thread::id worker_tid;
                    KNetUrl remote_url; 
                    NetOptions net_options; 
            };


    } // namespace tcp

}; // namespace knet
