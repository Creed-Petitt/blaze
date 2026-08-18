cmake_minimum_required(VERSION 3.20)
project(blaze_example_03_realtime_chat LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(03_realtime_chat main.cpp)
target_link_libraries(03_realtime_chat PRIVATE blaze::core)
