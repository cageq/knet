#pragma once
#include <memory>
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

		class PipeSession : public std::enable_shared_from_this<PipeSession>
		{
		public:
			PipeSession(const std::string &pid = "", const std::string &h = "", uint16_t p = 0)
			{
				m.host = h;
				m.port = p;
				m.pipeid = pid;
				ready_flag = false; 
			}

			virtual ~PipeSession() {}
			virtual bool handle_event(NetEvent evt)
			{
				dlog("handle net event {}", evt);
				return true;
			}
			virtual int32_t handle_message(const std::string &msg, uint64_t obdata = 0)
			{
				dlog("handle pipe message : {}", msg);
				return 0;
			}

			int32_t transfer(const std::string &msg)
			{
				return transfer(msg.data(), msg.length());
			}

			int32_t transfer(const char *pData, uint32_t len)
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, len);
					dlog("transfer data to pipe {} , length {} type {}", m.pipeid, len, head.type);
					//dlog("send head length {}, type is {}", head.length, head.type);
					return connection->msend(
						std::string((const char *)&head, sizeof(PipeMsgHead)), std::string(pData, len));
				}
				else
				{
					wlog("pipe session has no connection {}", m.pipeid);
				}
				return -1;
			}

			int32_t send(uint64_t obdata, const std::string &msg)
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, msg.length());
					head.data = obdata;
					return connection->msend(std::string((const char *)&head, sizeof(PipeMsgHead)), msg);
				}
				return -1;
			}

			int32_t send(const std::string &msg)
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, msg.length());
					return connection->msend(std::string((const char *)&head, sizeof(PipeMsgHead)), msg);
				}
				return -1;
			}
			bool is_ready()
			{
				if (ready_flag.load(std::memory_order_acquire) && connection)
				{
					return connection->is_connected();
				}
				return false;
			}

			int32_t send(const char *pData, uint32_t len)
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, len);
					return connection->msend(
						std::string((const char *)&head, sizeof(PipeMsgHead)), std::string(pData, len));
				}
				return -1;
			}

			int32_t send_heartbeat(const std::string &msg)
			{
				PipeMsgHead head(PIPE_MSG_HEART_BEAT, msg.length());
				if (is_ready())
				{
					return connection->msend(std::string((const char *)&head, sizeof(PipeMsgHead)), msg);
				}
				return -1;
			}

			template <class P, class... Args>
			int32_t msend_with_obdata(uint64_t obdata, const P &first, const Args &...rest)
			{
				if (is_ready())
				{
					uint32_t bodyLen = pipe_data_length(first, rest...);
					dlog("msend body length is {}", bodyLen);
					PipeMsgHead head(PIPE_MSG_DATA, bodyLen);
					head.data = obdata;
					return connection->msend(std::string((const char *)&head, sizeof(PipeMsgHead)), first, rest...);
				}
				return -1;
			}

			template <class P, class... Args>
			int32_t msend(const P &first, const Args &...rest)
			{
				if (is_ready())
				{
					uint32_t bodyLen = pipe_data_length(first, rest...);
					dlog("msend body length is {}", bodyLen);
					PipeMsgHead head(PIPE_MSG_DATA, bodyLen);
					return connection->msend(std::string((const char *)&head, sizeof(PipeMsgHead)), first, rest...);
				}
				return -1;
			}

			void bind(PipeConnectionPtr conn)
			{
				this->connection = conn;
				conn->pipeid = m.pipeid;
				conn->session = this->shared_from_this();
				//ready_flag  = true;
				ready_flag.store(true, std::memory_order_acquire);
			}
			void unbind()
			{

				//ready_flag = false;
				ready_flag.store(false, std::memory_order_release);
				 
				connection.reset();
				// if (connection->session)
				// {
				// 	connection->session.reset();
				// }
			}

			inline void set_host(const std::string &h)
			{
				m.host = h;
			}

			inline void set_port(uint16_t p)
			{
				m.port = p;
			}

			inline void set_pipeid(const std::string &pid)
			{
				m.pipeid = pid;
			}

			inline void update_pipeid(const std::string &pid)
			{
				if (m.pipeid.empty())
				{
					m.pipeid = pid;
				}
			}

			inline const std::string &get_pipeid() const
			{
				return m.pipeid;
			}
			inline uint16_t get_port() const
			{
				return m.port;
			}

			inline const std::string &get_host() const
			{
				return m.host;
			}
		private:
			struct
			{
				std::string pipeid;
				std::string host;
				uint16_t port;
			} m;
		 	std::atomic<bool> ready_flag; 
			PipeConnectionPtr connection;
		};

		using PipeSessionPtr = std::shared_ptr<PipeSession>;
	}

}
