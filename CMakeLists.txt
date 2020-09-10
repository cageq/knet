cmake_minimum_required (VERSION 3.1.0)
project (knet)

set(LIBPATH "$ENV{HOME}/app")


include_directories(".")
include_directories("include")
include_directories("deps/include")
include_directories("klog")
include_directories("include/tcp")
include_directories("include/udp")
include_directories("include/pipe")
include_directories("opt")
include_directories("include")
include_directories("http")
include_directories("utils")
 
include_directories("opt/libgo/libgo")
include_directories("kcp")


include_directories("${LIBPATH}/include")
link_directories("${LIBPATH}/lib") 
link_directories("deps/lib") 
link_directories("/usr/local/lib") 

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(APPLE)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc++11-extensions -std=c++11 -stdlib=libc++  -g  -D NDEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc++17-extensions -Wall -std=c++17 -stdlib=libc++  -Wno-unused-function -g  ")
  include_directories("/usr/local/Cellar/openssl/1.0.2t/include")
  link_directories("/usr/local/Cellar/openssl/1.0.2t/lib") 
  ADD_DEFINITIONS(-fcoroutines-ts --stdlib=libc++ -Xclang -fconcepts-ts)
elseif(UNIX)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11 -pthread  -D NDEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -pthread  -g  -O2 -Wno-unused-function ")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pthread -pg -g -DNDEBUG -Wunused-value ")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -D NDEBUG")
endif()


ADD_DEFINITIONS(-DASIO_STANDALONE)

#add the executable
#add_library(knet ${SRCS})
#add_executable(server samples/server.cpp  )
#add_executable(mtserver samples/mt_server.cpp  )
add_executable(tcpserver samples/tcpserver.cpp  )
target_link_libraries(tcpserver fmt dl)

add_executable(tcpclient samples/tcpclient.cpp)
target_link_libraries(tcpclient fmt)
add_executable(fserver  samples/factory_server.cpp)
target_link_libraries(fserver fmt)
 
# add_executable(tlsclient samples/tlsclient.cpp)
# add_executable(tlsserver samples/tlsserver.cpp)
# target_link_libraries(tlsclient fmt ssl crypto)
# target_link_libraries(tlsserver fmt ssl crypto)
 

 add_executable(webserver samples/webserver.cpp   )
 target_link_libraries(webserver fmt  dl)

add_executable(webclient samples/webclient.cpp   )
target_link_libraries(webclient fmt)


# add_executable(websslserver samples/websslserver.cpp   )
# target_link_libraries(websslserver fmt ssl crypto)

add_executable(wsclient samples/wsclient.cpp    )
target_link_libraries(wsclient fmt   )


 add_executable(wsserver samples/wsserver.cpp    )
 target_link_libraries(wsserver fmt )


# add_executable(wssclient samples/wssclient.cpp    )
# target_link_libraries(wssclient fmt ssl crypto )

# add_executable(wssserver samples/wssserver.cpp    )
# target_link_libraries(wssserver fmt ssl crypto)

add_executable(kcpserver samples/kcpserver.cpp  kcp/ikcp.c)
target_link_libraries(kcpserver fmt )

add_executable(kcpclient samples/kcpclient.cpp  kcp/ikcp.c)
target_link_libraries(kcpclient fmt )


 
add_executable(udpclient samples/udpclient.cpp  )
target_link_libraries(udpclient fmt)


add_executable(udpserver samples/udpserver.cpp  )
target_link_libraries(udpserver fmt)


# add_executable(mudpclient samples/mudpclient.cpp  )
# target_link_libraries(mudpclient fmt)
# add_executable(mudpserver samples/mudpserver.cpp  )
# target_link_libraries(mudpserver fmt)


# add_executable(seqserver samples/seqserver.cpp   )
# add_executable(seqclient samples/seqclient.cpp   )

# target_link_libraries(seqserver fmt)
# target_link_libraries(seqclient fmt)

 
add_executable(pipeserver samples/pipe_server.cpp  )
target_link_libraries(pipeserver fmt)

add_executable(pipeclient samples/pipe_client.cpp  )
target_link_libraries(pipeclient fmt)

add_executable(logger samples/logger.cpp  )
target_link_libraries(logger fmt)

