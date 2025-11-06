cmake_minimum_required(VERSION 3.10)
project(hello_world)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent directory framework sources
set(FRAMEWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_executable(hello_world
    main.cpp
    ${FRAMEWORK_DIR}/framework/app.cpp
    ${FRAMEWORK_DIR}/framework/router.cpp
    ${FRAMEWORK_DIR}/framework/request.cpp
    ${FRAMEWORK_DIR}/framework/response.cpp
)

target_include_directories(hello_world PRIVATE ${FRAMEWORK_DIR})

# Link threads
find_package(Threads REQUIRED)
target_link_libraries(hello_world PRIVATE Threads::Threads)
