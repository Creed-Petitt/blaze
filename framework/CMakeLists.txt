find_package(Boost REQUIRED)

set(SOURCES
src/app.cpp
src/request.cpp
src/response.cpp
src/router.cpp
src/server.cpp)

add_library(blaze STATIC ${SOURCES})

target_include_directories(blaze PUBLIC include)
target_include_directories(blaze PRIVATE src)
target_link_libraries(blaze PUBLIC Boost::boost)
