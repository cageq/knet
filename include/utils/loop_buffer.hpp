

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
  int32_t read(DataHandler handler = nullptr) {
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

  uint32_t commit(uint32_t len) { head += len;return len;  }

  bool empty() const { return head == tail; } 

  bool peek() { return head != tail; }

  uint32_t size() const{
	return tail - head; 
  }

  void clear(){ head = tail = 0; }

private:
  // https://stackoverflow.com/questions/8819095/concurrency-atomic-and-volatile-in-c11-memory-model
  std::atomic<uint32_t> head{0};
  std::atomic<uint32_t> tail{0};
  std::array<char, BufferSize> buffer;
  Mutex write_mutex;
};

} // namespace utils
} // namespace knet
