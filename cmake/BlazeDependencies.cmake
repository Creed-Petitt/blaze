include(FetchContent)

set(BLAZE_BOOST_SOURCE_DIR "" CACHE PATH "Path to a Boost 1.85 source tree. If unset, Blaze downloads Boost with FetchContent.")

function(blaze_fetch_boost)
    if(BLAZE_BOOST_SOURCE_DIR)
        if(NOT EXISTS "${BLAZE_BOOST_SOURCE_DIR}/boost/asio.hpp")
            message(FATAL_ERROR "BLAZE_BOOST_SOURCE_DIR does not look like a Boost source tree: ${BLAZE_BOOST_SOURCE_DIR}")
        endif()
        set(BLAZE_BOOST_SOURCE_DIR "${BLAZE_BOOST_SOURCE_DIR}" PARENT_SCOPE)
        return()
    endif()

    message(STATUS "Blaze: Fetching Boost 1.85.0...")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        FetchContent_Declare(
            boost_all
            URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
            SOURCE_SUBDIR cmake/blaze-no-project
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
    else()
        FetchContent_Declare(
            boost_all
            URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
            SOURCE_SUBDIR cmake/blaze-no-project
        )
    endif()

    FetchContent_MakeAvailable(boost_all)
    FetchContent_GetProperties(boost_all)

    set(BLAZE_BOOST_SOURCE_DIR "${boost_all_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
