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
)
add_library(blaze::core ALIAS blaze_core)

target_include_directories(blaze_core PUBLIC include)
target_include_directories(blaze_core PRIVATE src)
target_link_libraries(blaze_core PUBLIC 
    Boost::system 
    Boost::thread 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

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
