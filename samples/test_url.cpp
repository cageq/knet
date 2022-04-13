#include "utils/knet_url.hpp"

#include <iostream> 



using namespace knet::utils; 



int main(int argc, char * argv[]){



    KNetUrl urlInfo; 
    auto info = urlInfo.parse("tcp://127.0.0.1:8999?delay=0&sendsize=180000&recvsize=33333"); 

    std::cout << urlInfo.dump() << std::endl ; 
    std::cout << urlInfo.encode() << std::endl; 
 
    info = urlInfo.parse("tcp://127.0.0.1:8999"); 

    std::cout << urlInfo.dump() << std::endl ; 
    std::cout << urlInfo.encode() << std::endl; 
}; 
