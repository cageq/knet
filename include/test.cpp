//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#include "loop_buffer.hpp"
#include <thread>
#include <iostream> 
#include <unistd.h>


	std::string write_data = ""; 
	std::string read_data = ""; 

int main(int argc, char * argv[])
{
	std::string msg = "hello world"; 
	uint32_t MAX_LOOP  = 1000000; 
	LoopBuffer<char, 10240>  loopBuffer; 
	//
	//	loopBuffer.push_back(msg.c_str(),  msg.length()); 
	//	std::cout << "push length :" << msg.length() << std::endl; 
	//
	//	loopBuffer.pop_front(msg.length(), [](const std::vector<char> & data) { 
	//
	//			std::cout << "pop message :" << data.data() << std::endl; 
	//			std::cout << "pop length  :" << data.size() << std::endl; 
	//
	//			}); 

	std::thread producer([&]() { 
			dlog("star producer\n"); 
			int total = MAX_LOOP; 
			int count = 10; 
			while(true)
			{
				bool ret = loopBuffer.push_back(msg.c_str(), msg.length()); 
				if ( ret  && total -- > 0 ) {
				write_data.append(msg); 
					count = count > 0 ? count --:10; 
					if (count == 1) 
					{
						usleep(1); 
						dlog("push  %ld\n", (MAX_LOOP - total) * msg.length());
					}

					if (total <=0 ) 
					{
						break; 
					}
				}
			}

			}); 


	int64_t total_cousume  = 0; 
	std::thread consumer([&]() { 

			int count = 10; 
			
			while(1)
			{

			int ret = loopBuffer.pop_segment([&](const char * pData, uint32_t  len ){ 
					read_data.append(pData, len); 


					}); 

				if (ret > 0 ) {
					loopBuffer.drop_front(ret); 
				}

				//int popSize = rand() %(msg.length() *10) ; 
				//int32_t ret = loopBuffer.pop_front(msg.length() ); 

				total_cousume += ret; 
				count = count > 0 ? count --:10; 
				if (count == 1) {
					usleep(1); 
					dlog("pop %ld \n",total_cousume ); 
				}

				if (total_cousume == msg.size() * MAX_LOOP) {
					if (read_data == write_data) {
						dlog("read equal write  , buffer is empty%d", loopBuffer.empty()); 
					}
					else {

						elog("read and write not equal "); 
					}
					break; 
				}
				dlog("total consume %ld \n", total_cousume); 
			}

			}); 

	producer.join(); 
	consumer.join(); 

	dlog("pop all  %ld \n",total_cousume ); 
	return 0; 
}
