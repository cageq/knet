//***************************************************************
//    created:    2020/08/01
//    author:        arthur wong
//***************************************************************

#pragma once
#include <string>
#include <memory>


#include <asio.hpp>
#include "utils/knet_log.hpp"
#include "utils/timer.hpp"
#include "ikcp.h"
#include "kcp_message.hpp"
#include "knet_worker.hpp"
#include "knet_handler.hpp"
#include "utils/loop_buffer.hpp"
#include <set> 
namespace knet {
namespace kcp {

using asio::ip::udp;
using namespace knet::utils;
using namespace std::chrono;

using KcpSocketPtr = std::shared_ptr<udp::socket>;

inline std::string addrstr(udp::endpoint pt) {
    return pt.address().to_string() + ":" + std::to_string(pt.port());
}
enum KcpPackageType {
    KCP_PACKAGE_PING,
    KCP_PACKAGE_PONG,
    KCP_PACKAGE_USER,
}; 

template <typename T>
class KcpConnection : public std::enable_shared_from_this<T> {
public:
    enum ConnectionStatus { CONN_IDLE, CONN_OPEN, CONN_KCP_READY, CONN_CLOSING, CONN_CLOSED };
    using TPtr = std::shared_ptr<T>; 
    KcpConnection(){
        static std::atomic<uint64_t>  index = 1024;
        cid = ++index; 
    } 
    void init(KNetWorkerPtr w){
        event_worker = w;  
        shakehand_request_.cmd = KCP_SHAKEHAND_REQUEST;
        shakehand_response_.cmd = KCP_SHAKEHAND_RESPONSE;
        disconnect_message_.cmd = KCP_GOODBYE;
        heartbeat_message_.cmd = KCP_HEARTBEAT;
        conn_status = CONN_IDLE;
    }
 
    int32_t send(const char * data ,uint32_t len){
        knet_dlog("send user message on conn_status {}", static_cast<uint32_t>(conn_status));
        if (conn_status == CONN_KCP_READY) {
            if (kcp) {
                return ikcp_send(kcp, data, len);
            }
        } else {
            this->shakehand_request();
        }
        return -1;
    }

    int32_t send(const std::string& msg) {
        return send(msg.data(), msg.length()); 
    }
      template <typename P >  
        inline uint32_t write_data( const P & data  ){
            return send_buffer.push((const char*)&data, sizeof(P));  
        }  
        inline uint32_t  write_data(const std::string_view &  data ){
            return send_buffer.push(data.data(), data.length());  
        }

        inline uint32_t write_data(const std::string &  data ){
            return send_buffer.push(data.data(), data.length());  
        }

        inline uint32_t write_data(const char* data ){
            if (data != nullptr ){ 
                return send_buffer.push(data , strlen(data));  
            }
            return 0;                         
        }

    template <class P, class... Args>
        int32_t msend(const P &first, const Args &...rest)
        {
            std::string sendBuffer; // calc all params size 
            return mpush(sendBuffer, first, rest...);
        }

    template <typename F, typename... Args>
        int32_t mpush(  const F &data, Args... rest)
        {
            this->write_data(   data);
            return mpush(  rest...);
        }

    int32_t mpush( )
    {
        if (conn_status == CONN_KCP_READY) {
            if (kcp) {
                auto sentLen = send_buffer.read([this ](const char * data, uint32_t dataLen){
                    return ikcp_send(kcp, data, dataLen);
                }); 
                return sentLen; 
            }
        } else {
            this->shakehand_request();
        }
        return -1;
    }

    virtual int32_t handle_package(const char* data, uint32_t len) { 
        knet_dlog("on recv kcp message {} , length {}", data, len);
        return len;
    }

    virtual bool handle_event(NetEvent evt) 
    { 
        return true; 
    }


    int32_t disconnect() {

        if (kcp_sock && conn_status < CONN_CLOSING) {
            conn_status = CONN_CLOSING;
            disconnect_message_.conv = cid;
            auto self = this->shared_from_this(); 

            if (kcp_timerid != 0 ) {
                event_worker->stop_timer(kcp_timerid);
                kcp_timerid = 0; 
            }

            kcp_sock->async_send_to(
                asio::buffer((const char*)&disconnect_message_, sizeof(KcpShakeHandMsg)),
                remote_point, [=](std::error_code ec, std::size_t len /*bytes_sent*/) {
                    if (ec) {                        
                        knet_dlog("sent message , error code {}, {}", ec.value(), ec.message());
                    }
                    self->handle_event(EVT_DISCONNECT);
 
                    self->release();
                }); 
        

            return 0;
        }
        return -1;
    } 

