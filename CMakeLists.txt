cmake_minimum_required (VERSION 3.10.0)
project (knet)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(include)
include_directories(deps/fmt/include)
include_directories(deps/asio)
include_directories(deps/spdlog/include)

if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    	FIND_PACKAGE(OpenSSL)
    	if(${OpenSSL_FOUND})
    	  include_directories(${OPENSSL_INCLUDE_DIR})
    	link_directories(${OPENSSL_ROOT_DIR}/lib)
    	endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall  -stdlib=libc++  -Wno-unused-function -g  -Wno-deprecated-declarations")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread   -g -Wno-unused-function -O2  -Wno-unused-variable ")
 
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O2")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -D NDEBUG")
endif()


ADD_DEFINITIONS(-DASIO_STANDALONE)
add_definitions(-DFMT_HEADER_ONLY=ON)
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

add_executable(kcpserver samples/kcpserver.cpp  include/kcp/ikcp.c)

add_executable(kcpclient samples/kcpclient.cpp  include/kcp/ikcp.c)
 
add_executable(udpclient samples/udpclient.cpp  )

add_executable(udpserver samples/udpserver.cpp  ) 

add_executable(pipeserver samples/pipe_server.cpp  ) 

add_executable(pipeclient samples/pipe_client.cpp  ) 

add_executable(unbindpipeclient samples/unbind_pipe_client.cpp  ) 

add_executable(unbindpipeserver samples/unbind_pipe_server.cpp  ) 

add_executable(testurl samples/test_url.cpp  )


add_executable(wssclient samples/wssclient.cpp    )
target_link_libraries(wssclient ssl crypto dl ) 

add_executable(wssserver samples/wssserver.cpp    )
target_link_libraries(wssserver ssl crypto dl) 

add_executable(tlsclient samples/tlsclient.cpp  )
target_link_libraries(tlsclient ssl crypto dl) 

add_executable(tlsserver samples/tlsserver.cpp  )
target_link_libraries(tlsserver ssl crypto dl) 


add_executable(timeclient samples/timeclient.cpp)

add_executable(timeserver samples/timeserver.cpp)
