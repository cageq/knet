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

// #define ilog(...) spdlog::info(__VA_ARGS__)
// #define dlog(...) spdlog::debug(__VA_ARGS__)
// #define wlog(...) spdlog::warn(__VA_ARGS__)
// #define flog(...) spdlog::critical(__VA_ARGS__)
// #define elog(...) spdlog::error(__VA_ARGS__)




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

    void add_file_logger(const std::string & filePath, uint32_t hour, uint32_t minute ){
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


inline void add_console_logger(){
    KNetLogger::instance().add_console();    
}

inline void add_file_logger(const std::string &filePath, uint32_t hour, uint32_t minute ){
  KNetLogger::instance().add_file_logger(filePath, hour,minute);    
}

 
#define ilog(format, args...) KNetLogger::instance().get_logger().info(format, ##args)
#define dlog(format, args...) KNetLogger::instance().get_logger().debug(format, ##args)
#define wlog(format, args...) KNetLogger::instance().get_logger().warn(format, ##args)
#define flog(format, args...) KNetLogger::instance().get_logger().critical(format, ##args)
#define elog(format, args...) KNetLogger::instance().get_logger().error(format, ##args)


#else //
#include "klog.hpp" 
 
inline void add_console_logger(){
    kLogIns.add_sink<klog::ConsoleSink<std::mutex, true> >(); 
}

inline void add_file_logger(const std::string &filename){
    kLogIns.add_sink<FileSink<> >(filename);
}

#endif //
