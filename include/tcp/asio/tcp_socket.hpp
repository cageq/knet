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
#include "utils/loop_buffer.hpp"
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

                    enum { kReadBufferSize = 1024 * 16, kMaxPackageLimit = 16 * 1024  };
                    using TPtr = std::shared_ptr<T>;//NOTICY not weak_ptr 
                    TcpSocket(const std::thread::id& tid, knet::KNetWorkerPtr worker , void * = nullptr): io_worker(worker), 
                          tcp_sock(worker->context()) {
                              worker_tid = tid;
                              socket_status = SocketStatus::SOCKET_INIT; 
                            
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
                        if (!net_options.sync){
                               async_connect(tcp_sock, addrResult,
                                    [self, this ](asio::error_code ec, typename decltype(addrResult)::endpoint_type endpoint) {
                                    if (!ec) {
                                        if (!self->net_options.tcp_delay)
                                        {
                                            self->tcp_sock.set_option(asio::ip::tcp::no_delay(true));        
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
                        }else {                                      
                            try {
                                std::future<asio::ip::tcp::endpoint> cf =  async_connect(tcp_sock, addrResult, asio::use_future); 
                                cf.get();             
                                if (!net_options.tcp_delay) {
                                    tcp_sock.set_option(asio::ip::tcp::no_delay(true));        
                                }                    
                                socket_status = SocketStatus::SOCKET_OPEN;
                            }catch(std::system_error& e){
                                ilog("connect to {}:{} failed, error : {}", remote_url.host, remote_url.port, e.what() );
                                self->tcp_sock.close();
                                self->socket_status = SocketStatus::SOCKET_CLOSED; 
                                return false; 
                            }                          
                        }                     
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

                    int32_t do_sync_read(const std::function<int32_t (const char * data, uint32_t len) > & handler){
                        if(tcp_sock.is_open()){
                            asio::error_code error;                            
                            asio::mutable_buffer  readBuffer ( (char*)read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos); 
                            std::future<size_t> len = tcp_sock.async_read_some(readBuffer, asio::use_future);

                            // if (error == asio::error::eof)
                            // {
                            //     return -1; 

                            // } else if (error)
                            // {
                            //     //throw asio::system_error(error); // Some other error.
                            //     return -1; 
                            // }
                            //len.wait(); 
                            size_t dataLen = len.get(); 
                            read_buffer_pos += dataLen ; 

                            int32_t readLen = handler((char *) read_buffer, read_buffer_pos); 
                            if (readLen > 0 && readLen < read_buffer_pos) {
                                memmove((char*)read_buffer, (char *)read_buffer + readLen, read_buffer_pos - readLen); 
                                read_buffer_pos = read_buffer_pos - readLen; 
                            }
                            return dataLen; 
                        }

                        return 0; 
                    }

                    void do_read() {
                        if (tcp_sock.is_open() ) {					                           
                            auto self = this->shared_from_this();
                            auto buf = asio::buffer((char*)read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos);
                            if (kReadBufferSize > read_buffer_pos){
                                tcp_sock.async_read_some(buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
                                    if (!ec) {                                        				 
                                        process_data(bytes_transferred);
                                        self->do_read();
                                    }else {
                                        ilog("read error, close connection {} , reason : {} ", ec.value(), ec.message() ); 
                                        send_buffer.clear(); 
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
                            /*
                            asio::async_write(tcp_sock, asio::const_buffer(pData, dataLen), [this](std::error_code ec, std::size_t length) {
                                    if (ec) {
                                        ilog("send in loop error : {} , {}", ec.value(), ec.message());
                                        this->do_close();
                                    }
                                    });
                                    */
                            // std::future<std::size_t> sentLen =  asio::async_write(tcp_sock,asio::const_buffer(pData, dataLen),  asio::use_future);   
                            // return  sentLen.get(); // Blocks until the send is complete. Throws any errors.

                            try {
                                return asio::write(tcp_sock, asio::const_buffer(pData, dataLen));
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
                                return asio::write(tcp_sock, asioBuffers);
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
                                return asio::write(tcp_sock, asioBuffers);
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
                            auto ret = send_buffer.pop([this](const char * data, uint32_t dataLen){
                                 asio::write(tcp_sock, asio::const_buffer(data, dataLen));                        
                            }); 
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
                        int32_t msend(const P& first, const Args&... rest) {
                            if (is_open()){
                                write_mutex.lock();                 
                                return this->mpush(first, rest...);
                            }
                            return -1;                         
                        }

                    template <typename P >  
                        inline uint32_t write_data( const P & data  ){ 
                            return send_buffer.push((const char*)&data, sizeof(P)); 
                             
                        } 


                    inline uint32_t  write_data(const std::string_view &  data ){
                        return send_buffer.push(data.data(), data.size()); 
                      
                    }

                    inline uint32_t write_data(const std::string &  data ){
                        return send_buffer.push(data.data(), data.size()); 
                      
                    }

                    inline uint32_t write_data(const char* data ){
                        if (data != nullptr ){
                            return send_buffer.push(data,  strlen(data));  
                        }
                        return 0;                         
                    }
 
                    template <typename F, typename ... Args>
                        int32_t mpush(const F &  data, Args... rest) { 					
                            this->write_data(data  ) ;   
 
         
                            return mpush(rest...);
                        }

                    int32_t mpush() {
                        this->write_mutex.unlock();	 
                        if (!send_buffer.empty() ){ 	 
                            //#define USING_ASYNC_SEND 1 
                            #ifdef USING_ASYNC_SEND
                                do_async_send(); 	     	            
                            #else 						
                                return do_async_write(); 
                            #endif 
                        }else {
                             
                        }                        
                        return 0; 
                    }

                    int32_t do_async_write() {  
                        auto ret = send_buffer.read([this](const char * data, uint32_t dataLen){
                            auto self = this->shared_from_this(); 
                            asio::async_write(tcp_sock,asio::const_buffer(data, dataLen), [this, self,dataLen ](std::error_code ec, std::size_t length) {
                                    if (!ec ) {
                                    //	connection->process_event(EVT_SEND);
                                        send_buffer.commit(length); //assert(dataLen == length ) 
                                       
                                        if (send_buffer.peek()) { 
                                            self->do_async_write();          
                                        }
                                                               
                                    }else {
                                        ilog("write error, status is {}", static_cast<uint32_t>(self->socket_status));  
                                        send_buffer.clear(); 
                                        self->do_close();
                                    }
                                });
                                return dataLen; 
                        }); 
                       
                        return ret;  
                    }
 
#ifdef USING_ASYNC_SEND
                    int32_t do_async_send() {     
                        auto ret = send_buffer.read([this](const char * data, uint32_t dataLen){ 
                            auto self = this->shared_from_this();
                            tcp_sock.async_send(asio::const_buffer(data, dataLen), [this, self](std::error_code ec, std::size_t length) {
                                if (!ec ) {  
                                        send_buffer.commit(length);  
                                        if (send_buffer.peek()) { 
                                            self->do_async_send();          
                                        } 
                                    }else {
                                        ilog("write error, status is {}", static_cast<uint32_t>(self->socket_status)); 
                                        send_buffer.clear(); //drop cache buffer
                                        self->do_close();
                                    }
                                });
                        });   
                        return ret;  
                    }
 #endif 

                    inline bool is_open() {
                        return tcp_sock.is_open() && socket_status == SocketStatus::SOCKET_OPEN;
                    }
                    inline bool is_connecting() {
                        return  socket_status == SocketStatus::SOCKET_CONNECTING;
                    } 


                    bool process_data(uint32_t nread) {
                        read_buffer_pos += nread; 

                        int32_t readPos = 0; 
                        while(readPos < read_buffer_pos){
                            int32_t pkgLen = this->connection->process_package(&read_buffer[readPos], read_buffer_pos); 
                            if ( pkgLen <= 0 || pkgLen >  read_buffer_pos){
                                if (pkgLen > kReadBufferSize) {
                                //   wlog("single package size {} exceeds max buffer size ({}) , check package size", pkgLen, kReadBufferSize);
                                    this->do_close(); 
                                    return false;
                                }                                
                                break; 
                            } 
                            connection->process_data(std::string(&read_buffer[readPos ], pkgLen));
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
                        return worker_tid == std::this_thread::get_id();
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
                    LoopBuffer<8192> send_buffer;  
                    SocketStatus socket_status = SocketStatus::SOCKET_IDLE;
                    std::thread::id worker_tid;
                    KNetUrl remote_url; 
                    NetOptions net_options; 
            };


    } // namespace tcp

}; // namespace knet
