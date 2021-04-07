#pragma once

#include "utils/singleton.hpp"
#include <fmt/format.h>
#include <iostream>


#define KNET_LOG_SPDLOG  1 


#ifdef KNET_LOG_SPDLOG 
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

 



#define dout  std::cout 

class KNetLogger : public knet::utils::Singleton<KNetLogger>{
public: 
    KNetLogger(){
        logger = std::make_shared<spdlog::logger>("knet");
         logger->set_level(spdlog::level::trace);  
    }

    void add_console(){
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();      
        logger->sinks().emplace_back(console_sink);       
    }

    void add_file(const std::string & filePath, uint32_t hour, uint32_t minute ){
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filePath, hour, minute);
        logger->sinks().emplace_back(file_sink); 
    }

    void use_logger(std::shared_ptr<spdlog::logger> lg){
        logger = lg; 
    }

    spdlog::logger & get_logger(){
        return *logger; 
    }

    std::shared_ptr<spdlog::logger> logger; 
};


#define  KNetLogIns KNetLogger::instance()
 
#define ilog(format, args...) KNetLogIns.get_logger().info(format, ##args)
#define dlog(format, args...) KNetLogIns.get_logger().debug(format, ##args)
#define wlog(format, args...) KNetLogIns.get_logger().warn(format, ##args)
#define flog(format, args...) KNetLogIns.get_logger().critical(format, ##args)
#define elog(format, args...) KNetLogIns.get_logger().error(format, ##args)


#else //
#include "klog.hpp" 

class KNetLogger : public knet::utils::Singleton<KNetLogger>{
    public: 
     void add_console(){
        kLogIns.add_sink<klog::ConsoleSink<std::mutex, true> >(); 
     }

     void add_file(const std::string & filePath, uint32_t hour, uint32_t minute ){

             kLogIns.add_sink<FileSink<> >(filePath);
     }
}; 


#define  KNetLogIns KNetLogger::instance()
  
#endif //

