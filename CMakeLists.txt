cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project ("多线程并发服务器")

add_executable (server server.cpp)
add_executable (client client.cpp)

