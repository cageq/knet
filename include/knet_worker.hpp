//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include "utils/timer.hpp"
#include <mutex>
#include <thread>
using namespace knet::utils;
namespace knet
{
	class KNetWorker
	{
	public:
		using WorkStarter = std::function<void()>;
        using IOContextPtr = std::shared_ptr<asio::io_context>; 

		KNetWorker(IOContextPtr ctx = nullptr, void* udata = nullptr)
		{
			if (ctx == nullptr) {
				io_context = std::make_shared<asio::io_context>(1); 
				self_context = io_context; //hold self context
			}else {
				io_context = ctx;
			}
			this->user_data = udata;
			event_timer = std::make_unique<utils::Timer>(*io_context);
		}


		virtual ~KNetWorker()
		{
			 stop(); 
		}

		KNetWorker(KNetWorker&& rhs) = delete;
		KNetWorker& operator=(KNetWorker&& rhs) = delete;

		void start(WorkStarter starter = nullptr, uint32_t thrds = 1)
		{
			if (!is_running)
			{
				is_running = true; 
				// dlog("start event work {}", std::this_thread::get_id());
				work_starter = starter;
				if (self_context != nullptr)
				{
					for (uint32_t i = 0; i < thrds; i++)
					{
						//	wlog("real start event work here {}", std::this_thread::get_id());
						work_threads.emplace_back(std::thread(&KNetWorker::run, this));
					}
				}
				else
				{
					// it's outer context, user should start it in other place
				}

				if (work_starter)
				{
					asio::dispatch(*io_context, work_starter);
				}
			}
		}
		void stop()
		{
			if (self_context)
			{
				auto ctx = self_context; 
				self_context = nullptr;
				event_timer->clear(); 
				ctx->stop();
				for (auto& thrd : work_threads)
				{
					if (thrd.joinable())
					{
						thrd.join();
					}
				}
			 	work_threads.clear();		
			}	 			
			is_running = false; 
		}

		virtual void init() {}
		virtual void deinit() {};

		template <class Func>
		void post(const Func &  fn)
		{
			if (io_context)
			{
				asio::post(*io_context, fn);
			}
		}

		inline asio::io_context& context() { return *io_context; }
		inline std::thread::id thread_id() const { 
			return main_thread_id; 
		}

		uint64_t start_timer(const knet::utils::Timer::TimerHandler& handler, uint64_t interval, bool bLoop = true)
		{
			return event_timer->start_timer(handler, interval, bLoop);
		}
		void stop_timer(uint64_t timerId) { event_timer->stop_timer(timerId); }

		inline void* get_user_data() { return user_data; }


		void run()
		{			
			auto ioWorker = asio::make_work_guard(*io_context);
			std::call_once(init_flag, [&](){ this->init(); });
			main_thread_id = std::this_thread::get_id();
			io_context->run();

			std::call_once(deinit_flag, [&](){ this->deinit(); });

			dlog("exit event worker");
		}

	protected:
		std::once_flag init_flag;
		std::once_flag deinit_flag;

		void* user_data = nullptr;
		WorkStarter work_starter = nullptr;
		std::vector<std::thread> work_threads;

		IOContextPtr io_context = nullptr;
		std::thread::id main_thread_id;
		IOContextPtr self_context = nullptr;
		std::unique_ptr<utils::Timer> event_timer;
		bool is_running = false;
	};
	using KNetWorkerPtr = std::shared_ptr<KNetWorker>;
} // namespace knet
