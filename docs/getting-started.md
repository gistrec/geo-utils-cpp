# Getting started

This page walks through installing `geo-utils-cpp` and writing your first
program. For a wider tour of the library, see the [README](../README.md);
for full function-level documentation, see [api.md](api.md); for
performance comparisons against S2, Boost.Geometry, and GeographicLib,
see [benchmarks.md](benchmarks.md).

## Requirements

- C++17 or later (tested on C++17, C++20, and C++23 in CI)
- CMake 3.14 or later
- A standard-conformant compiler. CI covers recent GCC, Clang, AppleClang,
  and MSVC on Linux, macOS, and Windows — see the CI badges in the
  [README](../README.md) for the exact matrix.

The library is header-only with no runtime dependencies — there is nothing
to link against beyond the headers themselves.

## Integration

Pick the option that matches how your project already manages dependencies.
If you don't have a preference, **FetchContent** is the lowest-friction
path: no separate install step, version pinned per project, works offline
once cached.

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

The library is available in the official
[vcpkg registry](https://github.com/microsoft/vcpkg).

Classic mode — install once into the vcpkg shared store:

```sh
vcpkg install geo-utils-cpp
```

Manifest mode — pin the dependency per-project in `vcpkg.json` (preferred
for new projects, since the manifest commits alongside your source):

```json
{
  "dependencies": [
    "geo-utils-cpp"
  ]
}
```

Either mode, then consume it from CMake:

```cmake
find_package(GeoUtilsCpp 1.0.1 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

### xrepo

The library is available in the official
[xmake-repo](https://github.com/xmake-io/xmake-repo) registry as
`geo-utils-cpp`.

Install via the `xrepo` CLI:

```sh
xrepo install geo-utils-cpp
```

Or declare it as a dependency in your `xmake.lua`:

```lua
add_requires("geo-utils-cpp")

target("your_target")
    set_kind("binary")
    add_files("main.cpp")
    add_packages("geo-utils-cpp")
```

A minimal smoke-test consumer (`tests/consumer/`) is included for both CMake
and xmake builds — see [`tests/consumer/xmake.lua`](../tests/consumer/xmake.lua)
for the xmake variant.

### Conan

```sh
conan install --requires=geo-utils-cpp/1.0.1 --build=missing
```

Conan Center support is pending
[conan-io/conan-center-index#30152](https://github.com/conan-io/conan-center-index/pull/30152);
until that lands, install from the local recipe in the
[conan/](https://github.com/gistrec/geo-utils-cpp/tree/master/conan)
directory.

### System install

Build and install the library to a prefix on your machine, then have
downstream projects locate it via `find_package`:

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --install build-cmake --prefix /usr/local
```

```cmake
find_package(GeoUtilsCpp 1.0.1 REQUIRED)
target_link_libraries(your_target PRIVATE geo::utils)
```

If `find_package` can't locate the library, point CMake at the install
prefix via `-DCMAKE_PREFIX_PATH=/usr/local` (or wherever you installed it).

### Manual

Copy the `include/` directory into your project and add it to your
compiler's include path:

```sh
g++ main.cpp -std=c++17 -I/path/to/geo-utils-cpp/include -o main
```

## Usage

Include the umbrella header for everything, or pull in individual modules
for tighter compile times:

```cpp
// Everything at once
#include <geo/geo.hpp>

// Or individual modules
#include <geo/latlng.hpp>     // types
#include <geo/spherical.hpp>  // distance, heading, offset, area
#include <geo/poly.hpp>       // point-in-polygon, on-path
```

### Example: distance and heading between two points

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

### Example: round-trip with custom tolerance

`LatLng::operator==` is an approximate comparison with tolerance `1e-12`
degrees (≈ 0.1 nanometers on Earth) — fine for bit-exact equality, too
strict to compare results of floating-point geometry. For coarser-scale
comparisons, pass an explicit tolerance to `approx_equal`:

```cpp
#include <geo/spherical.hpp>

geo::LatLng start{40.0, -74.0};
geo::LatLng end = geo::offset(start, 100'000.0, 90.0);     // 100 km east
auto recovered  = geo::offset_origin(end, 100'000.0, 90.0);

assert(recovered.has_value());
// 1e-6 degrees ≈ 11 cm on the equator — comfortably above the
// floating-point noise of a 100 km round-trip.
assert(recovered->approx_equal(start, 1e-6));
```

### Example: point-in-polygon

```cpp
#include <iostream>
#include <vector>

#include <geo/poly.hpp>

int main() {
    // A small box around midtown Manhattan.
    std::vector<geo::LatLng> polygon = {
        { 40.7650, -73.9900 },
        { 40.7650, -73.9700 },
        { 40.7450, -73.9700 },
        { 40.7450, -73.9900 },
    };

    geo::LatLng timesSquare = { 40.7580, -73.9855 };
    geo::LatLng centralPark = { 40.7829, -73.9654 };

    std::cout << std::boolalpha;
    std::cout << "Times Square inside: " << geo::contains(timesSquare, polygon) << "\n"; // true
    std::cout << "Central Park inside: " << geo::contains(centralPark, polygon) << "\n"; // false
}
```

More runnable samples live in [`examples/`](../examples/) — each is a
single `.cpp` file you can build and run standalone.

### Editor integration

After CMake configure, `compile_commands.json` is generated in the build
directory. Most language servers (clangd, ccls) pick it up automatically;
point your editor at the build directory if it doesn't.

## Building the library itself

The sections above are for *using* `geo-utils-cpp` as a dependency. If
you're working on the library — running tests, building examples,
measuring coverage — clone the repo and configure it as a top-level
project:

```sh
cmake -S . -B build-cmake
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
```

### Build options

| Option                            | Default                  | Purpose |
| --------------------------------- | ------------------------ | ------- |
| `GEO_UTILS_CPP_BUILD_TESTS`       | `ON` if top-level        | Build unit tests |
| `GEO_UTILS_CPP_BUILD_EXAMPLES`    | `ON` if top-level        | Build the examples in [`examples/`](../examples/) |
| `GEO_UTILS_CPP_ENABLE_COVERAGE`   | `OFF`                    | gcov instrumentation; GCC/Clang only |

Downstream users consuming `geo-utils-cpp` as a dependency don't need to
touch these — tests and examples are off by default when the library is
included via `FetchContent` / `find_package`.

## Troubleshooting

A few issues that catch people on first use:

- **`find_package(GeoUtilsCpp)` fails.** The install prefix isn't on
  CMake's search path. Add `-DCMAKE_PREFIX_PATH=/path/to/prefix` to your
  configure command (or set the env var).

- **`FetchContent` re-downloads on every configure.** Add
  `-DFETCHCONTENT_UPDATES_DISCONNECTED=ON` once the dependency is cached,
  or pin to a specific `GIT_TAG` (which we already do in the snippet
  above).

- **`no matching function for call to 'area({{...}, ...})'`.** A braced
  initializer can't be deduced as a `Path` template parameter — wrap it
  in a `std::vector<geo::LatLng>` first. See the Path notes in
  [api.md](api.md#path).

- **clangd doesn't see the headers.** Make sure CMake has generated
  `compile_commands.json` (it does so automatically once you've
  configured a build directory) and that your editor is pointed at it.

## Next steps

- [API reference](api.md) — every public function, with examples and
  edge-case notes.
- [Benchmarks](benchmarks.md) — how `geo-utils-cpp` compares to S2,
  Boost.Geometry, and GeographicLib, and where each one is the right
  tool.
- [`examples/`](../examples/) — runnable code samples.
- [Open an issue](https://github.com/gistrec/geo-utils-cpp/issues) if
  something here doesn't work or doesn't make sense.
