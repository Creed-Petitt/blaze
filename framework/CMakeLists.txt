include(FetchContent)

# 1. Fetch modern Boost 1.85.0 (The entire engine)
message(STATUS "Blaze: Fetching modern Boost 1.85.0 engine. The first build will take ~3-5 minutes...")
FetchContent_Declare(
    boost_all
    URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
)

# Tell Boost to build its core engine (system/thread) from source
set(BOOST_ENABLE_CMAKE ON CACHE BOOL "")
set(BOOST_INCLUDE_LIBRARIES system thread charconv CACHE STRING "")

FetchContent_MakeAvailable(boost_all)

find_package(OpenSSL REQUIRED)
find_package(PostgreSQL)

# Core Library
add_library(blaze_core STATIC
    src/app.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/crypto.cpp
)
add_library(blaze::core ALIAS blaze_core)

target_include_directories(blaze_core BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
)
target_include_directories(blaze_core PRIVATE src)

# Link to the INTERNAL Boost targets we just built
# These are lowercase names provided by Boost's internal CMake
target_link_libraries(blaze_core PUBLIC 
    boost_system 
    boost_thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Redis Driver (Self-contained via Source Injection)
add_library(blaze_redis STATIC
    src/drivers/redis/redis_pool.cpp
)
add_library(blaze::redis ALIAS blaze_redis)
target_include_directories(blaze_redis BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
)
target_link_libraries(blaze_redis PUBLIC blaze::core)

# MySQL Driver (Self-contained via Source Injection)
add_library(blaze_mysql STATIC
    src/drivers/mysql/mysql_pool.cpp
)
add_library(blaze::mysql ALIAS blaze_mysql)
target_include_directories(blaze_mysql BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    include
)
target_link_libraries(blaze_mysql PUBLIC blaze::core)

# Postgres Driver (Optional)
if(PostgreSQL_FOUND)
    add_library(blaze_postgres STATIC
        src/drivers/postgres/pg_connection.cpp
        src/drivers/postgres/pg_pool.cpp
        src/drivers/postgres/pg_result.cpp
    )
    add_library(blaze::postgres ALIAS blaze_postgres)

    target_include_directories(blaze_postgres BEFORE PUBLIC 
        $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
        include
        ${PostgreSQL_INCLUDE_DIRS}
    )
    target_include_directories(blaze_postgres PRIVATE src/drivers/postgres)
    target_link_libraries(blaze_postgres PUBLIC 
        blaze::core 
        PostgreSQL::PostgreSQL
    )
endif()