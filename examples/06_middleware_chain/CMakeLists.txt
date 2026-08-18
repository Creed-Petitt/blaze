cmake_minimum_required(VERSION 3.20)
project(blaze_example_06_middleware_chain LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(06_middleware_chain main.cpp)
target_link_libraries(06_middleware_chain PRIVATE blaze::core)
