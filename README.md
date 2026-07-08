<div align="center">

# geo-utils-cpp

**Google Maps geometry, ported to modern C++ — in a single header.**

_Stop hand-rolling haversine, polyline, and polygon math yourself._

<!-- Row 1 — identity + trust -->
<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/header--only-yes-brightgreen" alt="Header-only">
  <img src="https://img.shields.io/badge/dependencies-zero-brightgreen" alt="Zero dependencies">
  <a href="https://github.com/gistrec/geo-utils-cpp/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-Apache--2.0-blue" alt="License: Apache-2.0"></a>
  <a href="https://github.com/gistrec/geo-utils-cpp/actions/workflows/ci.yml"><img src="https://github.com/gistrec/geo-utils-cpp/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://codecov.io/gh/gistrec/geo-utils-cpp"><img src="https://codecov.io/gh/gistrec/geo-utils-cpp/graph/badge.svg" alt="Coverage"></a>
  <a href="https://github.com/gistrec/geo-utils-cpp/releases"><img src="https://img.shields.io/github/v/release/gistrec/geo-utils-cpp" alt="Latest release"></a>
</p>

<!-- Row 2 — get it (package registries) -->
<p align="center">
  <a href="https://vcpkg.io/en/package/geo-utils-cpp"><img src="https://img.shields.io/vcpkg/v/geo-utils-cpp?logo=cmake&logoColor=white" alt="vcpkg version"></a>
  <a href="https://github.com/xmake-io/xmake-repo/tree/master/packages/g/geo-utils-cpp"><img src="https://img.shields.io/badge/xrepo-geo--utils--cpp-2C3E50" alt="xrepo package"></a>
  <a href="https://cppget.org/libgeo-utils-cpp"><img src="https://img.shields.io/badge/build2-libgeo--utils--cpp-2C3E50" alt="build2 / cppget package"></a>
  <a href="https://github.com/conan-io/conan-center-index/pull/30152"><img src="https://img.shields.io/badge/Conan--Center-pending-9E9E9E?logo=conan&logoColor=white" alt="Conan Center (pending)"></a>
  <a href="https://github.com/mesonbuild/wrapdb/pull/2820"><img src="https://img.shields.io/badge/Meson%20WrapDB-pending-9E9E9E?logo=meson&logoColor=white" alt="Meson WrapDB (pending)"></a>
</p>

<!-- ASSET: docs/assets/hero-pipeline.gif (hero, width=1000) -->

