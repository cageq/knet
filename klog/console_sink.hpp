#pragma once 
#include <vector>

class ConsoleSink { 

    public: 

        int32_t write(int level , const std::string & ){

            switch (level)
            {
            case /* constant-expression */:
                /* code */
                break;
            
            default:
                break;
            }

        }

        void append(int level , const std::string & ){}


    std::thread_local std::vector<std::string> buffer_logs; 


}; 