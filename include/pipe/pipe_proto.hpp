#pragma once
#include "utils/c11patch.hpp"
#include <vector>


namespace knet{ 

	namespace pipe{ 
		enum PipeMsgType {
		 PIPE_MSG_SHAKE_HAND = 1, 
		 PIPE_MSG_HEART_BEAT, 
		 PIPE_MSG_DATA 
		};

		struct PipeMsgHead {
			uint32_t length : 24;
			uint32_t type : 8;

			PipeMsgHead(uint16_t t = 0, uint32_t len = 0)
				: length(len)
				  , type(t) {}
		};
 
 
	}

}
