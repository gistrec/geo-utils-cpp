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

Practical latitude/longitude geometry for C++17 projects that need GPS math,
not a full geometry framework.

Distance, heading, polygon area, point-in-polygon, and path proximity checks —
header-only, no dependencies, no build step.

The API is inspired by Google Maps geometry utilities and uses the same spherical
Earth approximation model.

## Features

- **Lat/lng-native API** — pass latitude/longitude coordinates directly, no
  framework-specific point types to convert through.
- **Header-only, dependency-free** — about 36 KB across 4 headers; nothing
  to build or link.
- **Spherical math** — distance, heading, offset, interpolation, area.
- **Polygon utilities** — point-in-polygon and path proximity checks.
- **Fast** — matches hand-written haversine on `distance`; especially strong
  on polygon `area` (see [benchmarks](docs/benchmarks.md)).
- **Focused scope** — intentionally small API for GPS, navigation, tracking,
  backend, and GIS workflows.

## Installation

### FetchContent

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

### Conan

```sh
conan install --requires=geo-utils-cpp/1.0.1 --build=missing
```

Conan Center support is pending
[conan-io/conan-center-index#30152](https://github.com/conan-io/conan-center-index/pull/30152)

### build2 / bpkg / cppget

> **Package name note:** in the build2 ecosystem (cppget.org) this library
> follows the build2 `libfoo` convention and is published as **`libgeo-utils`**.
> Everywhere else — GitHub project, CMake, vcpkg, Conan, xrepo — it is named
> **`geo-utils-cpp`**. Both names refer to the same library, same headers, same
> `#include <geo/...>` API.

Add the dependency to your package's `manifest`:

```
depends: libgeo-utils ^1.0.1
```

And in the consuming `buildfile`:

```
import libs = libgeo-utils%lib{geo-utils}

exe{hello}: cxx{hello} $libs
```

To install it into a build2 configuration directly:

```sh
bpkg build libgeo-utils
```

### Manual

Copy the `include/` directory into your project and add it to your include path.

### Using it from CMake

With any of the above methods (vcpkg, xrepo, Conan, FetchContent, or a
system `find_package`), wire it into your build with:

```cmake
find_package(GeoUtilsCpp 1.0.1 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

For more details, see [docs/getting-started.md](docs/getting-started.md).

## Usage

Distance and heading between two points:

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

Polygon area, point-in-polygon, path length, and path proximity:

```cpp
#include <iostream>
#include <vector>

#include <geo/poly.hpp>

int main() {
    // A small box around midtown Manhattan (vertices in CCW order).
    std::vector<geo::LatLng> midtown = {
        {40.74, -74.01}, {40.74, -73.96}, {40.78, -73.96}, {40.78, -74.01},
    };
    geo::LatLng timesSquare{40.7580, -73.9855};

    std::cout << "Times Square inside: "
              << (geo::contains(timesSquare, midtown) ? "true" : "false") << "\n";
    std::cout << "Polygon area: "
              << geo::area(midtown) / 1e6 << " km^2\n";

    // A short polyline along Broadway, and a point near it.
    std::vector<geo::LatLng> route = {
        {40.7580, -73.9855},  // Times Square
        {40.7680, -73.9818},  // Columbus Circle
        {40.7780, -73.9740},  // Lincoln Center
    };
    geo::LatLng nearby{40.7670, -73.9820};

    std::cout << "Route length: "
              << geo::path_length(route) / 1000.0 << " km\n";
    std::cout << "Point within 200 m of route: "
              << (geo::on_path(nearby, route, /*geodesic=*/true, /*tolerance=*/200.0)
                  ? "true" : "false")
              << "\n";
}
```

## Benchmarks

`geo-utils-cpp` is header-only with no runtime dependencies. Throughput on
Apple M1 / clang 17 / `-O2 -DNDEBUG` (higher is better):

| Library              | `distance_between` (M pairs/s) | `area` (poly N=100, M polys/s) |
| -------------------- | -----------------------------: | -----------------------------: |
| **geo-utils-cpp**    |                       **40.5** |                       **67.2** |
| naive haversine      |                           38.3 |                             —  |
| S2 Geometry          |                           82.9 |                           14.0 |
| Boost.Geometry       |                           39.8 |                           36.2 |
| GeographicLib        |                            1.2 |                            2.0 |

Native types are pre-built outside the timed loop, so the table compares
algorithmic cost rather than object-construction overhead. `geo-utils-cpp`
matches hand-written haversine and Boost.Geometry on simple spherical operations,
is especially strong on `area`, while S2 is faster on several operations when
conversion from lat/lng is excluded.

See [docs/benchmarks.md](docs/benchmarks.md) for full methodology, all
operations, and when to use each library.

## When not to use

- If you need high-precision ellipsoidal geodesics or sub-meter accuracy, use
  GeographicLib.
- If polygon containment is your main hot path, especially for larger polygons,
  consider S2 Geometry.
- If you need many geometry types, coordinate systems, or generic geometry
  algorithms, Boost.Geometry may be a better fit.
- If you need spatial indexing, use S2, CGAL, or another dedicated spatial index.

## API Reference

See [docs/api.md](docs/api.md) for the full API reference.

## Support

[Please open an issue on GitHub](https://github.com/gistrec/geo-utils-cpp/issues)

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
