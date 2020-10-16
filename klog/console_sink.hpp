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
			virtual ~ConsoleSink(){}

			int32_t write(const std::string &msg)
			{
				buffer.append(msg); 
				return 0;
			}
			virtual void flush(const std::string & log = "")
			{
				buffer.append(log); 
				std::cout << buffer << std::endl; 
				buffer.clear(); 
			}


		private:
			std::string buffer; 
	};

} // namespace klog
