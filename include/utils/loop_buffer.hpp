

//***************************************************************
//	created:	2023/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <inttypes.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace knet {
namespace utils {

//https://blog.csdn.net/erazy0/article/details/6210569

#if defined(__linux__) || defined(__APPLE__)

#define __MEM_BARRIER__ \
    __asm__ __volatile__("mfence":::"memory")
 
#define __READ_BARRIER__ \
    __asm__ __volatile__("lfence":::"memory")
 
#define __WRITE_BARRIER__ \
    __asm__ __volatile__("sfence":::"memory")
 
#endif // __linux__
 


#ifdef _WIN64 
#include <windows.h>
#include <immintrin.h>
#include <intrin.h>
#include <chrono> 
#define __MEM_BARRIER__ \
    _mm_mfence()

#define __READ_BARRIER__ \
    _ReadBarrier()

#define __WRITE_BARRIER__ \
    _WriteBarrier() 

inline bool atomic_compare_and_swap(uint32_t *ptr, uint32_t expected,
                                    uint32_t new_value) {
  return (InterlockedCompareExchange((LONG volatile *)ptr, new_value,
                                     expected) == expected);
}
 
#endif // _WIN64
 

struct NonMutex{
    inline void lock(){}
    inline void unlock(){}
}; 

template <std::size_t kMaxSize = 4096, class Mutex = NonMutex>
class LoopBuffer {
public:
  using DataHandler = std::function<uint32_t(const char *, uint32_t)>;
  // https://www.techiedelight.com/round-next-highest-power-2/
  static constexpr inline uint32_t get_power_of_two(uint32_t n) {
    // decrement `n` (to handle the case when `n` itself is a power of 2)
    n--;
    // set all bits after the last set bit
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    // increment `n` and return
    return ++n;
  }

  static constexpr uint32_t BufferSize =
      kMaxSize & (kMaxSize - 1) ? get_power_of_two(kMaxSize) : kMaxSize;
 
      template <typename F, typename ... Args>
        uint32_t calc_data_length(const F &  data, Args... rest) { 					
            auto len = this->get_data_length(data) ;   
            return len + calc_data_length(rest...);
        }

        inline uint32_t calc_data_length() {  
            return 0; 
        }

        template <typename P >  
            inline uint32_t get_data_length( const P & data  ){ 
                return  sizeof(P);                              
            }  

        inline uint32_t get_data_length( const std::string_view &  data ){ 
            return  data.size();                              
        }     

        inline uint32_t get_data_length( const std::string &  data ){ 
            return  data.size();                              
        }                   

        inline uint32_t get_data_length( const char* data ){ 
            return strlen(data) ;                        
        }   

            
		template < typename ... Args>
			int32_t mpush(  Args &&... rest) { 	
        std::lock_guard<Mutex> lock(write_mutex);
        return _mpush(std::forward<Args>(rest)...); 
      }
		template <typename F, typename ... Args>
			int32_t _mpush(const F &  data, Args &&... rest) { 				
    
				auto ret = this->push(data) ;   
				return ret > 0 ?(ret+mpush(std::forward<Args>(rest)...)):0;
			}

		inline int32_t _mpush() {  
			return 0; 
		}

		template <class T> 
			uint32_t push(const T & data ) {
				return push((const char *)&data, sizeof(T)); 
			}
		template <class T> 
			uint32_t push(const std::vector<T>  & data ) {
				return push(data.data(), data.length()* sizeof(T)); 
			}

		uint32_t push(const std::string_view  & data ) {
		        return push(data.data(), data.length()); 
		}
		uint32_t push(const std::string  & data ) {
			return push(data.data(), data.length()); 
		}

		uint32_t push(const char * data ) {
			return push(data, strlen(data)); 
		}


  uint32_t push(const char *data, uint32_t dataLen) {
    std::lock_guard<Mutex> lock(write_mutex);
    uint32_t bufLen = BufferSize - (tail - head);
    if (bufLen < dataLen) {
      // printf("left buffer length %d\n", bufLen);
      return 0;
    }

    __MEM_BARRIER__;
    uint32_t writePos = tail & (BufferSize - 1);
    uint32_t tailSize = BufferSize - writePos;
    if (dataLen > tailSize) {
      memcpy(&buffer[writePos], data, tailSize);
      memcpy(&buffer[0], &data[tailSize], dataLen - tailSize);
    } else {
      memcpy(&buffer[writePos], data, dataLen);
    }
    __WRITE_BARRIER__;
    tail += dataLen;
    return dataLen;
  }

  int32_t pop(DataHandler handler = nullptr) {
    uint32_t dataLen = tail - head;
    if (dataLen == 0) {
      return 0;
    }
    uint32_t readPos = (head & (BufferSize - 1));
    __READ_BARRIER__;
    uint32_t len = std::min(dataLen, BufferSize - readPos);
    handler(&buffer[readPos], len);
    head += len;
    return len;
  }

  // should commit after read
  int32_t read(DataHandler handler ) {
    uint32_t dataLen = tail - head;
    if (dataLen == 0) {
      return 0;
    }
    uint32_t readPos = (head & (BufferSize - 1));
    __READ_BARRIER__;
    uint32_t len = std::min(dataLen, BufferSize - readPos);
    handler(&buffer[readPos], len);
    return len;
  }
  std::pair<char * , uint32_t>  read() {
 
      uint32_t dataLen = tail - head;
      if (dataLen == 0) {
        return std::make_pair(nullptr, 0);
      }
      __READ_BARRIER__;
      uint32_t readPos = (head & (BufferSize - 1));

      uint32_t len = std::min(dataLen, BufferSize - readPos); 
      return std::make_pair(&buffer[readPos], len);    
  }

  uint32_t commit(uint32_t len) { 
    head += len;
    notify();  
    return len; 
   }

  bool empty()   { 
    std::lock_guard<Mutex> lock(write_mutex);
    return head == tail; 
  } 


  uint32_t peek() { 
      if constexpr (std::is_same<NonMutex, Mutex>::value){
				return BufferSize -(tail - head) ;
			}else {
				std::lock_guard<Mutex> lk(write_mutex);
				return BufferSize -(tail - head) ;
			}  
  }

  uint32_t size() const{
	return tail - head; 
  }

  void  wait(uint32_t len,uint32_t msTimeOut = 0 )  {   
			if constexpr  (std::is_same<NonMutex, Mutex>::value){ 
				std::this_thread::sleep_for(std::chrono::microseconds(msTimeOut==0?1:msTimeOut));   
			}else {
				std::unique_lock<Mutex> lk(write_mutex);
				if (msTimeOut > 0){
					write_cond.wait_for(lk, std::chrono::microseconds(msTimeOut), [this, len  ]{return BufferSize -(tail - head)  >= len   ;});
				}else {
					write_cond.wait(lk, [this, len  ]{ return BufferSize -(tail - head)  >= len   ;});
				}
			}        
		}
		void notify(){
			if constexpr  (std::is_same<NonMutex, Mutex>::value){
				//
			}else {
				write_cond.notify_one();
			}        
		}



  void clear(){ head = tail = 0; }

private:
  // https://stackoverflow.com/questions/8819095/concurrency-atomic-and-volatile-in-c11-memory-model
  std::atomic<uint32_t> head{0};
  std::atomic<uint32_t> tail{0};
  std::array<char, BufferSize> buffer;
  Mutex write_mutex;
  	std::condition_variable write_cond;
};

} // namespace utils
} // namespace knet
