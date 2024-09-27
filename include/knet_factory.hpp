//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once 
#include <functional>
#include <memory>

namespace knet {

		template <class T, class ... Params >
		class KNetFactory {
		public:
			using ConnectionPtr = std::shared_ptr<T>;
			KNetFactory(Params ... params) :init_params(params ...) {
			}

			template <class ... Args> 
			ConnectionPtr create(Args ... args ) {
				auto session =  std::make_shared<T> (std::forward<Args>(args)...);			
				this->on_create(session);
				return session;
			} 
		
			ConnectionPtr create() {
				auto session =  std::apply(&KNetFactory<T, Params...>::create_helper, init_params);
				this->on_create(session);
				return session;
			} 

			void release(ConnectionPtr sess) {
				on_release(sess);
			} 

			virtual void on_create(ConnectionPtr ptr) {

			}

			virtual void on_release(ConnectionPtr ptr) { 
				 
			}

		private:
			static ConnectionPtr create_helper(Params ... params)
			{
				return std::make_shared<T> (std::forward<Params>(params)...);				 
			}

			std::tuple<Params ...> init_params; 
		}; 
}



