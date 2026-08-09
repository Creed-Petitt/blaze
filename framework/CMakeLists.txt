cmake_minimum_required(VERSION 3.15)
project(blaze_framework LANGUAGES CXX)

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
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
        FetchContent_MakeAvailable(boost_all)
    else()
        FetchContent_Populate(boost_all)
    endif()
endif()

# boost_system
if(NOT TARGET boost_system)
    add_library(boost_system STATIC
        ${boost_all_SOURCE_DIR}/libs/system/src/error_code.cpp
    )
    target_include_directories(boost_system PUBLIC
        $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    set_target_properties(boost_system PROPERTIES EXPORT_NAME boost_system)
    add_library(Boost::system ALIAS boost_system)
endif()

find_package(Threads REQUIRED)

option(BLAZE_ENABLE_MYSQL "Build the MySQL/MariaDB driver" OFF)
option(BLAZE_ENABLE_POSTGRES "Build the PostgreSQL driver" OFF)

if(BLAZE_ENABLE_MYSQL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MARIADB REQUIRED IMPORTED_TARGET libmariadb)
endif()

if(BLAZE_ENABLE_POSTGRES)
    find_package(PostgreSQL REQUIRED)
endif()

# --- TARGET: BLAZE_CORE ---
set(CORE_SOURCES
    src/app.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/environment.cpp
    src/json.cpp
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
set_target_properties(blaze_core PROPERTIES EXPORT_NAME core)
target_compile_features(blaze_core PUBLIC cxx_std_20)

target_include_directories(blaze_core BEFORE PUBLIC 
    $<BUILD_INTERFACE:${boost_all_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(blaze_core PUBLIC 
    boost_system
    Threads::Threads
)

# TARGET: BLAZE_MYSQL
if(BLAZE_ENABLE_MYSQL)
    set(MYSQL_SOURCES
        src/drivers/mysql/mysql_connection.cpp
        src/drivers/mysql/mysql_pool.cpp
        src/drivers/mysql/mysql_result.cpp
    )
    add_library(blaze_mysql STATIC ${MYSQL_SOURCES})
    add_library(blaze::mysql ALIAS blaze_mysql)
    set_target_properties(blaze_mysql PROPERTIES EXPORT_NAME mysql)
    
    target_link_libraries(blaze_mysql PUBLIC blaze::core PkgConfig::MARIADB)
    message(STATUS "Blaze: MySQL driver enabled.")
else()
    message(STATUS "Blaze: MySQL driver disabled.")
endif()

# TARGET: BLAZE_POSTGRES
if(BLAZE_ENABLE_POSTGRES)
    set(PG_SOURCES
        src/drivers/postgres/pg_connection.cpp
        src/drivers/postgres/pg_pool.cpp
        src/drivers/postgres/pg_result.cpp
    )
    add_library(blaze_postgres STATIC ${PG_SOURCES})
    add_library(blaze::postgres ALIAS blaze_postgres)
    set_target_properties(blaze_postgres PROPERTIES EXPORT_NAME postgres)
    
    target_link_libraries(blaze_postgres PUBLIC blaze::core PostgreSQL::PostgreSQL)
    message(STATUS "Blaze: PostgreSQL driver enabled.")
else()
    message(STATUS "Blaze: PostgreSQL driver disabled.")
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

install(
    TARGETS blaze_core boost_system
    EXPORT blazeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BLAZE_ENABLE_MYSQL)
    install(
        TARGETS blaze_mysql
        EXPORT blazeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(BLAZE_ENABLE_POSTGRES)
    install(
        TARGETS blaze_postgres
        EXPORT blazeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${boost_all_SOURCE_DIR}/boost DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
