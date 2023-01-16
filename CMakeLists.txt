cmake_minimum_required (VERSION 3.1.0)
project (knet)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")
#include_directories("opt/asio/asio/include")
include_directories("opt/asio-1.24.0/include")
include_directories("opt/spdlog-1.11.0/include")
 
include_directories("kcp")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall  -stdlib=libc++  -Wno-unused-function -g  ")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread   -g -Wno-unused-function -O2 ")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D NDEBUG")
endif()

include_directories(opt/fmt-9.1.0/include) 
add_subdirectory(opt/fmt-9.1.0 EXCLUDE_FROM_ALL) 

ADD_DEFINITIONS(-DASIO_STANDALONE)

#add the executable
#add_executable(server samples/server.cpp  )
#add_executable(mtserver samples/mt_server.cpp  )
add_executable(tcpserver samples/tcpserver.cpp  ) 

add_executable(tcpclient samples/tcpclient.cpp) 

add_executable(fserver  samples/factory_server.cpp)  

add_executable(webserver samples/webserver.cpp   ) 

add_executable(webclient samples/webclient.cpp   ) 

add_executable(wsclient samples/wsclient.cpp    ) 

add_executable(wsserver samples/wsserver.cpp    )

add_executable(kcpserver samples/kcpserver.cpp  kcp/ikcp.c)

add_executable(udpclient samples/udpclient.cpp  ) 

add_executable(udpserver samples/udpserver.cpp  ) 

add_executable(pipeserver samples/pipe_server.cpp  ) 

add_executable(pipeclient samples/pipe_client.cpp  ) 

add_executable(unbindpipeclient samples/unbind_pipe_client.cpp  ) 

add_executable(unbindpipeserver samples/unbind_pipe_server.cpp  ) 

add_executable(testurl samples/test_url.cpp  )


add_executable(timeclient samples/timeclient.cpp)

add_executable(timeserver samples/timeserver.cpp)
