#ifndef __PLAT_H__
#define __PLAT_H__

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
   #else
      //define something for Windows (32-bit only)
   #endif


   #define _WINSOCK_DEPRECATED_NO_WARNINGS
//
//   #define WIN32_LEAN_AND_MEAN
//   #include <windows.h>
//   #include <winsock2.h>
//   #pragma comment(lib, "Ws2_32.lib")
//
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif

    #include <sys/socket.h>
    #include <sys/event.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <errno.h>
    #include <string.h>
    #include <stdlib.h>
    #include <pthread.h>

#elif __ANDROID__
    // android
#elif __linux__
    // linux
	#include <sys/epoll.h> 
#include <unistd.h>
   //#define INVALID_SOCKET  (SOCKET)(~0)
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif


#include <iostream>
#include <chrono>
#include <thread>


#define MAX_THREAD 50
#define MAX_WAIT_EVENT 128 
//#define trace { printf("[stack]%s:%d\n", __FUNCTION__, __LINE__); }


#ifndef NDEBUG



//#define Log(msg) do{ std::cout << __FILE__ << "(@" << __LINE__ << "): " << msg << '\n'; } while( false )
#define Log(msg) do{ std::cout <<   msg << '\n'; } while( false )
//#define Log(msg) do{ std::cout << __FILE__ << "(@" << __LINE__ << "): " << msg << '\n'; } while( false )
#define Trace( format, ... )   printf( "%s::%s(%d)" format, __FILE__, __FUNCTION__,  __LINE__, ##__VA_ARGS__ )
#else
#define Log(msg) 
#define Trace( format, ... )
#endif

struct NoneMutex{ 
	inline void lock() { 
	}
	inline void unlock() { 
	}
}; 


#endif


