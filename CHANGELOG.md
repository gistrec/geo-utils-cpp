# Changelog

## v1.0.2

Performance optimizations to the spherical math hot path, new benchmarks, and
expanded packaging: the library can now be consumed via vcpkg, xrepo, and
build2 in addition to CMake / FetchContent. The public C++ API is unchanged.

### Added

- vcpkg packaging: installable as `geo-utils-cpp` from the vcpkg registry.
- xrepo packaging: installable as `geo-utils-cpp` (xmake).
- build2 / bpkg packaging (`manifest`, buildfiles) exposing a binless
  `lib{geo-utils-cpp}` target. In the build2 ecosystem the package carries the
  conventional `lib` prefix and is named `libgeo-utils-cpp`; everywhere else
  (GitHub, CMake, vcpkg, Conan, xrepo) it stays `geo-utils-cpp`.
- Benchmarks (`docs/benchmarks.md`, `benchmarks/`) comparing speed and binary
  size against S2 Geometry, Boost.Geometry, GeographicLib, and a naive
  haversine baseline.
- CI smoke tests for the vcpkg, xrepo, build2, and benchmark builds.

### Performance

- Math hot path: precompute `kDegToRad` / `kRadToDeg` so `deg2rad` / `rad2deg`
  use a single multiply, and make the `arc_hav` clamp branch-free.
- `heading()`: cache the per-call latitude `sin` / `cos` values and fold the two
  `deg2rad` conversions into one. Results are unchanged.

### Docs

- Expanded `docs/getting-started.md` and `docs/api.md`.

### Unchanged (downstream-compatible)

- Public C++ API, the `geo::` and `geo::detail::` namespaces, the public
  headers, and the CMake `geo::utils` target — no source changes needed
  downstream.

## v1.0.1

Renamed package to `geo-utils-cpp` to avoid a name collision with an existing
`geo-utils` component on [repology](https://repology.org). The public C++ API
is unchanged — downstream source code does not need to be updated.

### Changed

- pkg-config name: `geo-utils` → `geo-utils-cpp` (`.pc` file installed as
  `geo-utils-cpp.pc`).
- CPack package name: `geo-utils` → `geo-utils-cpp`.
- CMake package name: `find_package(GeoUtils ...)` → `find_package(GeoUtilsCpp ...)`.
  Config files install to `${CMAKE_INSTALL_LIBDIR}/cmake/GeoUtilsCpp/` as
  `GeoUtilsCppConfig.cmake`, `GeoUtilsCppConfigVersion.cmake`, and
  `GeoUtilsCppTargets.cmake`.
- Build options renamed with `GEO_UTILS_CPP_` prefix:
  `GEO_UTILS_BUILD_TESTS` → `GEO_UTILS_CPP_BUILD_TESTS`,
  `GEO_UTILS_BUILD_EXAMPLES` → `GEO_UTILS_CPP_BUILD_EXAMPLES`,
  `GEO_UTILS_ENABLE_COVERAGE` → `GEO_UTILS_CPP_ENABLE_COVERAGE`,
  `GEO_UTILS_INSTALL_PKGCONFIG` → `GEO_UTILS_CPP_INSTALL_PKGCONFIG`.

### Unchanged (downstream-compatible)

- Imported target `geo::utils`.
- C++ namespace `geo::` and `geo::detail::`.
- Public headers: `<geo/geo.hpp>`, `<geo/latlng.hpp>`, `<geo/spherical.hpp>`,
  `<geo/poly.hpp>`.

## v1.0.0

Initial stable release. Header-only C++17 library for spherical geographic
geometry, no runtime dependencies.

### API

- All public symbols live in the `geo::` namespace; internal helpers in `geo::detail::`.
- `geo::LatLng` — `struct` with `constexpr` constructor, antimeridian-aware
  `operator==`, tunable `approx_equal(other, eps)`, and `operator<<` for
  debugging.
- Spherical functions: `geo::heading`, `geo::offset`, `geo::offset_origin`,
  `geo::interpolate`, `geo::angle_between`, `geo::distance_between`,
  `geo::path_length`, `geo::area`, `geo::signed_area`.
- Polygon / polyline functions: `geo::contains`, `geo::on_edge`,
  `geo::on_path`, `geo::distance_to_segment`.
- Public constants: `geo::kDefaultTolerance`, `geo::LatLng::kDefaultEpsilon`.
- All non-template functions are `noexcept`.

### Headers

- Umbrella header `<geo/geo.hpp>`; per-module includes
  `<geo/latlng.hpp>`, `<geo/spherical.hpp>`, `<geo/poly.hpp>`.
- `#pragma once` throughout. No `M_PI` macro: π is provided as
  `geo::detail::kPi` constant (portable to MSVC without `_USE_MATH_DEFINES`).

### Build & integration

- CMake 3.14+ required.
- Imported target `geo::utils`.
- `find_package(GeoUtils 1.0 REQUIRED)` after `cmake --install`.
- `FetchContent_Declare(GeoUtils ...)` for in-tree consumption.
- `SameMajorVersion` package compatibility.
- Build options `GEO_UTILS_BUILD_TESTS` and `GEO_UTILS_BUILD_EXAMPLES` —
  default ON when geo-utils is the top-level project, OFF when consumed via
  `add_subdirectory` or `FetchContent`.
- `GEO_UTILS_ENABLE_COVERAGE` option for gcov instrumentation (GCC/Clang only).

### Tests & examples

- GoogleTest-based unit tests covering LatLng equality, math helpers,
  spherical functions, and polygon/polyline operations — including edge
  cases at poles, antipodes, and the antimeridian.
- Self-verifying examples (`examples/spherical.cpp`, `examples/poly.cpp`)
  registered as ctest tests.
- Standalone consumer fixture (`tests/consumer/`) for downstream
  `find_package` smoke tests.

### CI

- Multi-OS / multi-compiler matrix: Ubuntu (gcc, clang), macOS (AppleClang),
  Windows (MSVC).
- Separate workflows: build & test (`ci.yml`), install + consumer integration
  on three OSes (`install.yml`), code coverage via Codecov (`coverage.yml`).
- All workflows runnable manually via `workflow_dispatch`.
