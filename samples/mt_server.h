#pragma once 
#include <stdio.h>
#include "knet.hpp"
#include <iostream>



using namespace knet::tcp;


class MyFactory; 



class TcpSession : public TcpConnection<TcpSession> {
	public: 
		typedef std::shared_ptr<TcpSession> TcpSessionPtr; 
		TcpSession() {
			bind_data_handler(&TcpSession::on_recv ); 

			bind_event_handler([this](  TcpSessionPtr,NetEvent evt){
					
					return 0; 
					} ); 
		}

		virtual ~TcpSession() {
			dlog("destroy tcp session");
		}


		//read on buffer for one package
		//return one package length ,if not enough return -1
		int read_packet(const char *pData, uint32_t dataLen) {
			return dataLen;
		}

		void on_disconnect() {
			dlog("on session disconnect");
			//   trace;
		}

		//will invoke in multi-thread , if you want to process it main thread , push it to msg queue
		int on_recv(char *pBuf, uint32_t len) ; 

	private:
		int m_id;
};



class MyFactory: public ConnFactory<TcpSession > { 

	public:

 

		virtual void handle_event(TPtr conn, NetEvent evt) ; 

		virtual bool handle_data(TPtr conn, const char * data, uint32_t len) ;  


}; 


