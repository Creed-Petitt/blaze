cmake_minimum_required(VERSION 3.15)
project(blaze_framework)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch modern Boost 1.85.0 (Source Only)
message(STATUS "Blaze: Fetching modern Boost 1.85.0 engine...")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    FetchContent_Declare(
        boost_all
        URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE 
    )
else()
    FetchContent_Declare(
        boost_all
        URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
    )
endif()

FetchContent_GetProperties(boost_all)
if(NOT boost_all_POPULATED)
    FetchContent_Populate(boost_all)
endif()

# boost_system
if(NOT TARGET boost_system)
    add_library(boost_system STATIC
        ${boost_all_SOURCE_DIR}/libs/system/src/error_code.cpp
    )
    target_include_directories(boost_system PUBLIC ${boost_all_SOURCE_DIR})
    add_library(Boost::system ALIAS boost_system)
endif()

# boost_thread
if(NOT TARGET boost_thread)
    find_package(Threads REQUIRED)
    file(GLOB BOOST_THREAD_SOURCES 
        "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/thread.cpp"
        "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/once.cpp"
    )
    if(NOT BOOST_THREAD_SOURCES)
         file(GLOB_RECURSE BOOST_THREAD_SOURCES "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/*.cpp")
    endif()

    add_library(boost_thread STATIC ${BOOST_THREAD_SOURCES})
    target_include_directories(boost_thread PUBLIC ${boost_all_SOURCE_DIR})
    target_link_libraries(boost_thread PUBLIC Threads::Threads)
    add_library(Boost::thread ALIAS boost_thread)
endif()

# OPTIONAL DRIVER DETECTION
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(MARIADB QUIET libmariadb)
endif()

find_package(OpenSSL REQUIRED)
find_package(PostgreSQL QUIET)

# --- TARGET: BLAZE_CORE ---
set(CORE_SOURCES
    src/app.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/environment.cpp
    src/json.cpp
    src/crypto.cpp
    src/client.cpp
    src/multipart.cpp
    src/logger.cpp
    src/util/string.cpp
    src/util/circuit_breaker.cpp
    src/db_result.cpp
    src/middleware.cpp
)

add_library(blaze_core STATIC ${CORE_SOURCES})
add_library(blaze::core ALIAS blaze_core)

target_include_directories(blaze_core BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
)

target_link_libraries(blaze_core PUBLIC 
    Boost::system 
    Boost::thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# TARGET: BLAZE_MYSQL
if(MARIADB_FOUND)
    set(MYSQL_SOURCES
        src/drivers/mysql/mysql_connection.cpp
        src/drivers/mysql/mysql_pool.cpp
        src/drivers/mysql/mysql_result.cpp
    )
    add_library(blaze_mysql STATIC ${MYSQL_SOURCES})
    add_library(blaze::mysql ALIAS blaze_mysql)
    
    target_link_libraries(blaze_mysql PUBLIC blaze::core ${MARIADB_LIBRARIES})
    target_include_directories(blaze_mysql PUBLIC ${MARIADB_INCLUDE_DIRS})
    target_link_directories(blaze_mysql PUBLIC ${MARIADB_LIBRARY_DIRS})
    message(STATUS "Blaze: MySQL driver enabled.")
else()
    message(STATUS "Blaze: MySQL driver disabled (libmariadb not found).")
endif()

# TARGET: BLAZE_POSTGRES
if(PostgreSQL_FOUND)
    set(PG_SOURCES
        src/drivers/postgres/pg_connection.cpp
        src/drivers/postgres/pg_pool.cpp
        src/drivers/postgres/pg_result.cpp
    )
    add_library(blaze_postgres STATIC ${PG_SOURCES})
    add_library(blaze::postgres ALIAS blaze_postgres)
    
    target_link_libraries(blaze_postgres PUBLIC blaze::core PostgreSQL::PostgreSQL)
    target_include_directories(blaze_postgres PUBLIC ${PostgreSQL_INCLUDE_DIRS})
    message(STATUS "Blaze: PostgreSQL driver enabled.")
else()
    message(STATUS "Blaze: PostgreSQL driver disabled (libpq not found).")
endif()

# Sanitizers for core
if(BLAZE_ENABLE_SANITIZERS)
    target_compile_options(blaze_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(blaze_core PUBLIC -fsanitize=address,undefined)
endif()

if(BLAZE_ENABLE_TSAN)
    target_compile_options(blaze_core PUBLIC -fsanitize=thread)
    target_link_options(blaze_core PUBLIC -fsanitize=thread)
endif()

