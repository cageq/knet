#pragma once

#include "knet.hpp"
using namespace knet::tcp;

namespace knet{
	namespace pipe{



		class PipeSession;

		class PipeConnection : public TcpConnection<PipeConnection> {
			public:
				PipeConnection(const std::string& pid = "") { pipeid = pid; }

				std::shared_ptr<PipeSession> session;

				std::string pipeid;
		};

		using PipeConnectionPtr = std::shared_ptr<PipeConnection>; 
	}

}
