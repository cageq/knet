

#include "klog.hpp"



using namespace klog; 




int main(int argc, char **argv)
{

    KLog::instance().add_file_sink("./klog.log"); 


    dout <<" test tcp server with cout format"; 

    return 0; 

}
