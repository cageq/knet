#pragma once
#include <functional>
#include <unordered_map>
#include <asio.hpp>
#include <chrono>
#include "klog.hpp"
namespace knet
{
	namespace utils
	{

		using TimerHandler = std::function<void()>;
		class Timer final
		{
		public:
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


			Timer(asio::io_context &ctx)
				: context(ctx) {}
				
			~Timer()
			{ 
				for (auto &item : timers)
				{
					stop_timer(item.first);
				}
				timers.clear();
			}

			void handle_timeout(TimerNode *node)
			{

				if (node->alive)
				{

					if (node->handler)
					{
						node->handler();
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
					delete node;
				}
			}


			uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
			{
				static uint64_t timer_index = 1;

				TimerNode *node = new TimerNode(context);
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
				asio::dispatch(context, [this, timerId]() {
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
			std::unordered_map<uint64_t, TimerNode *> timers;
			asio::io_context &context;
		};

		using TimerPtr = std::shared_ptr<Timer>;

	} // namespace utils
} // namespace knet
