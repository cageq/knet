//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once

#include "timer.hpp"
using namespace knet::utils;
namespace knet {

class EventWorker {
public:
	using WorkStarter = std::function<void()>;
	EventWorker( ) {
		wlog("start event work by default {}", std::this_thread::get_id()); 
		io_context = new asio::io_context();
		self_context = io_context;
		event_timer = std::make_shared<Timer>(*io_context);
	}
	EventWorker(asio::io_context* ctx  ) {
		wlog("start event work {} using outter context", std::this_thread::get_id()); 
		io_context = ctx;
		event_timer = std::make_shared<Timer>(*io_context);
	}

	EventWorker(EventWorker&& rhs) = default;
	EventWorker& operator=(EventWorker&& rhs) = default;

	void start(WorkStarter starter = nullptr) {
		wlog("start event work here {}", std::this_thread::get_id()); 
		work_starter = starter;
		if (self_context != nullptr) {
			if (!work_thread.joinable()) { 
			//	wlog("real start event work here {}", std::this_thread::get_id()); 
				//work_thread = std::thread(&EventWorker::run, this);
				work_thread = std::thread([this](){
					this->run(); 
				});
			} else {
				wlog("post starter to work thread"); 
				if (work_starter) {
					asio::post(*io_context, work_starter);
				}
			}
		} else {
			
			if (work_starter) {
				asio::post(*io_context, work_starter);
			}
		} 
	}
	void stop(){
		if (work_thread.joinable()) {
			io_context->stop();
			work_thread.join();
		}
	}

	virtual void init() {}
	virtual void deinit(){};
	template <class Func>
	void post(Func fn) {
		if (io_context) {
			asio::post(*io_context, fn);
		}
	}

	virtual ~EventWorker() {
		if (self_context) {
			io_context = nullptr;
			delete self_context;
			self_context = nullptr;
		}
	}

	asio::io_context * get_context(){
		return io_context;
	}

	asio::io_context& context() { return *io_context; }
	std::thread::id thread_id() const { return work_thread.get_id(); }

	uint64_t start_timer(TimerHandler handler, uint64_t interval, bool bLoop = true) {
		return event_timer->start_timer(handler, interval, bLoop);
	}
	void stop_timer(uint64_t timerId) { event_timer->stop_timer(timerId); }

	Timer& timer() { return *event_timer; }
	void* user_data = nullptr;

	void loop(){
		if (self_context == nullptr){
			run(); 
		}
	}

	WorkStarter work_starter = nullptr;
	virtual void run() {
		dlog("start event worker {}", std::this_thread::get_id());
		if (work_starter) {
			work_starter();
		}
		
		auto ioWorker = asio::make_work_guard(*io_context);		
		this->init();
		io_context->run();
		deinit();
		dlog("quitting context run");
	}

protected: 
	std::thread work_thread;
	asio::io_context* io_context = nullptr;
	asio::io_context* self_context = nullptr;
	TimerPtr event_timer;
};
using EventWorkerPtr = std::shared_ptr<EventWorker>;
} // namespace gnet
