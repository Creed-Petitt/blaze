cmake_minimum_required(VERSION 3.20)
project(blaze_example_12_file_uploads LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/blaze")

add_executable(12_file_uploads main.cpp)
target_link_libraries(12_file_uploads PRIVATE blaze::core)
