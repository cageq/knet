#pragma once
#include <string>
#include <memory>

namespace klog
{

    class LogSink
    {

    public:
        virtual int32_t write(const std::string &)
        { 
            return 0;
        }
    };

    using LogSinkPtr = std::shared_ptr<LogSink>;

} // namespace klog