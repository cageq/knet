//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once 

namespace knet {
	namespace tcp{

		template <class T> 
			class ConnectionFactory{ 
				public:
				using TPtr = std::shared_ptr<T> ; 

				using ConnInitor = std::function<void(T & )>  ; 

				// using FactoryEventHandler = std::function<void (TPtr, NetEvent)>; 
				// using FactoryDataHandler =  std::function<int (TPtr , const std::string_view &)>;  

				
				template <class ... Args>
				TPtr create(Args ... args ) {
					dlog("connection factory create connection"); 
					return  std::make_shared<T> (args...); 							
				}

				virtual void destroy(TPtr conn) {
					//dlog("connection factory destroy connection"); 
				}	

				virtual void handle_event(TPtr conn, NetEvent evt) {
					ilog("handle event in connection factory {}", evt); 
				}

		
				virtual uint32_t handle_data(TPtr conn, const std::string_view & msg, MessageStatus status) { 

					//ilog("handle data in connection factory {}",len); 
					return msg.length() ;
				}; 
	
			}; 


	}

}



