cmake_minimum_required(VERSION 3.20)
project(blaze)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(framework)

option(BLAZE_BUILD_EXAMPLES "Build the educational examples" ON)
if(BLAZE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()

# Fuzzing
option(BLAZE_BUILD_FUZZERS "Build LibFuzzer targets (Requires Clang)" OFF)
if(BLAZE_BUILD_FUZZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Fuzzing Enabled (Clang detected)")
        add_subdirectory(tests/fuzzers)
    else()
        message(WARNING "BLAZE_BUILD_FUZZERS is ON, but compiler is not Clang. Fuzz targets will be skipped.")
    endif()
endif()

option(BLAZE_ENABLE_COVERAGE "Enable code coverage generation (GCC/Clang only)" OFF)
if(BLAZE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling Code Coverage")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Code coverage is only supported on GCC/Clang")
    endif()
endif()
