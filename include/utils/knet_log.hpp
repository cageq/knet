#pragma once

#include <stdio.h> 
#include <functional>
#include <inttypes.h>
#include <string> 
#include <fmt/format.h>
#include <fmt/chrono.h>



namespace knet {
    namespace log {
 
        enum KNetLogLevel :uint32_t {
            LOG_LEVEL_OFF, 
            LOG_LEVEL_TRACE,
            LOG_LEVEL_DEBUG,
            LOG_LEVEL_INFO,
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
            LOG_LEVEL_FATAL,
        }; 

		static const char * log_level_str [] = {

			"off", 
			"trace",
			"debug", 
			"info",
			"warn",
			"error", 
			"fatal", 
		}; 
        //level : 0 off, 1 trace, 2 debug, 3 info , 4 warn, 5 error, 6 fatal , 0x10 extend
		using KNetLogWriter = std::function<void(uint32_t level, const std::string &  msg )> ; 

        inline uint32_t from_string_level(const std::string & lv ){
            std::string str = lv; 
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            
            if (str == "off" || lv.empty()){
                return KNetLogLevel::LOG_LEVEL_OFF; 
            }
            if (str == "trace"){
                return KNetLogLevel::LOG_LEVEL_TRACE; 
            }
            if (str == "debug"){
                return KNetLogLevel::LOG_LEVEL_DEBUG; 
            }

            if (str == "info"){
                return KNetLogLevel::LOG_LEVEL_INFO; 
            }

            if (str == "warn"){
                return KNetLogLevel::LOG_LEVEL_WARN; 
            }

            if (str == "error"){
                return KNetLogLevel::LOG_LEVEL_ERROR; 
            }

            if (str == "fatal"){
                return KNetLogLevel::LOG_LEVEL_FATAL; 
            }
            return KNetLogLevel::LOG_LEVEL_OFF; 
        }

		//need c++17 
        inline KNetLogLevel knet_log_level = KNetLogLevel::LOG_LEVEL_OFF; 
        inline KNetLogWriter  knet_logger; 

        inline void register_logger(KNetLogWriter logger ){
            knet_logger = logger; 
        }

        inline void set_log_level(uint32_t level){
            knet_log_level = static_cast<KNetLogLevel>(level);             
        }

        template <class ... Args> 
        void knet_tlog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_TRACE){
                knet_logger(KNetLogLevel::LOG_LEVEL_TRACE, fmt::format(format, std::forward<Args >(args)... )); 
            }    
        }


        template <class ... Args> 
        void knet_dlog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_DEBUG){         
                knet_logger(KNetLogLevel::LOG_LEVEL_DEBUG, fmt::format(format, std::forward<Args >(args)... )); 
            }    
        }

        template <class ... Args> 
        void knet_ilog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_INFO){
                knet_logger(KNetLogLevel::LOG_LEVEL_INFO, fmt::format(format, std::forward<Args >(args)...  )); 
            }    
        }


        template <class ... Args> 
        void knet_wlog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_WARN){
                knet_logger(KNetLogLevel::LOG_LEVEL_WARN, fmt::format(format,std::forward<Args >(args)...  )); 
            }    
        }

        template <class ... Args> 
        void knet_elog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_ERROR){
                knet_logger(KNetLogLevel::LOG_LEVEL_ERROR, fmt::format(format, std::forward<Args >(args)... )); 
            }    
        }


        template <class ... Args> 
        void knet_clog(fmt::format_string<Args ...>  format, Args && ... args ){
            if (knet_logger && knet_log_level <= KNetLogLevel::LOG_LEVEL_FATAL){
                knet_logger(KNetLogLevel::LOG_LEVEL_FATAL, fmt::format(format, std::forward<Args >(args)... )); 
            }    
        }

    } //namespace log 
}  //namespace knet 


inline void knet_init_logger(uint32_t level, knet::log::KNetLogWriter logWriter = nullptr)
{
	if (logWriter != nullptr){    
		knet::log::register_logger(logWriter); 
	}
	knet::log::set_log_level(0x1F & level); 
}

inline void knet_add_console_sink(uint32_t level = 1){
	auto consoleSink  = [](uint32_t level, const std::string & log ){
		auto lv = static_cast<knet::log::KNetLogLevel>(level); 
		printf("[knet][%s] %s\n",knet::log::log_level_str[lv],  log.c_str()); 
	}; 
	knet_init_logger(level, consoleSink); 
}
