cmake_minimum_required(VERSION 3.20)
project(blaze)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(framework)

option(BLAZE_BUILD_EXAMPLES "Build the educational examples" ON)
if(BLAZE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()
