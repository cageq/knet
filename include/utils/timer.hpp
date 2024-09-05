#pragma once
#include <functional>
#include <unordered_map>
#include <asio.hpp>
#include <chrono>
#include "utils/knet_log.hpp"
#include <atomic> 
#include <mutex> 
namespace knet
{
	namespace utils
	{

		
		class Timer 
		{
		public:
			using TimerHandler = std::function<bool()>;
			struct TimerNode
			{
				TimerNode(asio::io_context &ctx)
					: timer(ctx) {}
				asio::steady_timer timer;
				TimerHandler handler;
				uint64_t interval;
				uint64_t id;
				bool alive = false;		
				bool skip = false; 		
				bool loop = true;
			};
			using TimerNodePtr = std::shared_ptr<TimerNode>; 

            using TimerMap = std::unordered_map<uint64_t, TimerNodePtr>; 
			Timer(asio::io_context &ctx)
				: context(ctx) {}
				
			~Timer()
			{ 
				clear(); 
			}

			void clear(){
                std::lock_guard<std::mutex> guard(timer_mutex); 
				for (auto &item : timer_nodes)
				{
					item.second->alive = false; 
					//add cancel 
					item.second->timer.cancel(); 
				}
				timer_nodes.clear();
			}

			void handle_timeout(TimerNodePtr node)
			{
				if (node->alive)
				{
					if (node->handler)
					{
						if (node->skip){
							node->skip = false; 
						}else {							
							bool ret = node->handler();
							if (!ret){ 
								node->alive = false; 
								node->timer.cancel(); 
                                {
                                    std::lock_guard<std::mutex> guard(timer_mutex); 
                                    timer_nodes.erase(node->id);				
                                }
								return; 
							}
						}
					
					}
					if (node->loop)
					{
						node->timer.expires_after(asio::chrono::microseconds(node->interval));
						node->timer.async_wait([this,node](const asio::error_code & ec){
                                    if (!ec){
                                        handle_timeout(node); 
                                    }
                                });
					}
					else
					{
					}
				}
				else
				{
                    std::lock_guard<std::mutex> guard(timer_mutex); 
					timer_nodes.erase(node->id);					 
				}
			}


			uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
			{
				static  std::atomic_uint64_t  timer_index{1};

				auto node = std::make_shared<TimerNode>(context);
				node->handler = handler;
				node->interval = interval;
				node->alive = true;
				node->id = timer_index++;
				node->loop = bLoop;

				//asio::dispatch(context, [this, node]() { timer_nodes[node->id] = node; });
                {
                    std::lock_guard<std::mutex> guard(timer_mutex); 
                    timer_nodes[node->id] = node; 
                }

				node->timer.expires_after(asio::chrono::microseconds(interval));
				node->timer.async_wait([this, node ](const asio::error_code & ec ){
                    if (!ec){
                        this->handle_timeout(node); 
                    }
				}); 
				return node->id;
			}

            bool restart_timer(uint32_t timerId ,uint64_t interval = 0  ){

                TimerNodePtr timerNode = nullptr; 

                {
                    std::lock_guard<std::mutex> guard(timer_mutex); 
                    auto itr = timer_nodes.find(timerId); 
                    if (itr != timer_nodes.end()){
						timerNode = itr->second; 
					}
                }

                if (timerNode)
                {						
                    timerNode->skip = true; 			
                    if (interval > 0){
                        timerNode->interval = interval; 
                    }	 
                }
                return true; 	 
            }


			void stop_timer(uint64_t timerId)
            { 
                TimerNodePtr timerNode = nullptr; 
                {
                    std::lock_guard<std::mutex> guard(timer_mutex); 
                    auto itr = timer_nodes.find(timerId); 
                    if (itr != timer_nodes.end()){
						timerNode = itr->second; 
					}
                }
                if (timerNode)
                {
                    timerNode->alive = false; //not safe? 
                    timerNode->timer.cancel();
                }
            }

			void reset_interval(uint64_t timerId, uint64_t interval)
            {
                TimerNodePtr timerNode = nullptr; 
                {
                    std::lock_guard<std::mutex> guard(timer_mutex);                     
					auto itr = timer_nodes.find(timerId); 
                    if (itr != timer_nodes.end()){
						timerNode = itr->second; 
					}
                }
                if (timerNode)
                {						
                    timerNode->timer.cancel();
                    timerNode->interval = interval; 
                    timerNode->timer.expires_after(asio::chrono::microseconds(timerNode->interval));
                    timerNode->timer.async_wait([this, timerNode ](const asio::error_code &ec){
                                if (!ec){
                                    this->handle_timeout(timerNode); 
                                }
                            }); 
                }
            }

		private:
            std::mutex timer_mutex; 
			TimerMap  timer_nodes;
			asio::io_context &context;
		};

		using TimerPtr = std::shared_ptr<Timer>;

	} // namespace utils
} // namespace knet
