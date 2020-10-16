

#include "klog.hpp"
#include "file_sink.hpp"
#include "console_sink.hpp"
#include <chrono>



using namespace klog;


int main(int argc, char **argv)
{

    KLog::instance().init_async(true);
    KLog::instance().add_sink<FileSink>("./klog.log");
    KLog::instance().add_sink<ConsoleSink>();
    uint32_t index =0; 
    while(true){


//        dout <<" dout log from xout " << index ;
//        iout <<" iout log from xout " << index ;
//        wout <<" wout log from xout " << index ;
//        eout <<" eout log from xout " << index ; 
        //dlog("debug log from dlog"); 
        //elog("debug log from elog"); 
        //        dlog("debug log from xlog {}  ", index ); 
        //        ilog("info log from xlog  {}  ", index ); 
        //        wlog("warn log from xlog  {}  ", index ); 
        //        elog("error log from xlog {}  ", index ); 


        dput("dput log from xput", index); 
        dput("dput log from xput", index); 
        iput("iput log from xput", index); 
        wput("wput log from xput", index); 
        eput("eput log from xput", index); 


        index ++; 
        std::this_thread::sleep_for(std::chrono::seconds(1)); 

    }


    return 0;

}
