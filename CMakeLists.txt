cmake_minimum_required(VERSION 3.10)
project(http_server)

set(CMAKE_CXX_STANDARD 20)

include_directories(framework)

add_executable(http_server
    main.cpp
    thread_pool.h
    framework/logger.h
    framework/request.h
    framework/request.cpp
    framework/response.h
    framework/response.cpp
    framework/router.h
    framework/router.cpp
    framework/app.h
    framework/app.cpp
    framework/BlazeServer.h
    framework/BlazeServer.cpp
)

