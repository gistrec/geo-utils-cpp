# Benchmarks for geo-utils-cpp.
#
# Build with:
#     cmake -B build -DGEO_UTILS_CPP_BUILD_BENCHMARKS=ON
#     cmake --build build --target bench_all
#
# Google Benchmark is fetched automatically. S2, Boost.Geometry, and
# GeographicLib are looked up via find_package — missing competitors are
# skipped with a status message instead of causing a hard error.

include(FetchContent)

# --- Google Benchmark -------------------------------------------------------

set(BENCHMARK_ENABLE_TESTING  OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL  OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS    OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    google_benchmark
    URL https://github.com/google/benchmark/archive/v1.8.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(google_benchmark)

# --- Shared infrastructure --------------------------------------------------

add_library(geo_utils_cpp_bench_common INTERFACE)
target_include_directories(geo_utils_cpp_bench_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_compile_features(geo_utils_cpp_bench_common INTERFACE cxx_std_17)

# Aggregator target: depend on every bench that ends up being built.
add_custom_target(bench_all)

function(_geo_utils_cpp_add_bench name source)
    add_executable(bench_${name} ${source})
    target_link_libraries(bench_${name} PRIVATE
        geo::utils
        geo_utils_cpp_bench_common
        benchmark::benchmark_main
        ${ARGN}
    )
    add_dependencies(bench_all bench_${name})
endfunction()

# --- geo-utils-cpp itself + naive haversine baseline ------------------------

_geo_utils_cpp_add_bench(geo_utils speed/bench_geo_utils.cpp)
_geo_utils_cpp_add_bench(naive     speed/bench_naive.cpp)

# --- Optional: S2 Geometry --------------------------------------------------
#
# vcpkg port name: `s2geometry`. Homebrew formula: `s2geometry`. The CMake
# package is conventionally exported as `s2`.
find_package(s2 QUIET)
if(s2_FOUND)
    _geo_utils_cpp_add_bench(s2 speed/bench_s2.cpp s2::s2)
    message(STATUS "geo-utils-cpp benchmarks: S2 found — bench_s2 enabled.")
else()
    message(STATUS "geo-utils-cpp benchmarks: S2 not found — bench_s2 skipped. "
                   "Install via 'vcpkg install s2geometry' or 'brew install s2geometry'.")
endif()

# --- Optional: Boost.Geometry -----------------------------------------------
#
# Boost.Geometry is header-only. CMake 3.30 deprecates the FindBoost module in
# favour of Boost's exported BoostConfig.cmake; we prefer CONFIG and fall back
# to the legacy module so this works across CMake 3.14+.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost 1.70 QUIET CONFIG)
if(NOT Boost_FOUND)
    find_package(Boost 1.70 QUIET)
endif()

if(Boost_FOUND)
    # Different Boost versions expose the header set as Boost::headers (>=1.71),
    # Boost::boost (older), or only via Boost_INCLUDE_DIRS.
    if(TARGET Boost::headers)
        _geo_utils_cpp_add_bench(boost speed/bench_boost.cpp Boost::headers)
    elseif(TARGET Boost::boost)
        _geo_utils_cpp_add_bench(boost speed/bench_boost.cpp Boost::boost)
    else()
        _geo_utils_cpp_add_bench(boost speed/bench_boost.cpp)
        target_include_directories(bench_boost PRIVATE ${Boost_INCLUDE_DIRS})
    endif()
    message(STATUS "geo-utils-cpp benchmarks: Boost.Geometry found — bench_boost enabled.")
else()
    message(STATUS "geo-utils-cpp benchmarks: Boost not found — bench_boost skipped. "
                   "Install via 'vcpkg install boost-geometry' or 'brew install boost'.")
endif()

# --- Optional: GeographicLib ------------------------------------------------
#
# Recent GeographicLib versions export the target `GeographicLib::GeographicLib`.
# Older versions exposed `${GeographicLib_LIBRARIES}` instead — we accept both.
find_package(GeographicLib QUIET)
if(GeographicLib_FOUND)
    if(TARGET GeographicLib::GeographicLib)
        _geo_utils_cpp_add_bench(geographiclib speed/bench_geographiclib.cpp
            GeographicLib::GeographicLib)
    else()
        _geo_utils_cpp_add_bench(geographiclib speed/bench_geographiclib.cpp
            ${GeographicLib_LIBRARIES})
        target_include_directories(bench_geographiclib PRIVATE ${GeographicLib_INCLUDE_DIRS})
    endif()
    message(STATUS "geo-utils-cpp benchmarks: GeographicLib found — bench_geographiclib enabled.")
else()
    message(STATUS "geo-utils-cpp benchmarks: GeographicLib not found — bench_geographiclib skipped. "
                   "Install via 'vcpkg install geographiclib' or 'brew install geographiclib'.")
endif()
