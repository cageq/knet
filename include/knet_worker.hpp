//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once

#include "utils/c11patch.hpp"
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
				io_context = std::make_shared<asio::io_context>(); 
				self_context = io_context; //hold self context
			}else {
				io_context = ctx;
			}
			this->user_data = udata;
			event_timer = std::make_unique<utils::Timer>(*io_context);
		}


		virtual ~KNetWorker()
		{
			if (self_context)
			{
				io_context = nullptr;
                for (auto& thrd : work_threads) {
                    if (thrd.joinable()) {
                        thrd.join();
                    }
                }
                self_context->stop();
                self_context = nullptr;
			}
		}

		KNetWorker(KNetWorker&& rhs) = delete;
		KNetWorker& operator=(KNetWorker&& rhs) = delete;

		void start(WorkStarter starter = nullptr, uint32_t thrds = 1)
		{
			//dlog("start event work {}", std::this_thread::get_id());
			work_starter = starter;
			if (self_context != nullptr)
			{
				for (uint32_t i = 0;i < thrds;i++) {
					//	wlog("real start event work here {}", std::this_thread::get_id());
					work_threads.emplace_back(std::thread(&KNetWorker::run, this));
				}
			}else {
				//it's outer context, user should start it in other place 
			}

			if (work_starter){
				asio::dispatch(*io_context, work_starter);
			}
		}
		void stop()
		{
            if (self_context) {
                self_context->stop();
                self_context = nullptr; 
            }
			for (auto& thrd : work_threads) {
                if (thrd.joinable()) {
                    thrd.join();
                }
			}
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
			if (!work_threads.empty()){
				return work_threads[0].get_id(); 
			}
			assert(false); 
			return std::thread::id(); 
		}

		uint64_t start_timer(const utils::Timer::TimerHandler& handler, uint64_t interval, bool bLoop = true)
		{
			return event_timer->start_timer(handler, interval, bLoop);
		}
		void stop_timer(uint64_t timerId) { event_timer->stop_timer(timerId); }

		inline void* get_user_data() { return user_data; }

	protected:

		void run()
		{			
			auto ioWorker = asio::make_work_guard(*io_context);
			std::call_once(init_flag, [&]() {
				this->init();
				});

			io_context->run();

			std::call_once(deinit_flag, [&]() {
				this->deinit();
				});

			dlog("exit event worker");
		}

		std::once_flag init_flag;
		std::once_flag deinit_flag;

		void* user_data = nullptr;
		WorkStarter work_starter = nullptr;
		std::vector<std::thread> work_threads;

		IOContextPtr io_context = nullptr;
		IOContextPtr self_context = nullptr;
        std::unique_ptr<utils::Timer> event_timer;
	};
	using KNetWorkerPtr = std::shared_ptr<KNetWorker>;
} // namespace knet
