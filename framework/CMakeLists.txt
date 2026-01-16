include(FetchContent)

# Fetch modern Boost 1.85.0 (Headers Only + System/Thread)
message(STATUS "Blaze: Fetching modern Boost 1.85.0...")
FetchContent_Declare(
    boost_modern
    URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
)

FetchContent_GetProperties(boost_modern)
if(NOT boost_modern_POPULATED)
    FetchContent_Populate(boost_modern)
endif()

# Find System Boost for compiled components (system, thread)
find_package(Boost REQUIRED COMPONENTS system thread)
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
    $<BUILD_INTERFACE:${boost_modern_SOURCE_DIR}>
    include
)
target_include_directories(blaze_core PRIVATE src)

target_link_libraries(blaze_core PUBLIC 
    Boost::system 
    Boost::thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Redis Driver (Self-contained via src.hpp)
add_library(blaze_redis STATIC
    src/drivers/redis/redis_pool.cpp
)
add_library(blaze::redis ALIAS blaze_redis)
target_include_directories(blaze_redis BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_modern_SOURCE_DIR}>
    include
)
target_link_libraries(blaze_redis PUBLIC blaze::core)

# MySQL Driver (Self-contained via src.hpp)
add_library(blaze_mysql STATIC
    src/drivers/mysql/mysql_pool.cpp
)
add_library(blaze::mysql ALIAS blaze_mysql)
target_include_directories(blaze_mysql BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_modern_SOURCE_DIR}>
    include
)
target_link_libraries(blaze_mysql PUBLIC blaze::core)

# Postgres Driver
if(PostgreSQL_FOUND)
    add_library(blaze_postgres STATIC
        src/drivers/postgres/pg_connection.cpp
        src/drivers/postgres/pg_pool.cpp
        src/drivers/postgres/pg_result.cpp
    )
    add_library(blaze::postgres ALIAS blaze_postgres)

    target_include_directories(blaze_postgres BEFORE PUBLIC 
        $<BUILD_INTERFACE:${boost_modern_SOURCE_DIR}>
        include
        ${PostgreSQL_INCLUDE_DIRS}
    )
    target_include_directories(blaze_postgres PRIVATE src/drivers/postgres)
    target_link_libraries(blaze_postgres PUBLIC 
        blaze::core 
        PostgreSQL::PostgreSQL
    )
endif()
