#include "mt_server.h"

std::mutex data_mutex; 
struct NetMsg{ 
	std::shared_ptr<TcpSession> session ; 
	std::string msg; 
}; 

std::vector<NetMsg> data_buffer; 

void * user_task() 
{ 

	while(true) { 
		if (!data_buffer.empty()){ 

			knet_dlog("process message in user thread" ); 
			data_mutex.lock(); 
			for(auto & msg:data_buffer) 
			{
				if (msg.session) {
					msg.session->send(msg.msg); 
				}

			}
			data_buffer.clear(); 
			data_mutex.unlock(); 

		}
		else { 
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	} 
	return nullptr; 
}


int TcpSession::on_recv(char *pBuf, uint32_t len) {
	//		knet_dlog(" connection id %d on thread %d", m_id, std::this_thread::get_id());
	//		knet_dlog("received data %s ", pBuf);

	data_mutex.lock(); 
	NetMsg msg { shared_from_this(), std::string(pBuf,len)}; 
	data_buffer.push_back(msg); 
	data_mutex.unlock(); 
	return len; 
}



void MyFactory::destroy(TPtr conn) {
	knet_dlog("connection factory destroy connection in factory "); 
}	

		void MyFactory::handle_event(TPtr conn, NetEvent evt) { 
	knet_ilog("### handle event in my connection factory ", evt); 
	if (evt == EVT_CONNECT) 
	{
	} else if (evt == EVT_DISCONNECT) 
	{
	}
}
int32_t MyFactory::handle_data(TPtr conn,  char * data, uint32_t len) { 
	return len ;
}; 




int main(int argc, char **argv)
{
	MyFactory factory; 
	knet_dlog("start server");
	Listener<TcpSession> listener(factory);
	int port = 8899;
	listener.start(  port); 
	knet_dlog("start server on port %d", port);

	//pthread_t tid ; 
	//pthread_create(&tid,nullptr, &user_task, nullptr); 
	std::thread user_thread(&user_task); 
	char c = getchar();
	while (c)
	{
		if (c == 'q')
		{
			printf("quiting ...\n");
			break;
		}
		c = getchar();
	}
	user_thread.join(); 

	knet_dlog("quit server");
	return 0;
}
