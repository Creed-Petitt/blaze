cmake_minimum_required(VERSION 3.20)
project(blaze_example_10_static_site LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(10_static_site main.cpp)
target_link_libraries(10_static_site PRIVATE blaze::core)
