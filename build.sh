#!/usr/bin/env bash
set -euo pipefail

# ANSI color codes
BOLD='\033[1m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() {
    echo -e "${CYAN}${BOLD}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}${BOLD}[SUCCESS]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}${BOLD}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}${BOLD}[ERROR]${NC} $*" >&2
}

# Default configuration
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_TESTS="ON"
ENABLE_SANITIZERS="OFF"
ENABLE_TSAN="OFF"
ENABLE_FUZZERS="OFF"
VERBOSE_FLAG=""
JOBS=""
COMMAND=""

usage() {
    echo -e "${BOLD}Usage:${NC} ./build.sh [options] [command]

${BOLD}Commands:${NC}
  build               Configure (if needed) and build Blaze (default)
  configure           Run CMake configuration into \${BUILD_DIR}
  test                Configure, build, and run CTest test suite
  clean               Remove build directory and compile_commands symlink
  rebuild             Clean and build from scratch
  install             Build and install Blaze package

${BOLD}Build Mode Flags:${NC}
  -r, --release        Build with Release mode (optimizations on)
  -d, --debug          Build with Debug mode (default, debug symbols)
  --relwithdebinfo     Build with RelWithDebInfo mode
  --build-type <type>  Specify custom CMAKE_BUILD_TYPE

${BOLD}Configuration Flags:${NC}
  -t, --tests          Enable test suite build (default: ON)
  --no-tests           Disable test suite build
  -s, --sanitizers     Enable ASan + UBSan (-fsanitize=address,undefined)
  --tsan               Enable ThreadSanitizer (-fsanitize=thread)
  -f, --fuzzers        Enable LibFuzzer targets (requires Clang)
  -b, --build-dir <dir> Specify build directory (default: 'build')
  -j, --jobs <N>       Number of concurrent compilation jobs
  -v, --verbose        Enable verbose build output
  -h, --help           Show this help message

${BOLD}Examples:${NC}
  ./build.sh                     # Debug build
  ./build.sh --release           # Release build
  ./build.sh -r test             # Build & run tests in Release mode
  ./build.sh -s test             # Build & test with ASan/UBSan
  ./build.sh clean               # Remove build folder"
}

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --relwithdebinfo)
            BUILD_TYPE="RelWithDebInfo"
            shift
            ;;
        --build-type)
            if [[ -z "${2:-}" ]]; then
                log_error "Missing argument for $1"
                exit 1
            fi
            BUILD_TYPE="$2"
            shift 2
            ;;
        -t|--tests)
            BUILD_TESTS="ON"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        -s|--sanitizers)
            ENABLE_SANITIZERS="ON"
            shift
            ;;
        --tsan)
            ENABLE_TSAN="ON"
            shift
            ;;
        -f|--fuzzers)
            ENABLE_FUZZERS="ON"
            shift
            ;;
        -b|--build-dir)
            if [[ -z "${2:-}" ]]; then
                log_error "Missing argument for $1"
                exit 1
            fi
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            if [[ -z "${2:-}" ]]; then
                log_error "Missing argument for $1"
                exit 1
            fi
            JOBS="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE_FLAG="--verbose"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        configure|build|test|clean|rebuild|install)
            if [[ -n "$COMMAND" ]]; then
                log_error "Multiple commands specified: '$COMMAND' and '$1'"
                exit 1
            fi
            COMMAND="$1"
            shift
            ;;
        *)
            log_error "Unknown option or command: '$1'"
            usage >&2
            exit 2
            ;;
    esac
done

COMMAND="${COMMAND:-build}"

link_compile_commands() {
    if [[ -f "${BUILD_DIR}/compile_commands.json" ]]; then
        ln -sf "${BUILD_DIR}/compile_commands.json" compile_commands.json
        log_info "Linked compile_commands.json -> ${BUILD_DIR}/compile_commands.json"
    fi
}

configure() {
    log_info "Configuring Blaze [Type: ${BUILD_TYPE}, Tests: ${BUILD_TESTS}, Dir: ${BUILD_DIR}]..."

    local cmake_options=(
        -S .
        -B "${BUILD_DIR}"
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        -DBLAZE_BUILD_TESTS="${BUILD_TESTS}"
        -DBLAZE_ENABLE_SANITIZERS="${ENABLE_SANITIZERS}"
        -DBLAZE_ENABLE_TSAN="${ENABLE_TSAN}"
        -DBLAZE_BUILD_FUZZERS="${ENABLE_FUZZERS}"
    )

    if [[ -n "${BLAZE_BOOST_SOURCE_DIR:-}" ]]; then
        cmake_options+=("-DBLAZE_BOOST_SOURCE_DIR=${BLAZE_BOOST_SOURCE_DIR}")
    fi

    cmake "${cmake_options[@]}"
    link_compile_commands
    log_success "Configuration complete."
}

build() {
    if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        configure
    fi

    log_info "Building Blaze targets in ${BUILD_DIR}..."
    local build_opts=(--build "${BUILD_DIR}")
    if [[ -n "$JOBS" ]]; then
        build_opts+=(--parallel "$JOBS")
    else
        build_opts+=(--parallel)
    fi
    if [[ -n "$VERBOSE_FLAG" ]]; then
        build_opts+=("$VERBOSE_FLAG")
    fi

    cmake "${build_opts[@]}"
    link_compile_commands
    log_success "Build complete."
}

test_blaze() {
    if [[ "$BUILD_TESTS" != "ON" ]]; then
        log_warn "Tests were explicitly disabled (--no-tests), re-enabling for 'test' command."
        BUILD_TESTS="ON"
    fi

    configure
    build

    log_info "Running CTest test suite..."
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
    log_success "All tests passed."
}

clean() {
    log_info "Cleaning ${BUILD_DIR} and LSP symlinks..."
    rm -rf "${BUILD_DIR}"
    if [[ -L "compile_commands.json" ]]; then
        rm -f "compile_commands.json"
    fi
    log_success "Clean complete."
}

rebuild() {
    clean
    configure
    build
}

install_blaze() {
    build
    log_info "Installing Blaze package..."
    if [[ -w /usr/local ]]; then
        cmake --install "${BUILD_DIR}"
    else
        sudo cmake --install "${BUILD_DIR}"
    fi
    log_success "Installation complete."
}

case "${COMMAND}" in
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
    rebuild)
        rebuild
        ;;
    install)
        install_blaze
        ;;
esac