    inline uint64_t get_cid() const { return cid; }
    uint32_t cid = 0;
    std::chrono::milliseconds last_msg_time;

private:
    template <class,class, class >
    friend class KcpListener;
    template <class,class, class >
    friend class KcpConnector;

    void init_kcp() { 
        knet_dlog("create kcp cid is {}", cid);
        if (kcp != nullptr) {
            ikcp_release(kcp);
            kcp = nullptr;
        }

        if (kcp == nullptr) {
            kcp = ikcp_create(cid, this);
            kcp->output = [](const char* buf, int len, ikcpcb* kcp, void* user) {
                auto self = static_cast<KcpConnection<T>*>(user);
                self->do_send(buf, len);
                return 0;
            };
            ikcp_nodelay(kcp, 1, 10, 1, 1);
            // ikcp_nodelay(kcp, 0, 40, 0, 0);
            if (kcp_timerid == 0) {
                kcp_timerid = event_worker->start_timer(  [this]() {
                        if (conn_status == CONN_KCP_READY && kcp) {
                            auto timeNow = std::chrono::system_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                timeNow.time_since_epoch());
 
                            ikcp_update(kcp, duration.count());
                        }
                        return true; 
                    },
                    100);
            }
        }

        conn_status = CONN_KCP_READY;
        knet_dlog("init kcp success , change conn_status  to ready {}", static_cast<uint32_t>(conn_status) );
    } 

    int32_t do_sync_send(const char* data, std::size_t len)
    {
        if (kcp_sock) {
                asio::error_code ec;
                knet_dlog("send message {} to {}:{}", len, remote_point.address().to_string(),
                remote_point.port());
                int32_t ret = kcp_sock->send_to(asio::const_buffer(data, len), remote_point,0, ec); 
 
                if (!ec) {
                    return ret;
                }
        }
        return -1; 
    }

    int32_t do_send(const char* data, std::size_t length) {
        if (kcp_sock) {
            knet_dlog("send message {} to {}:{}", length, remote_point.address().to_string(),
                remote_point.port()); 
    
            auto buffer = std::make_shared<std::string>(data, length);
            auto self = this->shared_from_this(); 
            kcp_sock->async_send_to(asio::buffer(*buffer), remote_point,
                [self, buffer](std::error_code ec, std::size_t len /*bytes_sent*/) {
                    if (!ec) { 
                        self->handle_event(EVT_SEND);
 
                    } else {
                        knet_dlog("sent message , error code {}, {}", ec.value(), ec.message());
                    }
                 
                });

            return 0;
        }
        return -1;
    }
 
    void release() {

        if (conn_status != CONN_CLOSED) {
            conn_status = CONN_CLOSED;
            knet_dlog("release all resources {}", this->cid);
            event_worker->stop_timer(heartbeat_timerid);
            event_worker->stop_timer(kcp_timerid);

            if (kcp_sock && !is_passive) {
                kcp_sock->close();
                kcp_sock = nullptr;
            }

            if (kcp) {
                ikcp_release(kcp);
                kcp = nullptr;
            }    
        }
    }

    void listen(KcpSocketPtr s) {
        kcp_sock = s;
        conn_status = CONN_OPEN;
        is_passive = true;
        if (kcp_sock) {
            auto self = this->shared_from_this(); 
            heartbeat_timerid = event_worker->start_timer( [=]() {
                    auto timeNow = std::chrono::system_clock::now();
                    auto elapseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timeNow.time_since_epoch() - last_msg_time);

                    if (elapseTime.count() > 3000) {
                        knet_wlog("check heartbeat failed, closing connection"); 
                        self->handle_event(EVT_DISCONNECT);  
                        self->release(); 
                    } else {
                        self->send_heartbeat();
                    }

                    //knet_ilog("check heartbeat timer {}", elapseTime.count());
                    return true; 
                },
                1000000);
        }
    }

