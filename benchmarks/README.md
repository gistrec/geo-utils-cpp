# Benchmarks

This directory measures `geo-utils-cpp` against three popular C++ geometry
libraries on two axes:

1. **Speed** — Google Benchmark micro-benchmarks for `distance`, `heading`,
   `contains`, `area`, and `path_length`.
2. **Disk footprint** — stripped binary size of a minimal consumer plus the
   on-disk install size of each library.

The benchmarks are **not built by default** and **not run in CI**. Set
`-DGEO_UTILS_CPP_BUILD_BENCHMARKS=ON` to opt in.

For a high-level summary of results and methodology see
[`docs/benchmarks.md`](../docs/benchmarks.md).

## Competitors

| Library              | Model       | Notes                                             |
| -------------------- | ----------- | ------------------------------------------------- |
| **geo-utils-cpp**    | sphere      | This library — header-only, zero deps             |
| **naive haversine**  | sphere      | ~30 lines of inline math, no library              |
| **S2 Geometry**      | sphere      | Google's mapping library; depends on abseil       |
| **Boost.Geometry**   | sphere\*    | `cs::spherical_equatorial<degree>` strategy       |
| **GeographicLib**    | ellipsoid   | Karney's iterative geodesic — more accurate, slower; **no native point-in-polygon** |

\* Boost.Geometry can also do ellipsoidal; we use the spherical strategy here
to compare apples-to-apples on the algorithm we ourselves implement.

## Installing competitors

The CMake build looks each library up via `find_package` and *skips* the
benchmark binary for any competitor it can't find — so you only need to
install the libraries you want to compare against.

### Homebrew (macOS / Linux)

```sh
brew install s2geometry boost geographiclib
```

### vcpkg

```sh
vcpkg install s2geometry boost-geometry geographiclib
```

(Then point CMake at the vcpkg toolchain file with
`-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`.)

### apt (Debian/Ubuntu)

S2 is not in apt; install from source or vcpkg. The other two:

```sh
sudo apt install libboost-dev libgeographiclib-dev
```

## Building and running speed benchmarks

```sh
cmake -S . -B build-bench \
    -DGEO_UTILS_CPP_BUILD_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target bench_all -j

# Run any one binary:
./build-bench/benchmarks/bench_geo_utils
./build-bench/benchmarks/bench_naive
./build-bench/benchmarks/bench_s2          # if S2 was found
./build-bench/benchmarks/bench_boost       # if Boost was found
./build-bench/benchmarks/bench_geographiclib  # if GeographicLib was found
```

Each binary supports the standard Google Benchmark flags. To produce a single
combined JSON report:

```sh
for b in build-bench/benchmarks/bench_*; do
    "$b" --benchmark_format=json --benchmark_out="$(basename $b).json"
done
```

## Measuring disk footprint

```sh
./benchmarks/size/measure.sh
```

This builds a minimal "distance + point-in-polygon" consumer against every
library it can find, strips the resulting binary, and reports both the
binary size and the on-disk install size of each library. Override compiler
or flags via `CXX=` and `CXXFLAGS=`.

## Methodology notes

- **Same inputs everywhere.** All benchmarks pull data from
  `common/random_data.hpp`, which is seeded deterministically.
- **Native types are pre-built outside the timing loop** for every library.
  The timed work is the library's per-call computation, isolated from
  `lat/lng → native-type` plumbing — so the numbers compare algorithmic cost,
  not data-shape conversion overhead. (`geo-utils-cpp` accepts lat/lng
  directly, so it has nothing to pre-build; that is a separate API-shape
  advantage and not part of these speed numbers.)
- **Polygons are pre-built** outside the timing loop because in real code a
  geofence is a one-time setup cost.
- **Don't compare GeographicLib's distance/heading head-to-head as "speed".**
  It computes a different (more accurate) thing on the WGS84 ellipsoid.
  Treat it as a "trade-off" data point.