**▶ [Try it live in Compiler Explorer](https://godbolt.org/z/hx6W3WMsa)** — no install needed

</div>

<p align="center"><b>~1.9× Boost (~5× S2) on <code>area</code> · 1.6–1.9× faster than S2 on <code>point_at_distance</code></b><br>
<sub><a href="#which-library-should-i-pick">→ see the numbers</a> · Apple M1 / clang 17 / <code>-O2 -DNDEBUG</code></sub></p>

---

**Contents:** [Why](#why-geo-utils-cpp) · [API at a glance](#api-at-a-glance) · [Quick start](#quick-start) · [Which library should I pick?](#which-library-should-i-pick) · [Installation](#installation) · [Requirements & compatibility](#requirements--compatibility) · [API reference](#api-reference)

## Why geo-utils-cpp

- **Drop-in.** About 50 KB across 8 headers — no dependencies, no build step. Copy `include/`
  (or the single amalgamated `geo.hpp`) and `#include <geo/...>`.
- **Lat/lng-native.** Pass latitude/longitude in degrees; there are no framework-specific
  point types to convert through.
- **Everything for GPS work.** Distance, heading, offset, interpolation, polygon area,
  point-in-polygon, path proximity, snap-to-route, Douglas–Peucker simplification, and
  antimeridian-aware `LatLngBounds` viewport math.
- **Speaks the map stack.** `encode`/`decode` for the Google Encoded Polyline format, including
  the polyline6 grid used by OSRM, Valhalla, and Mapbox.
- **Fast where it counts.** Matches hand-written haversine on `distance` and is especially strong
  on polygon `area` — see [the benchmarks](#which-library-should-i-pick).
- **Focused scope.** A small, stable API aimed at GPS, navigation, tracking, backend, and GIS
  workflows — not a full geometry framework.

The API is inspired by Google Maps geometry utilities and uses the same spherical Earth model.

## API at a glance

<!-- ASSET (future gallery, brief #2/#3): docs/assets/gallery/great-circle.png, snap-to-route.png, point-in-polygon.png, encode-decode.png, simplify.gif -->

| Header | Key functions | What it does |
| --- | --- | --- |
| `<geo/spherical.hpp>` | `distance_between`, `heading`, `offset`, `interpolate`, `path_length`, `point_at_distance`, `area` | Great-circle distance and bearing, move-by-distance, slerp, route length, the point _N_ meters along a route, and polygon area. |
| `<geo/poly.hpp>` | `contains`, `on_path`, `closest_point_on_path`, `simplify` | Point-in-polygon, path-proximity checks, snap-to-route projection, and Douglas–Peucker simplification. |
| `<geo/encoding.hpp>` | `encode`, `decode` | Google Encoded Polyline — precision 5, plus polyline6 for OSRM / Valhalla / Mapbox. |
| `<geo/bounds.hpp>` | `LatLngBounds`, `bounds` | Antimeridian-aware viewport rectangle: `contains`, `center`, `extend`, `intersects`. |
| `<geo/latlng.hpp>` | `LatLng`, `is_valid`, `normalized` | The degrees-in / degrees-out coordinate type used everywhere. |
| `<geo/geo.hpp>` | — | Umbrella header that pulls in all of the above. |

Full signatures and semantics: [docs/api.md](docs/api.md).

## Quick start

<!-- ASSET: docs/assets/demo.gif (quick start, terminal demo) -->

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

_Run both online, no install needed — [open them in Compiler Explorer](https://godbolt.org/z/hx6W3WMsa)._

## Which library should I pick?

`geo-utils-cpp` is header-only with no runtime dependencies. Throughput on
Apple M1 / clang 17 / `-O2 -DNDEBUG` (higher is better):

<!-- ASSET: docs/assets/benchmarks.svg (above the benchmark table) -->

| Library              | `distance_between` (M pairs/s) | `area` (poly N=100, M polys/s) | `point_at_distance` (route N=100, M queries/s) |
| -------------------- | -----------------------------: | -----------------------------: | ---------------------------------------------: |
| **geo-utils-cpp**    |                       **40.5** |                       **67.2** |                                        **0.79** |
| naive haversine      |                           38.3 |                             —  |                                              —  |
| S2 Geometry          |                           82.9 |                           14.0 |                                            0.50 |
| Boost.Geometry       |                           39.8 |                           36.2 |                                              —  |
| GeographicLib        |                            1.2 |                            2.0 |                                              —  |

### Feature matrix

| Capability | geo-utils-cpp | S2 Geometry | Boost.Geometry | GeographicLib |
| --- | :--: | :--: | :--: | :--: |
| Header-only, zero dependencies | ✅ | — | — | — |
| Lat/lng-native API (degrees in / out) | ✅ | — | — | — |
| Distance, heading, polygon area | ✅ | ✅ | ✅ | ✅ ¹ |
| Point-in-polygon & snap-to-route | ✅ | ✅ | ✅ | — |
| Google polyline encode/decode (5 & 6) | ✅ | — | — | — |
| Sub-meter WGS84 ellipsoidal geodesics | — | — | — | ✅ |
| Spatial indexing (S2 cells / R-tree) | — | ✅ | ✅ | — |
| Broad geometry types & coordinate systems | — | — | ✅ | — |

<sup>¹ GeographicLib computes on the WGS84 ellipsoid (sub-meter accuracy) rather than a sphere.</sup>

**Reach for something else when:**

- You need high-precision ellipsoidal geodesics or sub-meter accuracy — use **GeographicLib**.
- Polygon containment is your main hot path, especially for larger polygons — consider **S2 Geometry**.
- You need many geometry types, coordinate systems, or generic geometry algorithms — **Boost.Geometry**
  may be a better fit.
- You need spatial indexing — use **S2**, **CGAL**, or another dedicated spatial index.

**Accuracy.** `geo-utils-cpp` uses a spherical Earth model (mean radius 6371009 m — the same model
Google Maps geometry utilities use), not the WGS84 ellipsoid. That is the right trade-off for GPS,
navigation, and tracking, but not for sub-meter surveying; precision also degrades near the poles and
for antipodal pairs. See [docs/api.md](docs/api.md) and [docs/benchmarks.md](docs/benchmarks.md) for
the details.

<details>
<summary><b>Benchmark methodology</b></summary>

Native types are pre-built outside the timed loop, so the table compares algorithmic cost rather than
object-construction overhead. `geo-utils-cpp` matches hand-written haversine and Boost.Geometry on
simple spherical operations, is especially strong on `area`, and beats `S2Polyline::Interpolate` on
`point_at_distance` by ≈1.6× at the N=100 route shown above (rising to ~1.9× for longer routes). S2 is
faster on several other operations — notably `distance_between` — when the `lat/lng → S2Point`
conversion is excluded.

See [docs/benchmarks.md](docs/benchmarks.md) for full methodology, all operations, and when to use
each library.

</details>

## Installation

The fastest path is CMake **FetchContent** — no system install required:

```cmake
include(FetchContent)

FetchContent_Declare(
    GeoUtilsCpp
    GIT_REPOSITORY https://github.com/gistrec/geo-utils-cpp.git
    GIT_TAG        v1.2.2
)
FetchContent_MakeAvailable(GeoUtilsCpp)

target_link_libraries(your_target PRIVATE geo::utils)
```

<details>
<summary><b>Other package managers</b> — vcpkg · xrepo · Conan · build2 · single-header · CMake wiring</summary>

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
conan install --requires=geo-utils-cpp/1.2.2 --build=missing
```

Conan Center support is pending
[conan-io/conan-center-index#30152](https://github.com/conan-io/conan-center-index/pull/30152)

### build2 / bpkg

> **Package name note:** in the build2 ecosystem this library carries the
> conventional `lib` prefix and is named **`libgeo-utils-cpp`** — i.e. the same
> `geo-utils-cpp` with `lib` in front.
> Everywhere else (GitHub, CMake, vcpkg, Conan, xrepo) it stays
> **`geo-utils-cpp`**. Same library, same headers, same `#include <geo/...>` API.

Add the dependency to your package's `manifest`:

```
depends: libgeo-utils-cpp ^1.2.2
```

And in the consuming `buildfile`:

```
import libs = libgeo-utils-cpp%lib{geo-utils-cpp}

exe{hello}: cxx{hello} $libs
```

The package is published on [cppget.org](https://cppget.org/libgeo-utils-cpp)
in the `testing` section. Add that repository so the dependency above resolves —
put it in your project's `repositories.manifest`:

```
role: prerequisite
location: https://pkg.cppget.org/1/testing
```

### Manual / single-header

Copy the `include/` directory into your project and add it to your include
path. Or grab the single-header `geo.hpp` attached to
[releases](https://github.com/gistrec/geo-utils-cpp/releases) after v1.2.0
— one file, same API as `#include <geo/geo.hpp>`.

### Using it from CMake

With any of the above methods (vcpkg, xrepo, Conan, FetchContent, or a
system `find_package`), wire it into your build with:

```cmake
find_package(GeoUtilsCpp 1.2.2 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

For more details, see [docs/getting-started.md](docs/getting-started.md).

</details>

## Requirements & compatibility

Any toolchain with complete C++17 support. Continuous integration builds and tests on:

| Platform | Compiler (CI) | Notes |
| --- | --- | --- |
| Linux | GCC, Clang | built at `-Wall -Wextra -Wpedantic -Werror` |
| macOS | AppleClang | built at `-Wall -Wextra -Wpedantic -Werror` |
| Windows | MSVC | default warning level |

The full test suite also runs under AddressSanitizer + UndefinedBehaviorSanitizer, with a libFuzzer
smoke test over `geo::decode`. The library needs only C++17; the optional CMake integration requires
CMake ≥ 3.14. Releases follow [semantic versioning](https://semver.org), and `<geo/version.hpp>`
exposes `GEO_UTILS_CPP_VERSION` for `#if` compatibility checks.

## API reference

- 📖 **Full API reference** — [docs/api.md](docs/api.md)
- 🚀 **New here?** — [docs/getting-started.md](docs/getting-started.md)
- ⚡ **Performance details** — [docs/benchmarks.md](docs/benchmarks.md)

## Contributing & support

Questions, bug reports, and pull requests are all welcome —
[open an issue](https://github.com/gistrec/geo-utils-cpp/issues) or send a PR.

## Credits

Ported from and API-compatible with the geometry utilities in Google Maps'
[android-maps-utils](https://github.com/googlemaps/android-maps-utils) (Apache-2.0).

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

---

<div align="center">
<sub>If this saved you an afternoon, consider <a href="https://github.com/gistrec/geo-utils-cpp">starring the repo</a> ⭐</sub>
</div>
