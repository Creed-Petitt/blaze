cmake_minimum_required(VERSION 3.10)
project(http_server)

set(CMAKE_CXX_STANDARD 20)

add_executable(http_server main.cpp thread_pool.h logger.h)