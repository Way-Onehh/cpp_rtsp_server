cmake_minimum_required(VERSION 3.12)
project(rtsp_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# message("启用asan")
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(network STATIC network/dgram_server.cpp network/rtp_server.cpp network/stream_server.cpp network/udpchannel.cpp network/tcpchannel.cpp network/udp_config.cpp network/tcp_config.cpp)
target_include_directories(network   PRIVATE . )

add_library(utility STATIC  utility/threadpool_imp.cpp)
target_include_directories(utility   PRIVATE .)

add_executable(server server.cpp)
target_include_directories(server PRIVATE . )
target_link_libraries(server PRIVATE network utility)