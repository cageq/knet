#pragma once 
#include <string>
#include <memory>
class LogSink{ 

    public: 

    void init(){

    }
    
    int32_t write(int level , const std::string & ){
	
		return 0; 
	}
    void append(int level , const std::string & ){}

}; 

using LogSinkPtr =std::shared_ptr<LogSink>; 