    void connect( udp::endpoint pt, bool reconn = true) {
        remote_point = pt;
        is_passive = false;
        reconnect_flag = reconn;
        this->kcp_sock = std::make_shared<udp::socket>(event_worker->context(), udp::endpoint(udp::v4(), 0));
        if (kcp_sock) {
            auto self = this->shared_from_this(); 
            heartbeat_timerid = event_worker->start_timer( [=]() {
                    auto timeNow = std::chrono::system_clock::now();
                    auto elapseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timeNow.time_since_epoch() - last_msg_time);

                    if (elapseTime.count() > 3000) {
                        knet_wlog("check heartbeat failed, closing connection");  
                        self->handle_event(EVT_DISCONNECT);  
                        if(!reconnect_flag)
                        {
                            self->release();
                        }
                    } else {
                        this->send_heartbeat();
                    }

                    //knet_ilog("check heartbeat timer {}", elapseTime.count());
                    return true; 
                }, 1000000);
            conn_status = CONN_OPEN;
            shakehand_request();
            do_receive();
        }
    }

    void shakehand_request(uint32_t id = 0) {

        if (kcp_sock) {
            shakehand_request_.conv = id == 0 ? this->cid : id;
            knet_dlog("send shakehand request to client {}", shakehand_request_.conv);
            kcp_sock->async_send_to(
                asio::buffer((const char*)&shakehand_request_, sizeof(KcpShakeHandMsg)),
                remote_point, [this](std::error_code ec, std::size_t len /*bytes_sent*/) {
                    if (!ec) {
                        knet_dlog("send shakehand request successful {}", shakehand_request_.conv);
                    } else {
                        knet_dlog("sent message , error code {}, {}", ec.value(), ec.message());
                    }
                });
        } else {
            knet_elog("socket is not ready");
        }
    }

 

    void shakehand_response(uint32_t id = 0) {

        if (kcp_sock) { 
            shakehand_response_.conv = id == 0 ? this->cid : id;
            kcp_sock->async_send_to(
                asio::buffer((const char*)&shakehand_response_, sizeof(KcpShakeHandMsg)),
                remote_point, [this](std::error_code ec, std::size_t len /*bytes_sent*/) {
                    if (!ec) {
                    //knet_dlog("send to remote point {}", remote_point); 
                        knet_dlog("send shakehand response successful  {}", shakehand_response_.conv);
                    } else {
                        knet_dlog("sent message , error code {}, {}", ec.value(), ec.message());
                    }
                });
        }
    }

    void send_heartbeat() {
        if (kcp_sock) {
            heartbeat_message_.conv = this->cid;
            kcp_sock->async_send_to(
                asio::buffer((const char*)&heartbeat_message_, sizeof(KcpHeartbeat)), remote_point,
                [this](std::error_code ec, std::size_t len /*bytes_sent*/) {
                    if (!ec) {
                        //knet_dlog("send heartbeat message successful {}", shakehand_response_.conv);
                    } else {
                        knet_dlog("sent message , error code {}, {}", ec.value(), ec.message());
                    }
                });
        }
    }
    bool check_control_message(const char* pData, uint32_t len) {
        static uint32_t server_conv_index = 0x1000; 
        KcpMsgHead* head = (KcpMsgHead*)pData;

        //knet_dlog("check shakehand conn_status is {}, type is {}", conn_status, head->cmd);
        if (conn_status == CONN_OPEN) {

            switch (head->cmd) {
            case KCP_SHAKEHAND_REQUEST: {
                KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
                if (is_passive && shakeMsg->conv == 0 && this->cid == 0) {
                    this->cid = server_conv_index++;
                    knet_wlog("allocate cid to client {}", this->cid);
                }

                this->cid = shakeMsg->conv == 0 ? this->cid : shakeMsg->conv;
                knet_dlog("get shakehand request, cid is {} ", shakeMsg->conv);
                if (this->cid != 0) {
                    this->shakehand_response(this->cid);
                    this->init_kcp();
                    // if (event_handler) {
                    //     event_handler(this->shared_from_this(), EVT_CONNECT, {});
                    // }
                    handle_event(EVT_CONNECT); 
                }

            } break;
            case KCP_SHAKEHAND_RESPONSE: {

                KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
                knet_dlog("get shakehand response on open conn_status, cid is {}", shakeMsg->conv);
                if (this->cid == 0) {
                    knet_wlog("update cid from other side {}", shakeMsg->conv);
                    this->cid = shakeMsg->conv;
                }

                this->init_kcp();
                // if (event_handler) {
                //     event_handler(this->shared_from_this(), EVT_CONNECT, {});
                // }
                handle_event(EVT_CONNECT); 
            } break;
            case KCP_HEARTBEAT: {
                knet_dlog("receive heartbeat , ignore it ");
            } break;

            default:
                knet_wlog("on open state, need shakehand, send shakehand request ");
                this->shakehand_request(); // lost cid ,request cid again
                return true; // ignore message on open state , treat it as control message
            }

        } else if (conn_status == CONN_KCP_READY) {
            switch (head->cmd) {
            case KCP_SHAKEHAND_REQUEST: {
                KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
                knet_wlog("get shakehand request when kcp ready, cid is {} ", shakeMsg->conv);
                if (shakeMsg->conv == 0) {
                    knet_wlog("request shakehand cid is 0 ,using my cid {}", this->cid);
                } else {
                    if (this->cid == 0) {
                        this->cid = shakeMsg->conv;
                    }
                }
                conn_status = CONN_OPEN;
                this->init_kcp();
                this->shakehand_response(this->cid); // report my cid again
            } break;
            case KCP_SHAKEHAND_RESPONSE: {
                KcpShakeHandMsg* shakeMsg = (KcpShakeHandMsg*)pData;
                knet_wlog("get shakehand response on kcp ready conn_status, cid is {} ", shakeMsg->conv);
                if (this->cid == 0) {
                    this->cid = shakeMsg->conv;
                }
                this->init_kcp();
            } break;

            case KCP_HEARTBEAT: {
                //knet_dlog("receive heartbeat, ignore it on ready");
            } break;
            default:;
            }
        }
        return (head->cmd == KCP_SHAKEHAND_REQUEST || head->cmd == KCP_SHAKEHAND_RESPONSE ||
                head->cmd == KCP_HEARTBEAT);
    }

    void do_receive() {
 
        kcp_sock->async_receive_from(asio::buffer(recv_buffer, kMaxMessageLength), sender_point,
            [this](std::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    knet_dlog("receive message from {} length {}", sender_point.address().to_string(), bytes_recvd);
                    recv_buffer[bytes_recvd] = 0;

                    auto timeNow = std::chrono::system_clock::now();
                    last_msg_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timeNow.time_since_epoch());

                    bool ctrlMsg = check_control_message((const char*)recv_buffer, bytes_recvd); 
                    if (!ctrlMsg) {
                        handle_receive((const char*)recv_buffer, bytes_recvd);
                    }
                    do_receive();
                } else {
                    knet_elog("async receive error {}, {}", ec.value(), ec.message());
                }
            });
    }

    void handle_receive(const char* data, std::size_t dataLen) {

 
        if (kcp) {
            int32_t cmd = ikcp_input(kcp, data, dataLen);
            // knet_dlog("kcp input command  {}", cmd);

            if (cmd == IKCP_CMD_ACK) {
                //knet_ilog("it's ack command ,ignore it");
                return;
            }
            uint32_t recvLen = 0; 
            while (recvLen < dataLen)
            {
                int32_t msgLen = ikcp_peeksize(kcp);
                if (msgLen <= 0 ) {
                    break; 
                }

                char kcpBuf[kMaxMessageLength] = "";
                int32_t kcp_recvd_bytes = ikcp_recv(kcp, kcpBuf, sizeof(kcpBuf));
                if (kcp_recvd_bytes >  0) {
                    recvLen += msgLen ; 
            //        knet_dlog("received kcp message length {}", kcp_recvd_bytes);
                    this->handle_package(kcpBuf, kcp_recvd_bytes);
                    // if (event_handler) {
                    //     event_handler(this->shared_from_this(), EVT_RECV, {data, dataLen});
                    // }

                    handle_event(EVT_RECV); 
                } else {
                    break; 
                }
            }; 
        }
    }
 

    KcpSocketPtr kcp_sock; 
    enum { kMaxMessageLength = 4096 };
    char recv_buffer[kMaxMessageLength]; 
    ConnectionStatus conn_status;

    udp::endpoint sender_point;
    udp::endpoint remote_point;
 
    
    std::mutex write_mutex; 
    LoopBuffer<8192> send_buffer; 
    bool reconnect_flag = false;
    KcpShakeHandMsg shakehand_request_;
    KcpShakeHandMsg shakehand_response_;
    KcpShakeHandMsg disconnect_message_;
    KcpHeartbeat heartbeat_message_;
    bool is_passive = false; 
    KNetWorkerPtr event_worker;  
    std::set<uint64_t> conn_timers;
    ikcpcb* kcp = nullptr;
    uint64_t kcp_timerid = 0;
    uint64_t heartbeat_timerid = 0;
    
};

} // namespace udp
} // namespace knet
