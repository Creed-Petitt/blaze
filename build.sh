#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

usage() {
    cat <<EOF
Usage: ./build.sh [command]

Commands:
  configure   Configure CMake into ${BUILD_DIR}
  build       Configure if needed, then compile Blaze
  test        Configure, compile, and run tests
  clean       Remove ${BUILD_DIR}

Default command: build

Environment:
  BUILD_DIR=/path/to/build     Override the build directory
  BUILD_TYPE=Debug|Release     Override the CMake build type
EOF
}

configure() {
    local cmake_options=(
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DBLAZE_BUILD_TESTS=ON
    )

    if [ -n "${BLAZE_BOOST_SOURCE_DIR:-}" ]; then
        cmake_options+=("-DBLAZE_BOOST_SOURCE_DIR=${BLAZE_BOOST_SOURCE_DIR}")
    fi

    cmake -S . -B "${BUILD_DIR}" "${cmake_options[@]}"
}

build() {
    if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
        configure
    fi

    cmake --build "${BUILD_DIR}" --parallel
}

test_blaze() {
    configure
    cmake --build "${BUILD_DIR}" --parallel
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
}

clean() {
    rm -rf "${BUILD_DIR}"
}

command="${1:-build}"

case "${command}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    test)
        test_blaze
        ;;
    clean)
        clean
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "Unknown command: ${command}" >&2
        usage >&2
        exit 2
        ;;
esac
