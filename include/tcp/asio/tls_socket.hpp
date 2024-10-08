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
#include "utils/knet_log.hpp"
#include "utils/string_thief.hpp"
#include "knet_worker.hpp"
 
#include <asio/ssl.hpp>
using namespace knet::utils; 
using namespace knet::log; 

namespace knet {
    namespace tcp {

        using asio::ip::tcp;
        template <class T>
            class TlsSocket : public std::enable_shared_from_this<TlsSocket<T>> {
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
                    enum { kReadBufferSize = 1024 * 8,kWriteBufferSize = 1024 * 16, kMaxPackageLimit = 8 * 1024  };
                    using TPtr = std::shared_ptr<T>;//NOTICY not weak_ptr 
                    TlsSocket(const std::thread::id& tid, knet::KNetWorkerPtr worker, const std::string & caFile = "cert/ca.pem")
                        : io_worker(worker)
                          , ssl_context(new asio::ssl::context(asio::ssl::context::sslv23))
		                  , sslsock(worker->context(), *ssl_context) {
                            worker_tid = worker->thread_id();
                            ssl_context->load_verify_file(caFile);
		                    sslsock.set_verify_mode(asio::ssl::verify_peer);
		                    sslsock.set_verify_callback(std::bind(
			                        &TlsSocket::verify_certificate, this, std::placeholders::_1, std::placeholders::_2));

                            socket_status = SocketStatus::SOCKET_INIT; 
          
                          }

                    TlsSocket(const std::thread::id& tid, knet::KNetWorkerPtr worker, void* sslCtx)
                        :io_worker(worker), sslsock(worker->context(), *(asio::ssl::context*)sslCtx) {
                        worker_tid = tid;
                        socket_status = SocketStatus::SOCKET_INIT; 
                        send_buffer.reserve(4*1024); 
                        cache_buffer.reserve(4*1024); 
                    }

                    TlsSocket( tcp::socket sock, const std::thread::id& tid, knet::KNetWorkerPtr worker, void* sslCtx)
                        :io_worker(worker), sslsock(std::move(sock), *(asio::ssl::context*)sslCtx) {
                        worker_tid = tid;
                        socket_status = SocketStatus::SOCKET_INIT; 
                        send_buffer.reserve(4*1024); 
                        cache_buffer.reserve(4*1024); 
                    }

                    inline void init(TPtr conn, NetOptions opts) {
                        net_options = opts; 
                        connection = conn;
                    }

                    inline bool connect(){
                        return connect(remote_url ); 
                    }

                    bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx) {
                        char subject_name[256];
                        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);                        
                        knet_dlog("verifying ca {}", subject_name); 
                        // return preverified;
                        return true;
                    }
                                    
                    bool connect(const KNetUrl & urlInfo  ) {                       
                        remote_url = urlInfo; 
                        tcp::resolver resolver(io_worker->context()); 
                        tcp::resolver::results_type addrResult ; 
                        try {
                            addrResult = resolver.resolve(urlInfo.host, std::to_string(urlInfo.port));
                            if (addrResult.empty()){
                                knet_wlog("resolve address {} failed", urlInfo.host); 
                                return false; 
                            }
                            knet_dlog("resolve result :"); 
                            for(auto &e : addrResult){
                                knet_dlog("  ip {}", e.endpoint().address().to_string()); 
                            }
                        }catch(... ){
                            knet_wlog("resolve address {}:{} failed", urlInfo.host, urlInfo.port); 
                            return false; 
                        }
                        knet_dlog("connect to server {}:{}", urlInfo.host, urlInfo.port);                        
                        //sslsock.lowest_layer().close(); 
                       // sslsock.lowest_layer() = std::move(tcp::socket(io_context)); 
                        if ( urlInfo.has("bind_port")) {
                            std::string bindAddr = urlInfo.get("bind_addr"); 
                            std::string bindPort = urlInfo.get("bind_port"); 
                            asio::ip::tcp::endpoint laddr(asio::ip::make_address(bindAddr), std::stoi(bindPort) );
                            sslsock.lowest_layer().bind(laddr);
                        }   
                     
                        auto self = this->shared_from_this();
                        socket_status = SocketStatus::SOCKET_CONNECTING; 
                       
                               async_connect(sslsock.lowest_layer(), addrResult,
                                    [self, this ](asio::error_code ec, typename decltype(addrResult)::endpoint_type endpoint) {
                                    if (!ec) {
                                        if (!self->net_options.tcp_delay)
                                        {
                                            self->sslsock.lowest_layer().set_option(asio::ip::tcp::no_delay(true));        
                                        }
                                        
                                        knet_dlog("connect to {}:{} success",remote_url.host,remote_url.port);       
                                        self->init_read(true); 
                                                                           
                                    }else {
                                        knet_dlog("connect to {}:{} failed, error : {}", remote_url.host, remote_url.port, ec.message() );
                                        self->sslsock.lowest_layer().close();
                                        self->socket_status = SocketStatus::SOCKET_CLOSED; 
                                    }
                                }); 

                        return true;  
                    }

