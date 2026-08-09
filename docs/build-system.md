# Build System

Blaze is a CMake-first C++20 framework. Applications consume it directly from CMake.

---

## 1. Use Blaze in an application

The smallest application only needs `blaze::core`.

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_api LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    blaze
    GIT_REPOSITORY https://github.com/Creed-Petitt/blaze.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(blaze)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_20)
target_link_libraries(my_api PRIVATE blaze::core)
```

Configure and build it with normal CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/my_api
```

---

## 2. Build this repository

Use direct CMake commands for maintainer workflows:

```bash
cmake -B build/dev -DCMAKE_BUILD_TYPE=Debug -DBLAZE_BUILD_TESTS=ON -DBLAZE_BUILD_EXAMPLES=ON
cmake --build build/dev --parallel
ctest --test-dir build/dev --output-on-failure
```

---

## 3. Install and consume as a package

Blaze installs a CMake package config, so another project can use `find_package`.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBLAZE_BUILD_TESTS=OFF -DBLAZE_BUILD_EXAMPLES=OFF
cmake --build build/release --parallel
cmake --install build/release --prefix /tmp/blaze-install
```

Then consume it:

```cmake
find_package(blaze CONFIG REQUIRED)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_20)
target_link_libraries(my_api PRIVATE blaze::core)
```

Configure the consumer with:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/tmp/blaze-install
cmake --build build --parallel
```

---

## 4. Dependency policy

The default framework build is intentionally small:

- Required: C++20 compiler, CMake, threads, and Boost.Beast/Asio headers.
- Bundled by CMake: Boost 1.85 headers plus the small `boost_system` library.
- Not included: database drivers, TLS/HTTPS, OpenSSL, JWT, or a general-purpose crypto API.

Run TLS at the deployment edge with a reverse proxy, gateway, or load balancer. Blaze focuses on the application framework and plain HTTP backend runtime.
