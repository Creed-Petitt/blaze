cmake_minimum_required(VERSION 3.15)
project(blaze_framework)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch modern Boost 1.85.0
message(STATUS "Blaze: Fetching modern Boost 1.85.0 engine. The first build will take ~3-5 minutes...")
FetchContent_Declare(
    boost_all
    URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
)

set(BOOST_ENABLE_CMAKE ON CACHE BOOL "")
set(BOOST_INCLUDE_LIBRARIES system thread charconv CACHE STRING "")

FetchContent_MakeAvailable(boost_all)

# --- DETECT SYSTEM LIBRARIES (MANDATORY) ---
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_library(MARIADB_LIB NAMES mariadb mariadbclient REQUIRED)

# --- THE MONOLITHIC BLAZE LIBRARY ---
set(BLAZE_SOURCES
    src/app.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/crypto.cpp
    src/environment.cpp
    src/json.cpp
    # MySQL Driver
    src/drivers/mysql/mysql_connection.cpp
    src/drivers/mysql/mysql_pool.cpp
    src/drivers/mysql/mysql_result.cpp
    # Postgres Driver
    src/drivers/postgres/pg_connection.cpp
    src/drivers/postgres/pg_pool.cpp
    src/drivers/postgres/pg_result.cpp
)

add_library(blaze_framework STATIC ${BLAZE_SOURCES})
add_library(blaze::framework ALIAS blaze_framework)

# Legacy aliases
add_library(blaze::core ALIAS blaze_framework)
add_library(blaze::mysql ALIAS blaze_framework)
add_library(blaze::postgres ALIAS blaze_framework)

target_include_directories(blaze_framework BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
    ${PostgreSQL_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIR}
)

target_include_directories(blaze_framework PRIVATE 
    src
    src/drivers/mysql
    src/drivers/postgres
)

target_link_libraries(blaze_framework PUBLIC 
    boost_system 
    boost_thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${MARIADB_LIB}
    PostgreSQL::PostgreSQL
)