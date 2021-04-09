#pragma once
#include <functional>
#include "kpipe.hpp"
#include "pipe_connection.hpp"

namespace knet
{
	namespace pipe
	{

		enum PipeMode
		{
			PIPE_SERVER_MODE = 1,
			PIPE_CLIENT_MODE = 2,
			PIPE_DUET_MODE = 3
		};
		class PipeFactory : public KNetFactory<PipeConnection>, public KNetHandler<PipeConnection>
		{
		public:
			PipeFactory(PipeMode mode = PipeMode::PIPE_SERVER_MODE)
				: pipe_mode(mode) {}

			virtual bool handle_event(TPtr conn, NetEvent evt)
			{
				auto session = conn->get_session();
				//ilog("pipe factory event {} {} {}", evt, event_string(evt), std::this_thread::get_id());
				switch (evt)
				{
				case EVT_CONNECT:
				{
					if (!conn->is_passive())
					{
						send_shakehand(conn, conn->get_pipeid());
					}
				}
				break;
				case EVT_DISCONNECT:
				{
					if (session)
					{
						session->handle_event(NetEvent::EVT_DISCONNECT);
						session->unbind();
					}
					remove_unbind_pipe(conn->get_cid());
				}
				break;
				default:;
				}

				if (session)
				{
					dlog("handle session event {}", evt);
					session->handle_event(evt);
				}
				return false;
			}

			virtual bool handle_data(TPtr conn, const std::string &buf)
			{
				PipeMsgHead *msg = (PipeMsgHead *)buf.data();
				if (msg->length + sizeof(PipeMsgHead) > buf.length())
				{
					elog("data not enough, need length {}", msg->length + sizeof(PipeMsgHead));
					return 0;
				}
				dlog("pipe message type {}", msg->type);
				if (msg->type == PIPE_MSG_SHAKE_HAND)
				{
					dlog("handle pipe shake hande message ");
					if (conn->is_passive())
					{ //server side
						process_server_handshake(conn, msg);
					}
					else
					{
						process_client_handshake(conn, msg);
					}
					return true;
				}

				auto session = conn->get_session();
				if (session)
				{
					session->handle_message(std::string(buf.data() + sizeof(PipeMsgHead), msg->length), msg->data);
				}
				else
				{

					if (conn->is_passive())
					{
						wlog("connection has no session, send shakehand challenge");
						PipeMsgHead shakeMsg(PIPE_MSG_SHAKE_HAND); //challenge client to send shakehand again
						conn->msend(std::string((const char *)&shakeMsg, sizeof(PipeMsgHead)));
					}
				}
				return true;
			}

			std::string generate_id()
			{
				static uint64_t pid_index = 0;
				return fmt::format("pid{}", pid_index++);
			}

			void process_server_handshake(TPtr conn, PipeMsgHead *msg)
			{
				if (msg->length > 0)
				{
					std::string pipeId = std::string((const char *)msg + sizeof(PipeMsgHead), msg->length);
					dlog("get handshake from client,  pipeid is {}", pipeId);

					auto pipe = find_bind_pipe(pipeId);
					if (pipe)
					{
						pipe->bind(conn);
						ilog("bind pipe {} success", pipeId);
						PipeMsgHead shakeMsg(PIPE_MSG_SHAKE_HAND, pipeId.length());
						conn->msend(std::string((const char *)&shakeMsg, sizeof(PipeMsgHead)), pipeId);
						pipe->handle_event(NetEvent::EVT_CONNECT);
					}
					else
					{
						wlog("pipe id not found {}", pipeId);
						conn->close();
					}
				}
				else
				{
					//create a pipeid to client
					auto pipeId = this->generate_id();
					auto session = find_unbind_pipe(conn->get_cid());
		 
					if (session)
					{		 
						session->update_pipeid(pipeId);
					}
					else
					{
						dlog("create and bind session success {}", pipeId);
						session = std::make_shared<PipeSession>();
						session->bind(conn);
						add_unbind_pipe(conn->get_cid(), session);
					}

					if (session)
					{
						PipeMsgHead shakeMsg(PIPE_MSG_SHAKE_HAND, pipeId.length());
						conn->msend(std::string((const char *)&shakeMsg, sizeof(PipeMsgHead)), pipeId);
						session->handle_event(NetEvent::EVT_CONNECT);
					}
				}
			}

