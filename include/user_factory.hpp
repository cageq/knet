//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once 
#include <functional>
#include "utils/c11patch.hpp"

namespace knet {

		template <class T, class ... Params >
		class UserFactory {
		public:
			using TPtr = std::shared_ptr<T>;
			UserFactory(Params ... params) :init_params(params ...) {
			}

			template <class ... Args> 
			TPtr create(Args ... args ) {
				return std::make_shared<T> (std::forward<Args>(args)...);			
			} 

			TPtr create() {
				return std::apply(&UserFactory<T, Params...>::create_helper, init_params);
			} 
			void release(TPtr sess) {} 

			static TPtr create_helper(Params ... params)
			{
				return std::make_shared<T> (std::forward<Params>(params)...);				 
			}
		private:
			std::tuple<Params ...> init_params; 
		}; 


}



