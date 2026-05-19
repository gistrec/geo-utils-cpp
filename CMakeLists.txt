cmake_minimum_required(VERSION 3.14)
project(GeoUtilsCpp VERSION 1.0.1 LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(_geo_utils_cpp_is_top_level ON)
else()
    set(_geo_utils_cpp_is_top_level OFF)
endif()

include(CMakeDependentOption)
option(GEO_UTILS_CPP_BUILD_TESTS      "Build geo-utils-cpp tests"      ${_geo_utils_cpp_is_top_level})
option(GEO_UTILS_CPP_BUILD_EXAMPLES   "Build geo-utils-cpp examples"   ${_geo_utils_cpp_is_top_level})
option(GEO_UTILS_CPP_BUILD_BENCHMARKS "Build geo-utils-cpp benchmarks" OFF)
cmake_dependent_option(GEO_UTILS_CPP_ENABLE_COVERAGE
    "Enable gcov coverage instrumentation (GCC/Clang only)" OFF
    "GEO_UTILS_CPP_BUILD_TESTS" OFF)

# Header-only interface target
add_library(geo_utils_cpp INTERFACE)
add_library(geo::utils ALIAS geo_utils_cpp)

# So the exported imported target is geo::utils (matches the build-tree alias)
set_target_properties(geo_utils_cpp PROPERTIES EXPORT_NAME utils)

target_include_directories(geo_utils_cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(geo_utils_cpp INTERFACE cxx_std_17)

# Tests
if(GEO_UTILS_CPP_BUILD_TESTS)
    include(CTest)
    include(GoogleTest)

    if(TARGET gtest)
        # Someone upstream (the consuming project, or an earlier add_subdirectory)
        # already set up googletest. Reuse those targets — don't try to find or
        # fetch our own. Add the legacy GTest:: aliases only if they're missing.
        if(NOT TARGET GTest::GTest)
            add_library(GTest::GTest ALIAS gtest)
        endif()
        if(NOT TARGET GTest::Main)
            add_library(GTest::Main ALIAS gtest_main)
        endif()
    else()
        find_package(GTest QUIET)
        if(NOT GTest_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/v1.14.0.tar.gz
                DOWNLOAD_EXTRACT_TIMESTAMP ON
            )
            # Prevent overriding the parent project's compiler/linker settings on Windows.
            # Local (non-cache) variables — relies on CMP0077 (NEW since CMake 3.13,
            # default at our cmake_minimum_required of 3.14) so the option() calls
            # inside googletest don't overwrite these.
            set(gtest_force_shared_crt ON)
            # Don't leak GTest into our install prefix.
            set(INSTALL_GTEST OFF)
            FetchContent_MakeAvailable(googletest)
            # FetchContent creates 'gtest'/'gtest_main'; alias to match find_package names
            add_library(GTest::GTest ALIAS gtest)
            add_library(GTest::Main  ALIAS gtest_main)
        endif()
    endif()

    add_executable(geo_utils_cpp_tests tests/tests.cpp)
    target_link_libraries(geo_utils_cpp_tests
        PRIVATE geo::utils GTest::GTest GTest::Main
    )
    gtest_discover_tests(geo_utils_cpp_tests)

    if(GEO_UTILS_CPP_ENABLE_COVERAGE)
        # `--coverage` is split: compile inserts gcov hooks, link pulls in the
        # gcov runtime. The compiler-flag check must include both stages — see
        # CMAKE_REQUIRED_LINK_OPTIONS (CMake 3.14+). Otherwise the test object
        # compiles fine but link fails on missing gcov symbols, giving a false
        # negative on toolchains that actually support the flag (e.g. AppleClang).
        include(CheckCXXCompilerFlag)
        set(CMAKE_REQUIRED_LINK_OPTIONS --coverage)
        check_cxx_compiler_flag(--coverage GEO_UTILS_CPP_HAS_COVERAGE_FLAG)
        unset(CMAKE_REQUIRED_LINK_OPTIONS)

        if(NOT GEO_UTILS_CPP_HAS_COVERAGE_FLAG)
            message(FATAL_ERROR
                "GEO_UTILS_CPP_ENABLE_COVERAGE=ON requested, but the C++ compiler "
                "(${CMAKE_CXX_COMPILER_ID}) does not support --coverage. "
                "Coverage instrumentation requires GCC or Clang.")
        endif()
        target_compile_options(geo_utils_cpp_tests PRIVATE --coverage)
        target_link_options(geo_utils_cpp_tests PRIVATE --coverage)
    endif()
endif()

# Examples
if(GEO_UTILS_CPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Benchmarks (opt-in: pulls Google Benchmark and optionally S2/Boost/GeographicLib)
if(GEO_UTILS_CPP_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
include(GNUInstallDirs)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(TARGETS geo_utils_cpp
    EXPORT GeoUtilsCppTargets
)
install(EXPORT GeoUtilsCppTargets
    FILE GeoUtilsCppTargets.cmake
    NAMESPACE geo::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeoUtilsCpp
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/GeoUtilsCppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GeoUtilsCppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeoUtilsCpp
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GeoUtilsCppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GeoUtilsCppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GeoUtilsCppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeoUtilsCpp
)

# pkg-config
option(GEO_UTILS_CPP_INSTALL_PKGCONFIG "Install a pkg-config (.pc) file" ON)
if(GEO_UTILS_CPP_INSTALL_PKGCONFIG)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/geo-utils-cpp.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/geo-utils-cpp.pc
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/geo-utils-cpp.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
endif()

if(_geo_utils_cpp_is_top_level)
    set(CPACK_PACKAGE_NAME    "geo-utils-cpp")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_GENERATOR       "ZIP")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    include(CPack)
endif()
