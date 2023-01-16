#pragma once
#include <functional>
#include <unordered_map>
#include <asio.hpp>
#include <chrono>
#include "utils/knet_log.hpp"
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
				bool alive = false;
				uint64_t id;
				bool loop = true;
			};
			using TimerNodePtr = std::shared_ptr<TimerNode>; 


			Timer(asio::io_context &ctx)
				: context(ctx) {}
				
			~Timer()
			{ 
				clear(); 
			}

			void clear(){
				for (auto &item : timers)
				{
					item.second->alive = false; 
					//add cancel 
					item.second->timer.cancel(); 
				}
				timers.clear();
			}

			void handle_timeout(TimerNodePtr node)
			{
				if (node->alive)
				{
					if (node->handler)
					{
						bool ret = node->handler();
						if (!ret){ 
							node->alive = false; 
							node->timer.cancel(); 
							timers.erase(node->id);
			 
							return; 
						}
					}
					if (node->loop)
					{
						node->timer.expires_after(asio::chrono::microseconds(node->interval));
						node->timer.async_wait(std::bind(&Timer::handle_timeout, this, node));
					}
					else
					{
					}
				}
				else
				{
					timers.erase(node->id);					 
				}
			}


			uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
			{
				static uint64_t timer_index = 1;

				auto node = std::make_shared<TimerNode>(context);
				node->handler = handler;
				node->interval = interval;
				node->alive = true;
				node->id = timer_index++;
				node->loop = bLoop;

				asio::dispatch(context, [this, node]() { timers[node->id] = node; });

				node->timer.expires_after(asio::chrono::microseconds(interval));
				node->timer.async_wait(std::bind(&Timer::handle_timeout, this, node)); 
				return node->id;
			}

			void stop_timer(uint64_t timerId)
			{ 
				asio::post(context, [this, timerId]() {
					auto itr = timers.find(timerId);
					if (itr != timers.end())
					{
						itr->second->alive = false;
						itr->second->timer.cancel();
					}
				});
			}
			void reset_interval(uint64_t timerId, uint64_t interval)
			{
			}

		private:
			std::unordered_map<uint64_t, TimerNodePtr> timers;
			asio::io_context &context;
		};

		using TimerPtr = std::shared_ptr<Timer>;

	} // namespace utils
} // namespace knet
