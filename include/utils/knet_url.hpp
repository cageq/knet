
#pragma once 
#include <string> 
#include <vector>
#include <unordered_map>


namespace knet {
namespace utils {
/*
 
tcp://127.0.0.1:3333?key=value&

uri       = protocol "://" host [":" port] [ "?" param *( "&" param ) ]
protocol  = tcp/udp/ws/wss/kcp
param     = key "=" value
key       = *( "[^=]" )
value     = *( "[^&:]" )
*/

/*
options: 
    bind_addr
    bind_port 
    multi  //multi address 
 
*/
struct KNetUrl{
    std::string protocol;
    std::string host; 
    uint16_t    port; 
    //std::vector<std::pair<std::string, std::string > > params;
    KNetUrl(const std::string & url = ""){
        encode(url); 
    }
    
    KNetUrl(const std::string & proto ,const  std::string & host , uint16_t port =0 ){
        protocol = proto; 
        this->host = host; 
        this->port = port; 
    }

    std::unordered_map<std::string, std::string> params; 
 
    bool encode(const std::string &channel)
    { 
        auto pos = channel.find("://");
        if (pos != std::string::npos){
            protocol = channel.substr(0, pos);
        }else {
            return false; 
        }
        pos += 2; //skip "//" after ":"
        
        auto pPos = channel.find("?",pos); 
        std::string hostInfo = channel.substr(pos +1 , pPos); 
        auto cPos = hostInfo.find(":"); 
        if (cPos != std::string::npos){
            host = hostInfo.substr(0,cPos); 
            auto port = hostInfo.substr(cPos +1); 
            if (!port.empty())
            {
                port = std::stoi( port); 
            }
            
        }else {
            host = hostInfo; 
        }

        bool is_key = true;
        std::string key ;
        std::string value;
        for(uint32_t i = pPos +1;i <  channel.length(); i ++) {
            if ( channel[i] == '='){
                is_key = !is_key;
            }else if (channel[i] == '&'  ){
                params[key] = value;
                key.clear();
                value.clear();
                is_key = !is_key;
            }else {
                if(is_key){
                    key.append(1,channel[i]);
                } else {
                    value.append(1,channel[i]);
                }
            }
        }
        if (!key.empty()){
            params[key] = value;
        }
        return true;
    }

    std::string encode(){
        std::string url = protocol + "://"  + host + (port >0 ?":"+std::to_string(port):""); 
        if (!params.empty()){
            url  += "?"; 
            for(const auto & param : params){
                url += (param.first +"=" +param.second) +"&"; 
            }
        }
        return url; 
    }

    bool has(const std::string & key) const {
        return params.find(key) != params.end(); 
    }
 
    void set(const std::string & key  , const std::string & val){
        params[key] = val; 
    }

    std::string get(const std::string& key, const std::string & dft = "") const {
        auto itr = params.find(key); 
        if (itr!= params.end()){
            return itr->second; 
        }
        return dft; 
    }

    std::string  dump() const {
        std::string dumpStr = "protocol : " + protocol + "\n";       
                    dumpStr += "host : " + host + "\n";       
                    dumpStr += "port : " + std::to_string(port) + "\n";       
        for(auto & param : params){
            dumpStr +=  param.first + " : " + param.second +"\n";
        }
        return dumpStr; 
    }
};


} //namespace utils 
} //namespace knet 