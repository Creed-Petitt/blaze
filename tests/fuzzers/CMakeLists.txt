add_executable(fuzz_url fuzz_url.cpp)
target_link_libraries(fuzz_url PRIVATE blaze_core)
target_compile_options(fuzz_url PRIVATE -fsanitize=fuzzer)
target_link_options(fuzz_url PRIVATE -fsanitize=fuzzer)

add_executable(fuzz_json fuzz_json.cpp)
target_link_libraries(fuzz_json PRIVATE blaze_core)
target_compile_options(fuzz_json PRIVATE -fsanitize=fuzzer)
target_link_options(fuzz_json PRIVATE -fsanitize=fuzzer)
