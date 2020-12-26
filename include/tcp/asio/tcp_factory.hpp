//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once 
#include <functional>
#include "c11patch.hpp"

namespace knet {
	namespace tcp {

		template <class T, class ... Params >
		class TcpFactory {
		public:
			using TPtr = std::shared_ptr<T>;
			TcpFactory(Params ... params) :init_params(params ...) {
			}


			TPtr create() {
				return std::apply(&TcpFactory<T, Params...>::create_helper, init_params);
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



