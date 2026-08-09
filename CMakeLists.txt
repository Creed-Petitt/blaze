cmake_minimum_required(VERSION 3.20)
project(blaze VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/BlazeDependencies.cmake)

if(PROJECT_IS_TOP_LEVEL)
    set(BLAZE_DEFAULT_BUILD_EXAMPLES ON)
    set(BLAZE_DEFAULT_INSTALL ON)
else()
    set(BLAZE_DEFAULT_BUILD_EXAMPLES OFF)
    set(BLAZE_DEFAULT_INSTALL OFF)
endif()

option(BLAZE_BUILD_EXAMPLES "Build the educational examples" ${BLAZE_DEFAULT_BUILD_EXAMPLES})
option(BLAZE_BUILD_TESTS "Build the Blaze test suite" ${PROJECT_IS_TOP_LEVEL})
option(BLAZE_BUILD_FUZZERS "Build LibFuzzer targets (requires Clang)" OFF)
option(BLAZE_ENABLE_SANITIZERS "Build Blaze with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BLAZE_ENABLE_TSAN "Build Blaze with ThreadSanitizer" OFF)
option(BLAZE_INSTALL "Generate install rules for Blaze" ${BLAZE_DEFAULT_INSTALL})

blaze_fetch_boost()
find_package(Threads REQUIRED)

if(NOT TARGET blaze_boost_system)
    add_library(blaze_boost_system STATIC
        ${BLAZE_BOOST_SOURCE_DIR}/libs/system/src/error_code.cpp
    )
    target_include_directories(blaze_boost_system PUBLIC
        $<BUILD_INTERFACE:${BLAZE_BOOST_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    set_target_properties(blaze_boost_system PROPERTIES EXPORT_NAME boost_system)
endif()

set(BLAZE_SOURCES
    src/app.cpp
    src/environment.cpp
    src/json.cpp
    src/logger.cpp
    src/middleware.cpp
    src/multipart.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/util/string.cpp
)

add_library(blaze_core STATIC ${BLAZE_SOURCES})
add_library(blaze::core ALIAS blaze_core)
set_target_properties(blaze_core PROPERTIES EXPORT_NAME core)
target_compile_features(blaze_core PUBLIC cxx_std_20)

target_include_directories(blaze_core BEFORE PUBLIC
    $<BUILD_INTERFACE:${BLAZE_BOOST_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(blaze_core PUBLIC
    blaze_boost_system
    Threads::Threads
)

if(BLAZE_ENABLE_SANITIZERS)
    target_compile_options(blaze_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(blaze_core PUBLIC -fsanitize=address,undefined)
endif()

if(BLAZE_ENABLE_TSAN)
    target_compile_options(blaze_core PUBLIC -fsanitize=thread)
    target_link_options(blaze_core PUBLIC -fsanitize=thread)
endif()

if(BLAZE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BLAZE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

if(BLAZE_BUILD_FUZZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Blaze: fuzzing enabled")
        add_subdirectory(tests/fuzzers)
    else()
        message(WARNING "BLAZE_BUILD_FUZZERS is ON, but compiler is not Clang. Fuzz targets will be skipped.")
    endif()
endif()

if(BLAZE_INSTALL)
    install(
        TARGETS blaze_core blaze_boost_system
        EXPORT blazeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(DIRECTORY ${BLAZE_BOOST_SOURCE_DIR}/boost DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(
        EXPORT blazeTargets
        NAMESPACE blaze::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blaze
    )

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
endif()
