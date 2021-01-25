//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once 
#include <functional>
#include "utils/c11patch.hpp"

namespace knet {
	namespace tcp {

		template <class T, class ... Params >
		class UdpFactory {
		public:
			using TPtr = std::shared_ptr<T>;
			UdpFactory(Params ... params) :init_params(params ...) {
			}

			template <class ... Args> 
			TPtr create(Args ... args ) {
				return std::make_shared<T> (std::forward<Args>(args)...);			
			} 

			TPtr create() {
				return std::apply(&UdpFactory<T, Params...>::create_helper, init_params);
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

}



