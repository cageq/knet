#pragma once
#include <vector>
#include <functional>
#include <mutex> 
#include <condition_variable>

template <class T> 
class SyncQueue
{ 
  public:
    typedef  std::function<bool(T )> Processor; 
    void push(const T & item)
    {
      {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_data.push_back(std::move(item)); 
      }
      m_cond.notify_one();
    }


    // single thread 
    void process(Processor processor)
    {
      do{
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_data.empty();});
        if (m_cache.empty())
        {
          m_data.swap(m_cache);
        }
      }while(0); 
      bool status = false; 
      for (; m_index < m_cache.size(); m_index++)
      {
        status = processor(m_cache[m_index]); 
        if (!status)
        {
          break; 
        }
      }
      if (status)
      {
        m_index = 0; 
        m_cache.clear(); 
      }
    }

    void notify() {
      m_cond.notify_one();
    }

  private: 
    uint32_t m_index = 0 ; 
    std::mutex  m_mutex; 
    std::condition_variable m_cond;
    std::vector<T> m_data;  
    std::vector<T> m_cache;  
}; 
