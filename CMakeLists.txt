cmake_minimum_required (VERSION 3.1.0)
project (knet)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")
#include_directories("opt/asio/asio/include")
include_directories("opt/asio-1.19.2/include")
include_directories("opt/spdlog-1.9.2/include")
 
include_directories("kcp")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall  -stdlib=libc++  -Wno-unused-function -g  ")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread   -g -Wno-unused-function ")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D NDEBUG")
endif()

include_directories(opt/fmt-8.0.1/include) 
add_subdirectory(opt/fmt-8.0.1 EXCLUDE_FROM_ALL) 

ADD_DEFINITIONS(-DASIO_STANDALONE)

#add the executable
#add_executable(server samples/server.cpp  )
#add_executable(mtserver samples/mt_server.cpp  )
add_executable(tcpserver samples/tcpserver.cpp  )
#target_link_libraries(tcpserver fmt )

add_executable(tcpclient samples/tcpclient.cpp)
#target_link_libraries(tcpclient fmt)

add_executable(timeclient samples/timeclient.cpp)

add_executable(fserver  samples/factory_server.cpp)
#target_link_libraries(fserver fmt)
 

add_executable(webserver samples/webserver.cpp   )
#target_link_libraries(webserver fmt  dl)

add_executable(webclient samples/webclient.cpp   )
#target_link_libraries(webclient fmt)

add_executable(wsclient samples/wsclient.cpp    )
#target_link_libraries(wsclient fmt   )

add_executable(wsserver samples/wsserver.cpp    )
#target_link_libraries(wsserver fmt )


add_executable(kcpserver samples/kcpserver.cpp  kcp/ikcp.c)
#target_link_libraries(kcpserver fmt )

add_executable(kcpclient samples/kcpclient.cpp  kcp/ikcp.c)
#target_link_libraries(kcpclient fmt )
 
add_executable(udpclient samples/udpclient.cpp  )
#target_link_libraries(udpclient fmt)

add_executable(udpserver samples/udpserver.cpp  )
#target_link_libraries(udpserver fmt)

add_executable(pipeserver samples/pipe_server.cpp  )
#target_link_libraries(pipeserver fmt)

add_executable(pipeclient samples/pipe_client.cpp  )
#target_link_libraries(pipeclient fmt)

add_executable(unbindpipeclient samples/unbind_pipe_client.cpp  )
#target_link_libraries(unbindpipeclient fmt)

add_executable(unbindpipeserver samples/unbind_pipe_server.cpp  )
#target_link_libraries(unbindpipeserver fmt)



add_executable(testurl samples/test_url.cpp  )