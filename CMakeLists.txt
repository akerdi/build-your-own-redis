cmake_minimum_required(VERSION 3.15)
project(learn_redis)

set(CMAKE_CXX_STANDARD 11)

add_executable(learn_redis main.cpp server.cpp)