			void process_client_handshake(TPtr conn, PipeMsgHead *msg)
			{

				if (msg->length > 0)
				{
					std::string pipeId = std::string((const char *)msg + sizeof(PipeMsgHead), msg->length);
					dlog("get handshake from server,  pipeid is {}", pipeId);
					PipeSessionPtr session = find_bind_pipe(pipeId);
					if (session)
					{					 
						session->bind(conn);
						session->update_pipeid(pipeId);
						dlog("bind pipe session success {}", pipeId);
						session->handle_event(NetEvent::EVT_CONNECT);
					}
					else
					{
						dlog("try to find pipe by cid {} size is {}", conn->get_cid(), unbind_pipes.size());
						session = find_unbind_pipe(conn->get_cid());

						if (session)
						{
							session->bind(conn);
							session->update_pipeid(pipeId);

							dlog("bind session success");
							session->handle_event(NetEvent::EVT_CONNECT);

							add_bind_pipe(pipeId, session);

							remove_unbind_pipe(conn->get_cid());
						}
						else
						{
							wlog("pipe id not found {} cid is {}", pipeId, conn->get_cid());
							conn->close();
						}
					}
				}
				else
				{

					send_shakehand(conn, conn->get_pipeid());
					wlog("handshake from server is empty, resend client shakehand {}", conn->get_cid());
				}
			}

			void add_unbind_pipe(uint64_t cid, PipeSessionPtr pipe)
			{
				std::lock_guard<std::mutex> guard(unbind_mutex);
				unbind_pipes[cid] = pipe;
			}
			PipeSessionPtr find_unbind_pipe(uint64_t cid)
			{
				std::lock_guard<std::mutex> guard(unbind_mutex);
				auto itr = unbind_pipes.find(cid);
				if (itr != unbind_pipes.end())
				{
					return itr->second;
				}
				return nullptr;
			}
			bool remove_unbind_pipe(uint64_t cid)
			{
				std::lock_guard<std::mutex> guard(unbind_mutex);
				return unbind_pipes.erase(cid) > 0;
			}

			void add_bind_pipe(const std::string &pipeId, PipeSessionPtr pipe)
			{
				std::lock_guard<std::mutex> guard(bind_mutex);
				bind_pipes[pipeId] = pipe;
			}

			PipeSessionPtr find_bind_pipe(const std::string &pipeId)
			{
				std::lock_guard<std::mutex> guard(bind_mutex);
				auto itr = bind_pipes.find(pipeId);
				if (itr != bind_pipes.end())
				{
					return itr->second;
				}
				return nullptr;
			}
			bool remove_bind_pipe(const std::string &pipeId)
			{
				std::lock_guard<std::mutex> guard(bind_mutex);
				return bind_pipes.erase(pipeId) > 0;
			}

			void send_shakehand(TPtr conn, const std::string &pipeId)
			{

				dlog("shakehande request pipe id {}  ", pipeId);
				PipeMsgHead shakeMsg(PIPE_MSG_SHAKE_HAND, pipeId.length());
				conn->msend(std::string((const char *)&shakeMsg, sizeof(PipeMsgHead)), pipeId);

				// if (!pipeId.empty()) {

				// conn->send(shakeMsg.begin(), shakeMsg.length());
				// }
			}

			void broadcast(const char *pData, uint32_t len)
			{
				std::lock_guard<std::mutex> guard(bind_mutex);
				for (auto &item : bind_pipes)
				{
					if (item.second)
					{
						item.second->transfer(pData, len);
					}
				}

				// for (auto& item : unbind_pipes) {
				// 	if (item.second) {
				// 		item.second->transfer(pData, len);
				// 	}
				// }
			}
			void register_pipe(PipeSessionPtr pipe, uint64_t cid = 0)
			{
				if (pipe)
				{
					if (pipe->get_pipeid().empty())
					{
						if (cid != 0)
						{
							dlog("add unbind pipe {}", cid);
							add_unbind_pipe(cid, pipe);
						}
					}
					else
					{
						dlog("add normal pipe {}", pipe->get_pipeid());

						add_bind_pipe(pipe->get_pipeid(), pipe);
					}
				}
			}

			void start_clients(std::function<void(PipeSessionPtr)> handler)
			{

				{
					std::lock_guard<std::mutex> guard(bind_mutex);
					for (auto &item : bind_pipes)
					{
						auto &pipe = item.second;
						if (pipe && pipe->get_port() != 0 && !pipe->get_host().empty())
						{
							dlog("start connect pipe to {}:{}", pipe->get_host(), pipe->get_port());
							if (handler)
							{
								handler(pipe);
							}
						}
					}
				}

				{
					std::lock_guard<std::mutex> guard(unbind_mutex);
					for (auto &item : unbind_pipes)
					{
						auto &pipe = item.second;
						if (pipe && pipe->get_port() != 0 && !pipe->get_host().empty())
						{
							dlog("start connect pipe to {}:{}", pipe->get_host(), pipe->get_port());
							if (handler)
							{
								handler(pipe);
							}
						}
					}
				}
			}

			PipeMode get_mode() { return pipe_mode; }

		private:
			PipeMode pipe_mode;
			std::mutex bind_mutex;
			std::unordered_map<std::string, PipeSessionPtr> bind_pipes;

			std::mutex unbind_mutex;
			std::unordered_map<uint64_t, PipeSessionPtr> unbind_pipes;
		};

		using PipeFactoryPtr = std::shared_ptr<PipeFactory>;
	}

}
