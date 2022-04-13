
#pragma once 
#include <string> 
#include <vector>
#include <unordered_map>
#include <sstream>
#include <type_traits> 
#include <algorithm> 


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
    rbuf_size
    sbuf_size 
    encryption
    auth_token
    async
    backlogs 
    delay
    threads 
 
*/

    template<class T> 
    struct TypeCaster{
        T operator()(const std::string & val) {
            T rst; 
            std::stringstream ss ( val ); 
            ss  >> rst; 
            char c; 
            //https://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c 
            if (ss.fail() || ss.get(c)) {
                return T(); 
            }
            return rst; 
        }
    };

    template<> 
    struct TypeCaster<const char *> {
        inline const char *  operator()(const std::string &val ) {
            return val.c_str(); 
        }
    };


    template<> 
    struct TypeCaster<bool> {
        bool operator()(const std::string &val ) {
            std::string str = val ;
            std::transform(str.begin(), str.end(),str.begin(), 
                    [](unsigned char c){ return std::tolower(c);}
                    );
            if (val == "false" || val == "0" || val == "no"){
                return false; 
            }
            return true; 
        }
    };

    template<> 
    struct TypeCaster<float> {
        inline float operator()(const std::string &val ) {
            return std::stof(val); 
        }
    };

    template<> 
    struct TypeCaster<double> {
        inline double operator()(const std::string &val ) {
            return std::stod(val); 
        }
    };

    template<> 
    struct TypeCaster<long double> {
        inline long double operator()(const std::string &val ) {
            return std::stold(val); 
        }
    };

    template<> 
    struct TypeCaster<std::string> {
        inline std::string operator()(const std::string &val ) {
            return val; 
        }
    };


struct KNetUrl{
    std::string protocol;
    std::string host; 
    uint16_t    port  = 0 ; 
    KNetUrl(const std::string & url = ""){
        parse(url); 
    }
    
    KNetUrl(const std::string & proto ,const  std::string & host , uint16_t port =0 ){
        protocol = proto; 
        this->host = host; 
        this->port = port; 
    }


    bool parse(const std::string &channel)
    { 
        reset(); 
        auto pos = channel.find("://");
        if (pos != std::string::npos){
            protocol = channel.substr(0, pos);
        }else {
            return false; 
        }
        pos += 2; //skip "//" after ":"
        
        auto pPos = channel.find('?',pos); 
        if (pPos == std::string::npos)
        {
            pPos = channel.size(); 
        }
        std::string hostInfo = std::string (channel.begin() + pos +1 , channel.begin() + pPos); 
        auto cPos = hostInfo.find(":"); 
        if (cPos != std::string::npos){
            host = hostInfo.substr(0,cPos); 
            auto p = hostInfo.substr(cPos +1); 
            if (!p.empty())
            {
                port = (uint16_t)std::stoi( p); 
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

    inline bool has(const std::string & key) const {
        return params.find(key) != params.end(); 
    }
 
    void set(const std::string & key, const std::string & val){
        params[key] = val; 
    }

    std::string get(const std::string& key, const std::string & dft = "") const {
        auto itr = params.find(key); 
        if (itr!= params.end()){
            return itr->second; 
        }
        return dft; 
    }
    template <class T> 
        T get(const std::string & key, T dft = T()) const  {
            auto itr = params.find(key); 
            if (itr!= params.end()){
                return TypeCaster<T>{}(itr->second); 
            }
            return dft; 
        }

    std::string get_host() const {
        return host; 
    }

    uint16_t get_port() const {
        return port; 
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
    void reset(){
        params.clear(); 
        protocol.clear(); 
        host.clear(); 
        port = 0; 

    }

    private: 
    std::unordered_map<std::string, std::string> params; 
};


} //namespace utils 
} //namespace knet 
