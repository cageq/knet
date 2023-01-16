#pragma once
#include <memory>
#include <string>
#include <functional>
#include "pipe_proto.hpp"


namespace knet
{

	namespace pipe
	{

		enum PipeEvent
		{
			PIPE_CONNECT,
			PIPE_DISCONNECT,
			PIPE_HEARTBEAT_REQUEST,
			PIPE_HEARTBEAT_RESPONSE
		};
 
		using PipeEventHandler = std::function<bool(NetEvent , void *  )> ; 
		using PipeDataHandler  = std::function<int32_t (const std::string& , uint64_t , void * )>; 

		class PipeSession : public std::enable_shared_from_this<PipeSession>
		{
		public:
			PipeSession(const std::string &pid = "", const std::string &h = "", uint16_t p = 0)
			{
				peer_host = h;
				peer_port = p;
				pipe_id = pid;
				ready_flag = false; 
			}

			virtual ~PipeSession() {}

			void use_handlers(const PipeEventHandler & evtHandler, const PipeDataHandler & dataHandler , void * ctx ){
				pipe_event_handler = evtHandler; 
				pipe_data_handler = dataHandler; 
				pipe_handler_context = ctx; 
			}

			virtual bool handle_event(NetEvent evt)
			{				
				if (pipe_event_handler){
					return pipe_event_handler(evt , pipe_handler_context); 
				}
				return true;
			}
			virtual int32_t handle_message(const std::string &msg, uint64_t obdata = 0)
			{				 
				if (pipe_data_handler ){
					return pipe_data_handler(msg, obdata, pipe_handler_context); 
				}
				return 0;
			} 

			int32_t send(const std::string &msg, uint64_t obdata = 0 )
			{
				if (is_ready())
				{
					PipeMsgHead head{ (uint32_t)msg.length(), PIPE_MSG_DATA, 0 };
					head.data = obdata;
					return connection->msend(head, msg);
				}
				return -1;
			}

			bool is_ready()
			{
				if (ready_flag  && connection)
				{
					return connection->is_connected();
				}else {
					wlog("pipe is not ready, {}",pipe_id); 
				}
				return false;
			}

			int32_t send(const std::string_view & msg )
			{
				if (is_ready())
				{
					PipeMsgHead head{ (uint32_t)msg.length(), PIPE_MSG_DATA, 0 };
					return connection->msend(head, msg);
				}
				return -1;
			}
 

			template <class P, class... Args>
			int32_t msend_with_obdata(uint64_t obdata, const P &first, const Args &...rest)
			{
				if (is_ready())
				{
					uint32_t bodyLen = pipe_data_length(first, rest...);					
					PipeMsgHead head{ bodyLen, PIPE_MSG_DATA, 0 };
					head.data = obdata;
					return connection->msend(head, first, rest...);
				}
				return -1;
			}

			template <class P, class... Args>
			inline int32_t msend(const P &first, const Args &...rest)
			{
				return msend_with_obdata(0,first ,  rest ...); 			
			}

			void bind(const PipeConnectionPtr &  conn)
			{
				this->connection = conn;
				conn->pipeid = pipe_id;
				conn->session = this->shared_from_this();
				ready_flag  = true;				 
			}

			void unbind()
			{ 
				ready_flag = false; 
			}

			inline void set_host(const std::string &h)
			{
				peer_host = h;
			}

			inline void set_port(uint16_t p)
			{
				peer_port = p;
			}

			inline void set_pipeid(const std::string &pid)
			{
				pipe_id = pid;
			}

			inline void update_pipeid(const std::string &pid)
			{
				if (pipe_id.empty())
				{
					pipe_id = pid;
				}
			}

			inline const std::string &get_pipeid() const
			{
				return pipe_id;
			}
			inline uint16_t get_port() const
			{
				return peer_port;
			}

			inline const std::string &get_host() const
			{
				return peer_host;
			}

			void on_ready(){
				if (connection && connection->is_connected() ){
					if (connection->is_passive())
					{
						heartbeat_timeid = connection->start_timer([this](){ 
							if (connection->is_connected()){
								connection->send_heartbeat(pipe_id); 
								return true; 
							} 
							return false; 
						}, 3000000); 	
					}
				}				
			}
		private:
	 
			uint64_t heartbeat_timeid; 
			uint16_t peer_port;
			std::string peer_host;
			std::string pipe_id;
			PipeEventHandler pipe_event_handler; 
			PipeDataHandler  pipe_data_handler; 
			void * pipe_handler_context = nullptr ; 
		 	std::atomic<bool> ready_flag; 
			PipeConnectionPtr connection;
		};

		using PipeSessionPtr = std::shared_ptr<PipeSession>;
	}

}
