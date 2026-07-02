# libFuzzer-based fuzz targets. -fsanitize=fuzzer is a Clang driver feature;
# fail fast on other compilers instead of at link time. Note: on macOS the
# Command Line Tools clang compiles the flag but ships no libFuzzer runtime
# (libclang_rt.fuzzer_osx.a) — use a full Xcode toolchain or LLVM clang.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "GEO_UTILS_CPP_BUILD_FUZZERS=ON requires Clang (libFuzzer); "
        "the current compiler is ${CMAKE_CXX_COMPILER_ID}.")
endif()

add_executable(geo_utils_cpp_fuzz_decode fuzz_decode.cpp)
target_link_libraries(geo_utils_cpp_fuzz_decode PRIVATE geo::utils)
target_compile_options(geo_utils_cpp_fuzz_decode PRIVATE
    -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all -O1 -g)
target_link_options(geo_utils_cpp_fuzz_decode PRIVATE
    -fsanitize=fuzzer,address,undefined)
