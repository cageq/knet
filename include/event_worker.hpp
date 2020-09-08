//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include "timer.hpp"
using namespace knet::utils;
namespace knet
{

	class EventWorker
	{
	public:
		using WorkStarter = std::function<void()>;
 
		EventWorker(asio::io_context *ctx = nullptr ,void * udata = nullptr)
		{
			if (ctx == nullptr){
				io_context = new asio::io_context();
				self_context = io_context; //hold self context
			}else {
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

		EventWorker(EventWorker &&rhs) = delete;
		EventWorker &operator=(EventWorker &&rhs) = delete;

		void start(WorkStarter starter = nullptr)
		{
			wlog("start event work here {}", std::this_thread::get_id());
			work_starter = starter;
			if (self_context != nullptr)
			{
				if (!work_thread.joinable())
				{
					//	wlog("real start event work here {}", std::this_thread::get_id());
					work_thread = std::thread(&EventWorker::run, this);
				} 
			}else {
				//it's outer context, user should start it in other place 
			}

			wlog("post starter to work thread");
			if (work_starter)
			{
				asio::dispatch(*io_context, work_starter);
			}
		}
		void stop()
		{
			if (work_thread.joinable())
			{
				if (self_context)
				{
					self_context->stop();
					work_thread.join();
				} 
				
			}
		}

		virtual void init() {}
		virtual void deinit(){};

		template <class Func>
		void post(Func fn)
		{
			if (io_context)
			{
				asio::post(*io_context, fn);
			}
		}


		inline asio::io_context *get_context() { return io_context; } 
		inline asio::io_context &context() { return *io_context; }
		inline std::thread::id thread_id() const { return work_thread.get_id(); }

		uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true)
		{
			return event_timer->start_timer(handler, interval, bLoop);
		}
		void stop_timer(uint64_t timerId) { event_timer->stop_timer(timerId); } 
		
		void * get_user_data(){ return user_data; }

	protected: 

		void run()
		{
			dlog("start event worker {}", std::this_thread::get_id()); 
			auto ioWorker = asio::make_work_guard(*io_context);
			this->init();
			io_context->run();
			deinit();
			dlog("quitting context run");
		}
		void *user_data = nullptr; 
		WorkStarter work_starter = nullptr;
		std::thread work_thread;
		asio::io_context *io_context = nullptr;
		asio::io_context *self_context = nullptr;
		TimerPtr event_timer;
	};
	using EventWorkerPtr = std::shared_ptr<EventWorker>;
} // namespace knet
