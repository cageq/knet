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
                    enum { kReadBufferSize = 1024 * 8, kMaxPackageLimit = 8 * 1024  };
                    using TPtr = std::shared_ptr<T>;//NOTICY not weak_ptr 
                    TcpSocket(const std::thread::id& tid, asio::io_context& ctx, void* = nullptr)
                        : io_context(ctx)
                          , tcp_sock(ctx) {
                              worker_tid = tid;
                              socket_status = SocketStatus::SOCKET_INIT; 
                              send_buffer.reserve(1024); 
                              cache_buffer.reserve(1024); 
                          }

                    inline void init(TPtr conn) {
                        connection = conn;
                    }

                    bool connect(const std::string& host, uint32_t port, const std::string& localAddr = "0.0.0.0", uint32_t localPort = 0) {
                        tcp::resolver resolver(io_context);
                        auto result = resolver.resolve(host, std::to_string(port));
                        dlog("connect to server {}:{}", host.c_str(), port);
                        auto self = this->shared_from_this();
                        tcp_sock.close(); 
                        tcp_sock = std::move(tcp::socket(io_context)); 
                        if (localPort > 0) {
                            asio::ip::tcp::endpoint laddr(asio::ip::make_address(localAddr), localPort);
                            tcp_sock.bind(laddr);
                        }
                        socket_status = SocketStatus::SOCKET_CONNECTING; 
                        async_connect(tcp_sock, result,
                                [self, host, port ](asio::error_code ec, typename decltype(result)::endpoint_type endpoint) {
                                if (!ec) {
                                    if (!self->connection->net_options.tcp_delay)
                                    {
                                        self->tcp_sock.set_option(asio::ip::tcp::no_delay(true));        
                                    }
                                
                                    dlog("connect to {}:{} success",host,port); 
                                    if (!self->connection->net_options.sync){
                                        self->init_read(); 
                                    }
                                    
                                }else {
                                   dlog("connect to server failed, {}:{} , error : {}", host.c_str(), port, ec.message() );
                                    self->tcp_sock.close();
                                    self->socket_status = SocketStatus::SOCKET_CLOSED; 
                                }
                                });


                        return true;
                    }

                    template <class F>
                        void run_inloop(const F & fn) {
                            asio::dispatch(io_context, fn);
                        }
                    void init_read(){
                        if (connection) {
                            connection->process_event(EVT_CONNECT);
                            do_read(); 
                        }				
                    }
                    int32_t do_sync_read(const std::function<int32_t (const char * data, uint32_t len) > & handler){
                        if(tcp_sock.is_open()){
                            asio::error_code error;
                            
                            size_t len = tcp_sock.read_some(asio::buffer( (char*)read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos), error);

                            if (error == asio::error::eof)
                            {
                                return -1; 

                            } else if (error)
                            {
                                //throw asio::system_error(error); // Some other error.
                                return -1; 
                            }

                            read_buffer_pos += len ; 

                            uint32_t readLen = handler((char *) read_buffer, read_buffer_pos); 
                            if (readLen < read_buffer_pos) {
                                memmove((char*)read_buffer, (char *)read_buffer + readLen, read_buffer_pos - readLen); 
                                read_buffer_pos = read_buffer_pos - readLen; 
                            }

                            return len; 
                        }

                        return 0; 
                    }

                    void do_read() {
                        if (tcp_sock.is_open() ) {					
                            socket_status = SocketStatus::SOCKET_OPEN;
                            auto self = this->shared_from_this();
                            auto buf = asio::buffer((char*)read_buffer + read_buffer_pos, kReadBufferSize - read_buffer_pos);
                            if (kReadBufferSize > read_buffer_pos){
                                tcp_sock.async_read_some(
                                        buf, [this, self](std::error_code ec, std::size_t bytes_transferred) {
                                        if (!ec) {
                                        //dlog("received data length {}", bytes_transferred);								 
                                        process_data(bytes_transferred);
                                        self->do_read();
                                        }
                                        else {
                                        elog("read error, close connection {} , reason : {} ", ec.value(), ec.message() );
                                        do_close();
                                        }
                                        });

                            }else {						
                                wlog("read buffer {} is full, increase your receive buffer size,read pos is {}", kReadBufferSize, read_buffer_pos); 
                                process_data(0);
                                if (read_buffer_pos >= kReadBufferSize ){
                                    //packet size exceed the limit, so we close it. 
                                    self->do_close();
                                }						
                            }	
                        }
                        else {
                            dlog("socket is not open");
                        }
                    }

                    int32_t send_inloop(const char* pData, uint32_t dataLen) {
                        if (tcp_sock.is_open() ) {					
                            asio::async_write(tcp_sock, asio::buffer(pData, dataLen), [this](std::error_code ec, std::size_t length) {
                                    if (ec) {
                                    elog("send in loop error : {} , {}", ec.value(), ec.message());
                                    this->do_close();
                                    }
                                    });

                            //asio::write(tcp_sock, asio::const_buffer(pData, dataLen));
                            return 0;
                        }
                        return -1; 
                    }


                    int32_t send(const char* pData, uint32_t dataLen) { 
                        if (is_inloop()) {
                            return send_inloop(pData, dataLen);
                        }
                        return msend(std::string_view(pData, dataLen));
                    }

                    int32_t send(const std::string_view& msg) {
                        if (is_inloop()) {
                            return send_inloop(msg.data(), msg.length());
                        }
                        return msend(msg);
                    }

                    template <class P, class... Args>
                        int32_t msend(const P& first, const Args&... rest) {
                            write_mutex.lock(); 
                            send_buffer.clear(); 
                            return this->mpush(first, rest...);
                        }

                    template <typename P >  
                        inline int32_t write_data( const P & data  ){
                            send_buffer.append(std::string_view((const char*)&data, sizeof(P)));
                            return sizeof(P); 
                        } 


                    inline int32_t  write_data(const std::string_view &  data ){
                        send_buffer.append(data ); 
                        return std::size(data); 
                    }

                    inline int32_t write_data(const std::string &  data ){
                        send_buffer.append(data ); 
                        return std::size(data); 
                    }

                    inline int32_t write_data(const char* data ){
                        if (data != nullptr ){
                            send_buffer.append(std::string(data) ); 
                            return strlen(data); 
                        }
                        return 0;                         
                    }
 
                    template <typename F, typename ... Args>
                        int32_t mpush(const F &  data, Args... rest) { 					
                           int32_t ret =   this->write_data(data  ) ;                            
                           int32_t lastRet =  mpush(rest...);
                           return lastRet <0?lastRet:ret; 
                        }

                    int32_t mpush() {
                        write_mutex.unlock(); 
                        if (!this->is_open()){
                            return -1; 
                        }
                        auto self = this->shared_from_this();
                        //so cache_buffer is safe in network loop thread only
                        asio::dispatch(io_context, [this, self]() {
                                if (!this->is_open()) {
                                     elog("socket is not open"); 
                                    return ; 
                                }
                                if (cache_buffer.empty())
                                {
                                    if (write_mutex.try_lock()) {
                                        send_buffer.swap(cache_buffer);
                                        write_mutex.unlock();
                                    } 
                                }   
                                if (!cache_buffer.empty()){
                                //One or more buffers containing the data to be written. Although the buffers object may be copied as necessary, 
                                //ownership of the underlying memory blocks is retained by the caller, 
                                //which must guarantee that they remain valid until the handler is called.
                                asio::async_write(tcp_sock, asio::const_buffer(cache_buffer.data(), cache_buffer.length()), [this](std::error_code ec, std::size_t length) {
                                        if (!ec){
                                        cache_buffer.clear(); 
                                        }else 
                                        {
                                        elog("send error : {} , {}", ec.value(), ec.message());
                                        this->do_close();
                                        }
                                        });

                                //Write all of the supplied data to a stream before returning.
                                //asio::write(self->tcp_sock, asio::const_buffer(cache_buffer.data(), cache_buffer.size()));
                                //cache_buffer.clear(); 
                                }							

                        });
                        return 0; 
                    }

                    bool do_async_write() { 

                        if (!cache_buffer.empty())
                        {
                            auto self = this->shared_from_this();
                            asio::async_write(tcp_sock,asio::buffer(cache_buffer.data(), cache_buffer.size()),
                                    [this, self](std::error_code ec, std::size_t length) {
                                    if (!ec && tcp_sock.is_open() && length > 0) {
                                    //	connection->process_event(EVT_SEND);
                                    cache_buffer.clear(); 
                                    if (send_buffer.size() == 0) {
                                    return;
                                    }
                                    self->do_async_write();
                                    }else {
                                    dlog("write error , do close , socket_status is {}", self->socket_status); 
                                    self->do_close();
                                    }
                                    });

                        }
                        return true;
                    }

                    inline bool is_open() {
                        return tcp_sock.is_open() && socket_status == SocketStatus::SOCKET_OPEN;
                    }
                    inline bool is_connecting() {
                        return  socket_status == SocketStatus::SOCKET_CONNECTING;
                    }

                    void rewind_buffer(int32_t readPos){
                        if (read_buffer_pos >= readPos) {
                            //dlog("rewind buffer to front {} ", read_buffer_pos - readPos);
                            memmove(read_buffer, (const char*)read_buffer + readPos, read_buffer_pos - readPos);
                            read_buffer_pos -= readPos;
                        }
                    }

                    bool process_data(uint32_t nread) {
                        if (!connection  || nread <= 0) {
                            return false;
                        }
                        connection->process_event(EVT_RECV);				
                        read_buffer_pos += nread; 
                        int32_t pkgLen = this->connection->process_package((char*)read_buffer, read_buffer_pos); 

                        //package size is larger than data we have 
                        if (pkgLen >  read_buffer_pos){
                            if (pkgLen > kReadBufferSize) {
                                elog("single package size {} exceeds max buffer size ({}) , please increase it", pkgLen, kReadBufferSize);
                                this->do_close(); 
                                return false;
                            }
                            //dlog("need more data to get one package"); 
                            return true; 
                        }

                        int32_t readPos = 0;
                        while (pkgLen > 0) {
                            //dlog("process data size {} ,read buffer pos {}  readPos {}", pkgLen, read_buffer_pos, readPos);
                            if (readPos + pkgLen <= read_buffer_pos) {
                                char* pkgEnd = (char*)read_buffer + readPos + pkgLen + 1;
                                char endChar = *pkgEnd;
                                *pkgEnd = 0;
                                this->connection->process_data(std::string((const char*)read_buffer + readPos, pkgLen));
                                *pkgEnd = endChar;
                                readPos += pkgLen;
                            } else {
                                if (read_buffer_pos > readPos )
                                {
                                    rewind_buffer(readPos);
                                    break;
                                } 
                            }

                            if (readPos < read_buffer_pos) {
                                int32_t  dataLen = read_buffer_pos - readPos; 
                                pkgLen = this->connection->process_package( (char*)read_buffer + readPos, dataLen); 	
                                if (pkgLen <= 0 ||  pkgLen > dataLen) {

                                    if (pkgLen > kReadBufferSize) {
                                        elog("single package size {} exceeds max buffer size ({}) , please increase it", pkgLen, kReadBufferSize);
                                        this->do_close(); 
                                        return false; 
                                    }

                                    rewind_buffer(readPos);
                                    break;
                                }  

                            }else {
                                read_buffer_pos = 0;
                                break;
                            }
                        } 
                        return true; 
                    }

                    void close() { 

                        if (socket_status == SocketStatus::SOCKET_CLOSING || socket_status == SocketStatus::SOCKET_CLOSED) {
                            dlog("close, already in closing socket_status {}", socket_status);
                            return;
                        }

                        auto self = this->shared_from_this();
                        asio::post(io_context, [this, self]() { 

                                self->socket_status = SocketStatus::SOCKET_CLOSING;
                                auto & conn = self->connection; 
                                if (conn) {
                                if (send_buffer.size() > 0 && !send_buffer.empty()) {
                                do_async_write(); //try last write
                                }
                                conn->process_event(EVT_DISCONNECT); 
                                self->socket_status = SocketStatus::SOCKET_CLOSED;
                                if (tcp_sock.is_open()) {
                                tcp_sock.close();
                                }
                                conn->release();
                                conn.reset(); 
                                }else {
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
                            dlog("do close, already in closing socket_status {}", socket_status);
                            return;
                        }  
                        socket_status = SocketStatus::SOCKET_CLOSING;   
                        auto & conn = connection; 
                        if (conn) {
                            conn->process_event(EVT_DISCONNECT);					
                            if (conn->need_reconnect()) {
                                socket_status = SocketStatus::SOCKET_RECONNECT;
                            }else {
                                socket_status = SocketStatus::SOCKET_CLOSED;
                                if (tcp_sock.is_open()) {
                                    tcp_sock.close();
                                }
                                conn->release();
                                conn.reset();
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

                    inline asio::io_context& get_context() { return io_context; }
                private:
                    asio::io_context& io_context;
                    tcp::socket tcp_sock;		
                    TPtr connection;		

                    char read_buffer[kReadBufferSize+4];
                    int32_t read_buffer_pos = 0;
                    std::mutex write_mutex; 
                    std::string send_buffer; 
                    std::string cache_buffer; 
                    SocketStatus socket_status = SocketStatus::SOCKET_IDLE;
                    std::thread::id worker_tid;
            };


    } // namespace tcp

}; // namespace knet
