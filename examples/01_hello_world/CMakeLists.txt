cmake_minimum_required(VERSION 3.20)
project(blaze_example_01_hello_world LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(01_hello_world main.cpp)
target_link_libraries(01_hello_world PRIVATE blaze::core)
