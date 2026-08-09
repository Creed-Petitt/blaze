cmake_minimum_required(VERSION 3.20)
project(blaze VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_subdirectory(framework)

# Only default to building examples if building the framework directly
if(PROJECT_IS_TOP_LEVEL)
    set(DEFAULT_BUILD_EXAMPLES ON)
else()
    set(DEFAULT_BUILD_EXAMPLES OFF)
endif()

option(BLAZE_BUILD_EXAMPLES "Build the educational examples" ${DEFAULT_BUILD_EXAMPLES})
if(BLAZE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(BLAZE_BUILD_TESTS "Build the Blaze test suite" ${PROJECT_IS_TOP_LEVEL})
if(BLAZE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
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

install(
    EXPORT blazeTargets
    NAMESPACE blaze::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blaze
)

set(BLAZE_CONFIG_WITH_MYSQL ${BLAZE_ENABLE_MYSQL})
set(BLAZE_CONFIG_WITH_POSTGRES ${BLAZE_ENABLE_POSTGRES})

configure_package_config_file(
    cmake/blazeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/blazeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blaze
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/blazeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/blazeConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/blazeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blaze
)
