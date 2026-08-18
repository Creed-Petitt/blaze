cmake_minimum_required(VERSION 3.20)
project(blaze_example_09_background_tasks LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(09_background_tasks main.cpp)
target_link_libraries(09_background_tasks PRIVATE blaze::core)