                    void handshake(bool isClient = false) {
                        knet_dlog("start ssl handshake");
                       
                        auto self = TlsSocket<T>::shared_from_this();
                        sslsock.async_handshake(isClient ? asio::ssl::stream_base::client : asio::ssl::stream_base::server,
                            [self](const std::error_code& error) {
                                if (!error) {
                                    self->ssl_shakehand = true;
                                    knet_dlog("tls handshake success ");
                                    self->connection->handle_event(EVT_CONNECT);
                                    self->do_read();
                                } else {
                                    knet_elog("tls handshake failed {}", error.message());                                     
                                    self->sslsock.lowest_layer().close();
                                }
                            });
                    }

                    

                    template <class F>
                        void run_inloop(const F & fn) {
                            asio::dispatch(io_worker->context(), fn);
                        }

                    void init_read(bool isClient = false ){
                        if (connection) {                            
                            if (!ssl_shakehand) {
                                handshake(isClient);
                                return;
                            }
                            socket_status = SocketStatus::SOCKET_OPEN;
                            connection->process_event(EVT_CONNECT);
                            do_read(); 
                        }				
                    }


                    void do_read() {
                        if (sslsock.lowest_layer().is_open() ) {					                           
                            auto self = this->shared_from_this();
                            auto buf = asio::buffer( &read_buffer[read_buffer_pos], kReadBufferSize - read_buffer_pos);
                            if (kReadBufferSize > read_buffer_pos){
                                sslsock.async_read_some(buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
 
                                    if (!ec) {                                        				 
                                        process_data(bytes_transferred);
                                        self->do_read();
                                    }else {
                                        knet_ilog("read error, close connection {} , reason : {} ", ec.value(), ec.message() );
                                        //cache_buffer.clear(); //drop cache buffer
                                        string_resize(cache_buffer, 0); 
                                        do_close();
                                    }
                                });
                            }else {						
                                
                                process_data(0);
                                if (read_buffer_pos >= kReadBufferSize ){
                                    knet_wlog("read buffer {} is full, check package size, read pos is {}", static_cast<uint32_t>(kReadBufferSize), read_buffer_pos); 
                                    //packet size exceed the limit, so we close it. 
                                    this->do_close();                                                 
                                }						
                                
                            }	
                        } else {
                            knet_ilog("socket is not open");
                        }
                    }

                    int32_t sync_send(const char* pData, uint32_t dataLen) {
                        if (is_open() ) {	 
                            try {
                                auto ret = asio::write(sslsock, asio::const_buffer(pData, dataLen));
                                if (ret >0  && connection){
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                knet_elog("sync_send write failed {}", ex.what()); 
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
                                auto ret = asio::write(sslsock, asioBuffers);
                                if (ret >0  && connection){
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                knet_elog("sync_sendv write failed {}", ex.what()); 
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
                                auto ret =  asio::write(sslsock, asioBuffers);
                                if (ret > 0 &&  connection)
                                {
                                    connection->process_event(EVT_SEND);
                                }
                                return ret; 
                            }  catch(const  asio::system_error &ex ){
                                knet_elog("sync_sendv write failed {}", ex.what()); 
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
                             auto ret =  asio::write(sslsock, asio::const_buffer(send_buffer));                        
                             //send_buffer.clear(); 
                             string_resize(send_buffer,0); 
             
                             if (ret > 0 && connection)
                             {
                                 connection->process_event(EVT_SEND);
                             }
                             return ret; 
                        }  catch(const  asio::system_error &ex ){
                            knet_elog("mpush_sync failed {}", ex.what()); 
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
						auto self = this->shared_from_this();
                        asio::post(io_worker->context(), [self]() { 
                                if (!self->is_writting){
									self->do_async_write(); 
                                }
                            }); 					 
 
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
                        asio::async_write(sslsock,wBuf, [this, self](std::error_code ec, std::size_t length) {
                                if (!ec ) {
                                    if (connection)
                                    {
                                        connection->process_event(EVT_SEND);
                                	}
                                    //cache_buffer.clear();                                 
                                    string_resize(cache_buffer, 0); 
                                    self->do_async_write();                          
                                }else {
                                    knet_ilog("write error, status is {}", static_cast<uint32_t>(self->socket_status)); 
                                    //cache_buffer.clear(); //drop cache buffer
                                    string_resize(cache_buffer, 0); 
                                    self->do_close();
                                }
                            });
                        return 0;  
                    }
 

                    inline bool is_open() {
                        return sslsock.lowest_layer().is_open() && socket_status == SocketStatus::SOCKET_OPEN;
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
                                    knet_wlog("single package size {} exceeds max buffer size ({}) , check package size", pkgLen, static_cast<uint32_t>(kReadBufferSize));
                                    this->do_close(); 
                                    return false;
                                }                                
                                break; 
                            } 
                            connection->process_data( &read_buffer[readPos ], pkgLen);
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
                            knet_dlog("close, in status {}", static_cast<uint32_t>(socket_status));
                            return;
                        }

                        auto self = this->shared_from_this();
                        asio::post(io_worker->context(), [this, self]() { 

                                self->socket_status = SocketStatus::SOCKET_CLOSING;
                                auto & conn = self->connection; 
                                if (conn) {
                                    if (  !send_buffer.empty()) {
                                        do_async_write(); //try last write
                                    }
                                    if (self->socket_status <=  SocketStatus::SOCKET_CLOSING ){
                                        conn->process_event(EVT_DISCONNECT); 
                                    }
                                    self->socket_status = SocketStatus::SOCKET_CLOSED;
                                    if (sslsock.lowest_layer().is_open()) {
                                    sslsock.lowest_layer().close();
                                }
                                    if (conn->need_reconnect()) {
                                        socket_status = SocketStatus::SOCKET_RECONNECT;
                                    }else {
                                        conn->release();
                                        conn.reset(); 
                                    }
                                }else {
                                    self->socket_status = SocketStatus::SOCKET_CLOSED;
                                    if (sslsock.lowest_layer().is_open()) {
                                        sslsock.lowest_layer().close();							
                                    } 
                                }
                                self->read_buffer_pos = 0;
                        });

                    }

                    void do_close( ) {			 
                        if (socket_status == SocketStatus::SOCKET_CLOSING || socket_status == SocketStatus::SOCKET_CLOSED) {
                            knet_dlog("do close, already in closing socket_status {}", static_cast<uint32_t>(socket_status));
                            return;
                        }  
                        if (socket_status < SocketStatus::SOCKET_CLOSING){
                            socket_status = SocketStatus::SOCKET_CLOSING;   
                        } 
						auto & conn = connection; 
                        if (conn) {
							//send_buffer.clear();
                            string_resize(send_buffer, 0); 
                            //cache_buffer.clear(); //drop cache data
                            string_resize(cache_buffer, 0); 
                            if (socket_status <=  SocketStatus::SOCKET_CLOSING ){
                                conn->process_event(EVT_DISCONNECT); 
                            }			
                            if (conn->need_reconnect()) {
                                socket_status = SocketStatus::SOCKET_RECONNECT;
                            }else {
                                socket_status = SocketStatus::SOCKET_CLOSED;
                                if (sslsock.lowest_layer().is_open()) {
                                    sslsock.lowest_layer().close();
                                }
                                conn->release();
                                conn.reset();
                            }
                        }else {
                            socket_status = SocketStatus::SOCKET_CLOSED;
                            if (sslsock.lowest_layer().is_open()) {
                                sslsock.lowest_layer().close();							
                            } 
                        } 

                        read_buffer_pos = 0;
                    }

                    inline tcp::endpoint local_endpoint() {
                        return sslsock.lowest_layer().local_endpoint();
                    }

                    inline tcp::endpoint remote_endpoint() {
                        return sslsock.lowest_layer().remote_endpoint();
                    }

                    inline asio::basic_socket<asio::ip::tcp, asio::any_io_executor>& socket() {                        
                       return sslsock.lowest_layer();
                    }


                    inline bool is_inloop() const  {
                        return worker_tid == std::this_thread::get_id();
                    }

                  //  inline asio::io_context& get_context() { return io_context; }
                private:
                    knet::KNetWorkerPtr io_worker;      	
                    TPtr connection ;		
                    std::unique_ptr<asio::ssl::context> ssl_context;  
	                asio::ssl::stream<tcp::socket> sslsock;
	                bool ssl_shakehand = false;
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
