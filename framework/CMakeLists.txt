include(FetchContent)

find_package(Boost 1.75 COMPONENTS system thread json QUIET)

if(NOT Boost_FOUND)
    message(STATUS "Blaze: System Boost is too old or missing components. Fetching Boost 1.83.0...")
    
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz
    )
    
    # We only need specific libraries to keep build time sane
    set(BOOST_ENABLE_CMAKE ON)
    set(BOOST_INCLUDE_LIBRARIES system thread json)
    
    FetchContent_MakeAvailable(Boost)
    
    # Alias targets to match find_package style
    if(NOT TARGET Boost::system)
        add_library(Boost::system ALIAS boost_system)
    endif()
    if(NOT TARGET Boost::thread)
        add_library(Boost::thread ALIAS boost_thread)
    endif()
    if(NOT TARGET Boost::json)
        add_library(Boost::json ALIAS boost_json)
    endif()
endif()

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

target_include_directories(blaze_core PUBLIC include)
target_include_directories(blaze_core PRIVATE src)

# Link Boost
if(TARGET Boost::json)
    target_link_libraries(blaze_core PUBLIC Boost::system Boost::thread Boost::json OpenSSL::SSL OpenSSL::Crypto)
else()
    # Fallback for system boost (if it was found but cmake target names differ)
    target_link_libraries(blaze_core PUBLIC ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
    target_include_directories(blaze_core PUBLIC ${Boost_INCLUDE_DIRS})
endif()

# Postgres Driver (Optional)
if(PostgreSQL_FOUND)
    add_library(blaze_postgres STATIC
        src/drivers/postgres/pg_connection.cpp
        src/drivers/postgres/pg_pool.cpp
        src/drivers/postgres/pg_result.cpp
    )
    add_library(blaze::postgres ALIAS blaze_postgres)

    target_include_directories(blaze_postgres PUBLIC 
        include
        ${PostgreSQL_INCLUDE_DIRS}
    )
    target_include_directories(blaze_postgres PRIVATE src/drivers/postgres)
    target_link_libraries(blaze_postgres PUBLIC 
        blaze::core 
        PostgreSQL::PostgreSQL
    )
    message(STATUS "Blaze: PostgreSQL driver enabled")
else()
    message(WARNING "Blaze: PostgreSQL not found, driver will be disabled")
endif()