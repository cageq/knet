cmake_minimum_required (VERSION 3.1.0)
project (knet)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")
#include_directories("opt/asio/asio/include")
#include_directories("opt/asio-1.21.0/include")
include_directories("opt/asio-master/asio/include")
include_directories("opt/spdlog-1.9.2/include")
 
include_directories("kcp")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall  -stdlib=libc++  -Wno-unused-function -g  ")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g -O2 -Wno-unused-function -luring ")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D NDEBUG")
endif()

include_directories(opt/fmt-8.0.1/include) 
add_subdirectory(opt/fmt-8.0.1 EXCLUDE_FROM_ALL) 

ADD_DEFINITIONS(-DASIO_STANDALONE)
ADD_DEFINITIONS(-DASIO_DISABLE_EPOLL)
ADD_DEFINITIONS(-DASIO_HAS_IO_URING)

#add the executable
#add_executable(server samples/server.cpp  )
#add_executable(mtserver samples/mt_server.cpp  )
add_executable(tcpserver samples/tcpserver.cpp  )
target_link_libraries(tcpserver uring)

#add_executable(tcpclient samples/tcpclient.cpp)

if(UNIX)
add_executable(timeclient samples/timeclient.cpp)
target_link_libraries(timeclient  uring)
endif()

add_executable(fserver  samples/factory_server.cpp)
target_link_libraries(fserver  uring)
 

add_executable(webserver samples/webserver.cpp   )
target_link_libraries(webserver uring)

add_executable(webclient samples/webclient.cpp   )
target_link_libraries(webclient uring)

add_executable(wsclient samples/wsclient.cpp    )
target_link_libraries(wsclient uring)

add_executable(wsserver samples/wsserver.cpp    )
target_link_libraries(wsserver uring)


add_executable(kcpserver samples/kcpserver.cpp  kcp/ikcp.c)
target_link_libraries(kcpserver uring)

add_executable(kcpclient samples/kcpclient.cpp  kcp/ikcp.c)
target_link_libraries(kcpclient uring)
 
add_executable(udpclient samples/udpclient.cpp  )
target_link_libraries(udpclient uring)

add_executable(udpserver samples/udpserver.cpp  )
target_link_libraries(udpserver uring)

add_executable(pipeserver samples/pipe_server.cpp  )
target_link_libraries(pipeserver uring)

add_executable(pipeclient samples/pipe_client.cpp  )
target_link_libraries(pipeclient uring)

add_executable(unbindpipeclient samples/unbind_pipe_client.cpp  )
target_link_libraries(unbindpipeclient uring)

add_executable(unbindpipeserver samples/unbind_pipe_server.cpp  )
target_link_libraries(unbindpipeserver uring)
