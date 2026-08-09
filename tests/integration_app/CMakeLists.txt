project(blaze_integration_app LANGUAGES CXX)

add_executable(blaze_integration_app main.cpp)
target_compile_features(blaze_integration_app PRIVATE cxx_std_20)

target_link_libraries(blaze_integration_app
    PRIVATE
    blaze::core
    blaze::mysql
    blaze::postgres
)
