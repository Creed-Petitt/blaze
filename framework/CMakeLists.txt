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
    # Thread requires pthread on Linux
    find_package(Threads REQUIRED)
    
    file(GLOB BOOST_THREAD_SOURCES 
        "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/thread.cpp"
        "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/once.cpp"
    )
    
    # Fallback if specific files change, just grab the src/pthread content mostly
    if(NOT BOOST_THREAD_SOURCES)
         file(GLOB_RECURSE BOOST_THREAD_SOURCES "${boost_all_SOURCE_DIR}/libs/thread/src/pthread/*.cpp")
    endif()

    add_library(boost_thread STATIC ${BOOST_THREAD_SOURCES})
    target_include_directories(boost_thread PUBLIC ${boost_all_SOURCE_DIR})
    target_link_libraries(boost_thread PUBLIC Threads::Threads)
    add_library(Boost::thread ALIAS boost_thread)
endif()


# --- DETECT SYSTEM LIBRARIES ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)

find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

set(BLAZE_SOURCES
    src/app.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/environment.cpp
    src/json.cpp
    src/crypto.cpp
    src/client.cpp
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

if(BLAZE_ENABLE_SANITIZERS)
    message(STATUS "Blaze: Enabling ASan and UBSan...")
    target_compile_options(blaze_framework PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(blaze_framework PUBLIC -fsanitize=address,undefined)
endif()

if(BLAZE_ENABLE_TSAN)
    message(STATUS "Blaze: Enabling ThreadSanitizer (TSan)...")
    target_compile_options(blaze_framework PUBLIC -fsanitize=thread)
    target_link_options(blaze_framework PUBLIC -fsanitize=thread)
endif()

# Legacy aliases
add_library(blaze::core ALIAS blaze_framework)
add_library(blaze::mysql ALIAS blaze_framework)
add_library(blaze::postgres ALIAS blaze_framework)

target_include_directories(blaze_framework BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
    ${PostgreSQL_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
)

# Fix for macOS/Linux MariaDB linking
target_link_directories(blaze_framework PUBLIC ${MARIADB_LIBRARY_DIRS})

target_include_directories(blaze_framework PRIVATE 
    src
    src/drivers/mysql
    src/drivers/postgres
)

target_link_libraries(blaze_framework PUBLIC 
    Boost::system 
    Boost::thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${MARIADB_LIBRARIES}
    PostgreSQL::PostgreSQL
)