# geo-utils-cpp

<p align="left">
    <a href="https://github.com/gistrec/geo-utils-cpp/actions/workflows/ci.yml">
        <img src="https://github.com/gistrec/geo-utils-cpp/actions/workflows/ci.yml/badge.svg" alt="CI">
    </a>
    <a href="https://github.com/gistrec/geo-utils-cpp/actions/workflows/vcpkg.yml">
        <img src="https://github.com/gistrec/geo-utils-cpp/actions/workflows/vcpkg.yml/badge.svg" alt="vcpkg">
    </a>
    <a href="https://github.com/gistrec/geo-utils-cpp/actions/workflows/xrepo.yml">
        <img src="https://github.com/gistrec/geo-utils-cpp/actions/workflows/xrepo.yml/badge.svg" alt="xrepo">
    </a>
    <a href="https://app.codacy.com/gh/gistrec/geo-utils-cpp/dashboard">
      <img src="https://img.shields.io/codacy/grade/bcff544711544d5fb7da95b68abf566d" alt="Code quality">
    </a>
    <a href="https://codecov.io/gh/gistrec/geo-utils-cpp">
      <img src="https://codecov.io/gh/gistrec/geo-utils-cpp/graph/badge.svg" alt="Coverage">
    </a>
    <a href="https://github.com/gistrec/geo-utils-cpp/releases">
        <img src="https://img.shields.io/github/v/release/gistrec/geo-utils-cpp" alt="Release">
    </a>
</p>
<p align="left">
    <a href="#">
      <img src="https://img.shields.io/badge/C%2B%2B-17-blue" alt="C++17">
    </a>
    <a href="#">
      <img src="https://img.shields.io/badge/CMake-3.14%2B-064F8C?logo=cmake&logoColor=white" alt="CMake 3.14+">
    </a>
    <a href="#">
      <img src="https://img.shields.io/badge/header--only-yes-brightgreen" alt="Header-only">
    </a>
    <a href="#">
      <img src="https://img.shields.io/badge/platform-Linux%20%C2%B7%20macOS%20%C2%B7%20Windows-brightgreen" alt="Supported platforms">
    </a>
    <a href="https://github.com/gistrec/geo-utils-cpp/blob/master/LICENSE">
        <img src="https://img.shields.io/github/license/gistrec/geo-utils-cpp?color=brightgreen" alt="License">
    </a>
</p>

Header-only C++17 library for geographic (lat/lng) geometry (no dependencies).

Provides utilities for distance, bearing, polygon area, point-in-polygon, and
path proximity checks on Earth coordinates.

API inspired by Google Maps geometry utilities.
Uses spherical Earth approximation (like Google Maps).

## Features

* **`geo::` spherical functions** — distance, bearing, area, interpolation
* **`geo::` polygon functions** — point-in-polygon, path proximity, distance to segments

## Why use this library?

- Lightweight and header-only (no dependencies)
- Simple API for common GPS/lat-lng calculations
- Suitable for backend, GIS, navigation and tracking systems

## When not to use

- If you need high-precision geodesic calculations on an ellipsoid
- If you need advanced spatial indexing (use S2 / CGAL instead)

## Installation

### FetchContent (recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
    GeoUtilsCpp
    GIT_REPOSITORY https://github.com/gistrec/geo-utils-cpp.git
    GIT_TAG        v1.0.1
)
FetchContent_MakeAvailable(GeoUtilsCpp)

target_link_libraries(your_target PRIVATE geo::utils)
```

### vcpkg

```sh
vcpkg install geo-utils-cpp
```

Then in your `CMakeLists.txt`:

```cmake
find_package(GeoUtilsCpp 1.0.1 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

### xrepo

```sh
xrepo install geo-utils-cpp
```

Or declare it as a dependency in your `xmake.lua`:

```lua
add_requires("geo-utils-cpp")

target("your_target")
    add_packages("geo-utils-cpp")
```

### find_package

```cmake
find_package(GeoUtilsCpp 1.0.1 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

### Manual

Copy the `include/` directory into your project and add it to your include path.

For more details see [docs/getting-started.md](docs/getting-started.md).

## Usage

```cpp
#include <iostream>

#include <geo/spherical.hpp>

int main() {
    geo::LatLng newYork = { 40.7128, -74.0060 };
    geo::LatLng london  = { 51.5074,  -0.1278 };

    double distance = geo::distance_between(newYork, london);
    double heading  = geo::heading(newYork, london);

    std::cout << "Distance: " << distance / 1000.0 << " km\n";
    std::cout << "Heading:  " << heading << " deg\n";
}
```

## Benchmarks

`geo-utils-cpp` is a near-zero-overhead wrapper over the math itself, with a
tiny disk footprint thanks to header-only + zero dependencies.

| Library              | `distance_between` (M pairs/s) | `contains` (poly N=10, M qps) | Install size  |
| -------------------- | -----------------------------: | ----------------------------: | ------------: |
| **geo-utils-cpp**    |                       **39.5** |                      **15.9** |     **36 KB** |
| naive haversine      |                           37.4 |                            —  |             0 |
| S2 Geometry          |                           15.1 |                          12.9 |       32.8 MB |
| Boost.Geometry       |                           38.4 |                           1.85|       12.3 MB |
| GeographicLib        |                            1.2 |                 no native PIP |        4.6 MB |

Apple M1 · clang 17 · `-O2 -DNDEBUG`. Tied with Boost.Geometry on
per-pair ops (`distance`, `heading`) within noise; ahead on polygon ops
(`area`, `path_length`, `contains` for tiny polygons). Faster than S2 on
`distance`, `heading`, `area`, `path_length`, and on `contains` against
~10-vertex polygons. **S2 wins `contains` from ~100 vertices onward** via
its bounding-rectangle prefilter. **130–900× smaller install footprint**
than the alternatives. Zero overhead over hand-written haversine.

See [docs/benchmarks.md](docs/benchmarks.md) for the full methodology, all
operations, and a discussion of when to reach for each library.

## API Reference

See [docs/api.md](docs/api.md) for the full API reference.

## Support

[Please open an issue on GitHub](https://github.com/gistrec/geo-utils-cpp/issues)

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
