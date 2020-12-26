//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include "utils/timer.hpp"
#include <mutex>
using namespace knet::utils;
namespace knet
{
	class EventWorker
	{
	public:
		using WorkStarter = std::function<void()>;

		EventWorker(asio::io_context* ctx = nullptr, void* udata = nullptr)
		{
			if (ctx == nullptr) {
				io_context = new asio::io_context();
				self_context = io_context; //hold self context
			}
			else {
				io_context = ctx;
			}
			this->user_data = udata;
			event_timer = std::make_shared<Timer>(*io_context);
		}


		virtual ~EventWorker()
		{
			if (self_context)
			{
				io_context = nullptr;
				delete self_context;
				self_context = nullptr;
			}
		}

		EventWorker(EventWorker&& rhs) = delete;
		EventWorker& operator=(EventWorker&& rhs) = delete;

		void start(WorkStarter starter = nullptr, uint32_t thrds = 1)
		{
			wlog("start event work {}", std::this_thread::get_id());
			work_starter = starter;
			if (self_context != nullptr)
			{

				for (uint32_t i = 0;i < thrds;i++) {

					//	wlog("real start event work here {}", std::this_thread::get_id());

					work_threads.emplace_back(std::thread(&EventWorker::run, this));
				}
			}
			else {
				//it's outer context, user should start it in other place 
			}

			if (work_starter)
			{
				asio::dispatch(*io_context, work_starter);
			}
		}
		void stop()
		{

			for (auto& thrd : work_threads) {
				if (thrd.joinable()) {
					if (self_context) {
						self_context->stop();
						thrd.join();
					}
				}

			}
		}

		virtual void init() {}
		virtual void deinit() {};

		template <class Func>
		void post(Func fn)
		{
			if (io_context)
			{
				asio::post(*io_context, fn);
			}
		}


		inline asio::io_context* get_context() { return io_context; }
		inline asio::io_context& context() { return *io_context; }
		inline std::thread::id thread_id() const { return work_threads[0].get_id(); }

		uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
		{
			return event_timer->start_timer(handler, interval, bLoop);
		}
		void stop_timer(uint64_t timerId) { event_timer->stop_timer(timerId); }

		inline void* get_user_data() { return user_data; }

	protected:

		void run()
		{
			dlog("start event worker {}", std::this_thread::get_id());
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

		asio::io_context* io_context = nullptr;
		asio::io_context* self_context = nullptr;
		TimerPtr event_timer;
	};
	using EventWorkerPtr = std::shared_ptr<EventWorker>;
} // namespace knet