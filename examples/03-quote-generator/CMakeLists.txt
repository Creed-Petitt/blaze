cmake_minimum_required(VERSION 3.10)
project(quote_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent directory framework sources
set(FRAMEWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_executable(quote_generator
    main.cpp
    ${FRAMEWORK_DIR}/framework/app.cpp
    ${FRAMEWORK_DIR}/framework/router.cpp
    ${FRAMEWORK_DIR}/framework/request.cpp
    ${FRAMEWORK_DIR}/framework/response.cpp
)

target_include_directories(quote_generator PRIVATE ${FRAMEWORK_DIR})

# Link threads
find_package(Threads REQUIRED)
target_link_libraries(quote_generator PRIVATE Threads::Threads)
