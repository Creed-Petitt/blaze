project(blaze_integration_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(blaze_integration_app main.cpp)

target_link_libraries(blaze_integration_app 
    PRIVATE 
    blaze::framework
)
