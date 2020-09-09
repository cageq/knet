#pragma once
#include <vector>
#include <thread>
#include <iostream>
#include "log_sink.hpp"
namespace klog
{

    class ConsoleSink : public LogSink
    {

    public:
        ConsoleSink(bool sync = true)
        {
            m.is_sync = sync;
        }
        virtual ~ConsoleSink(){}

        int32_t write(const std::string &msg, bool sync = true)
        {

            if (sync)
            {
                std::cout << msg;
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
        std::vector<std::string> buffer_logs;
    };

} // namespace klog