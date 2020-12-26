#pragma once 


namespace knet
{


 template <class T> 
 class EventHandler {
     public: 

    virtual void handle_data(std::shared_ptr<T>    , const std::string & msg  ) = 0; 
    virtual void handle_event(std::shared_ptr<T>  , NetEvent ) = 0;   

 }; 


}