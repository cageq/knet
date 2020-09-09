#pragma once 
#include <vector>
#include <thread>
#include <iostream>

class ConsoleSink { 

    public: 

        int32_t write(const std::string & msg , bool sync = true ){

                if (sync) {
                    std::cout << msg ;  
                }else {
                    buffer_logs.emplace_back(msg);  
                }
            
            return 0; 
        }  
        
    private: 
         std::vector<std::string> buffer_logs; 

}; 