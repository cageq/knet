#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "log_sink.hpp"

namespace klog
{

    class FileSink : public LogSink
    {

    public:
        FileSink(const std::string &filePath, bool sync = true) : logfile(filePath, std::ofstream::app)
        {
            m.is_sync = sync;
        }
        virtual ~FileSink(){
            if (logfile.is_open())
            {
                logfile.close(); 
            }
            
        }

        int32_t write(const std::string &msg)
        {
           
            if (m.is_sync)
            {
                if (logfile.is_open())
                {
                    logfile << msg;
                    logfile.flush(); 
                }
            }
            else
            {
                buffer_logs.emplace_back(msg);
            }

            return 0;
        }

    private:
        struct
        {
            bool is_sync = true;

        } m;
        std::ofstream logfile;
        std::vector<std::string> buffer_logs;
    };

} // namespace klog