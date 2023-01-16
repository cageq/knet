#pragma once
#include <vector>

namespace knet
{

	namespace pipe
	{
		enum PipeMsgType
		{
			PIPE_MSG_SHAKE_HAND = 1,
			PIPE_MSG_HEART_BEAT,
			PIPE_MSG_DATA
		};

		struct PipeMsgHead
		{
			uint32_t length;
			uint32_t type;
			uint64_t data; //user data 
			char body[0]; 			
		};

		template <class T> 
		inline uint32_t data_length( const T & d){
			return sizeof(d); 
		}

 
		inline  uint32_t data_length( const std::string & d){
			return d.length(); 
		}

		inline uint32_t data_length( const std::string_view & d){
			return d.length(); 
		}

		inline uint32_t data_length( const char * d){
			return strlen(d) -1 ; 
		}

		inline uint32_t pipe_data_length()
		{
			return 0;
		}


		template <typename Arg, typename ... Args>
		uint32_t pipe_data_length(Arg arg, Args... args)
		{
			return data_length(arg) + pipe_data_length (args ... );
		} 

	}

}
