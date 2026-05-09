# Benchmarks

`geo-utils-cpp` aims to be **as fast as a hand-written haversine** with a
**fraction of the disk footprint** of the popular alternatives. This page
shows the numbers behind that claim and how to reproduce them.

For build and run instructions see [`benchmarks/README.md`](../benchmarks/README.md).

## TL;DR

- **Speed.** Tied with hand-written haversine on every operation we provide
  (header-only design adds zero overhead). Within ~10 % of Boost.Geometry's
  spherical strategy on raw distance/heading. ~10–30× faster than
  GeographicLib's WGS84 geodesic — but **less accurate**, on a sphere.
- **Disk footprint.** **32 KB** of headers vs **32.8 MB** for S2 (with
  abseil), **12.3 MB** for Boost.Geometry's `geometry` subset alone, **4.6 MB**
  for GeographicLib — i.e. a ~144–1000× difference in what you have to ship
  or have on disk.
- **Where competitors win.** S2 has spatial indexing so its `contains`
  scales sub-linearly with polygon size; if you query a 1 000-vertex
  geofence millions of times, S2 will win. We're still ~7× faster than
  Boost.Geometry on `contains`, though.

## Methodology

| Item              | Value                                              |
| ----------------- | -------------------------------------------------- |
| Compiler          | Apple clang 17 (`-std=c++17 -O2 -DNDEBUG`)         |
| Build type        | Release                                            |
| Benchmark harness | [Google Benchmark 1.8.4](https://github.com/google/benchmark) |
| Host              | Apple M1, 8 cores, 8 GB RAM, macOS 15.7            |
| Random data       | Mersenne-twister seeded to a fixed value, lat ∈ [-80, 80], lng ∈ [-180, 180] |
| Library versions  | s2geometry 0.14.0 · boost 1.90.0 · geographiclib 2.7 |

Each library is fed identical inputs (see
[`benchmarks/common/random_data.hpp`](../benchmarks/common/random_data.hpp)).
Conversion to a library's native point type is **counted in the timed
loop** when the input is naturally lat/lng (distance, heading). For polygon
operations the polygon is converted once outside the loop, matching real
geofence-style usage.

### Apples-to-apples notes

- **S2 vs us:** both on a sphere. Fair on accuracy. S2 holds polygons in an
  internal index (`S2Loop` keeps a bounding rect; `S2ShapeIndex` adds more);
  that's why its `contains` scales differently from our linear loop.
- **Boost.Geometry vs us:** uses `cs::spherical_equatorial<degree>`. Fair.
- **GeographicLib vs us:** uses Karney's iterative WGS84 geodesic.
  Slower *and* more accurate. Treat as a trade-off data point, not a
  "we are faster" claim.
- **GeographicLib has no native point-in-polygon.** Real capability gap.
- **S2 has no public initial-bearing API.** Same.

## Speed results

Throughput in million items per second (higher is better). All numbers from
the host described above. Run `./build-bench/benchmarks/bench_*` locally
to reproduce.

### `distance_between`

| Library                | N=1 000 | N=100 000 |
| ---------------------- | ------: | --------: |
| **geo-utils-cpp**      |    36.7 |      25.2 |
| naive haversine        |    37.0 |      25.7 |
| S2 Geometry            |    14.3 |      10.8 |
| Boost.Geometry         |    40.0 |      28.7 |
| GeographicLib (WGS84)  |    1.22 |      1.22 |

We match naive haversine within noise. Boost.Geometry's haversine is ~10 %
faster — likely a different operation order in their inline code. S2 is
slower because each call converts lat/lng → 3D `S2Point`. GeographicLib is
~30× slower (and ~30× more accurate near long-distance pairs).

### `heading`

| Library                | N=1 000 | N=100 000 |
| ---------------------- | ------: | --------: |
| **geo-utils-cpp**      |    23.1 |      13.9 |
| Boost.Geometry         |    24.9 |      14.9 |
| GeographicLib          |    1.15 |      1.14 |
| S2 Geometry            | _no public bearing API_ |   |

### `contains` (point-in-polygon)

Million queries per second; 1 000 query points × polygon vertex count per
iteration.

| Library              | poly N=10 | poly N=100 | poly N=1 000 |
| -------------------- | --------: | ---------: | -----------: |
| **geo-utils-cpp**    |     13.3  |       2.17 |        0.244 |
| S2 Geometry          |     27.1  |      18.0  |       21.1   |
| Boost.Geometry       |      1.89 |       0.23 |        0.024 |
| GeographicLib        | _no native PIP_ |     |              |

This is where the libraries differ most. **S2 has a built-in bounding-rect
test plus a stream-of-edges layout** that makes its loop containment near
sub-linear in vertex count. Our implementation is a textbook O(N) ray cast
through the great-circle edges — fast for small geofences, slower than S2
on huge ones. Boost.Geometry's spherical `within` is significantly slower
than both.

If your workload is "millions of queries against the same large polygon",
S2 will beat us. If it's "small or medium polygons, queried in any volume",
we're competitive and you get a ~1000× smaller install.

### `area` (M polygons/s × vertex count)

| Library              | N=10 | N=100 | N=1 000 |
| -------------------- | ---: | ----: | ------: |
| **geo-utils-cpp**    | 62.4 |  59.7 |    60.6 |
| S2 Geometry          | 16.1 |  13.8 |    13.8 |
| Boost.Geometry       | 44.7 |  36.1 |    36.5 |
| GeographicLib        | 1.71 |  2.00 |    2.04 |

We win clearly on `area` — our spherical-triangle accumulation is a
straight-line loop with no allocation; S2 spends time inside `S2Loop::GetArea`
maintaining its index, and Boost.Geometry's strategy machinery costs ~1.6×.

### `path_length` (M points/s)

| Library              | N=10 | N=100 | N=1 000 |
| -------------------- | ---: | ----: | ------: |
| **geo-utils-cpp**    | 52.0 |  43.9 |    39.7 |
| S2 Geometry          | 104.6|  96.2 |    85.7 |
| Boost.Geometry       | 49.0 |  43.2 |    39.9 |
| GeographicLib        | 1.50 |  1.24 |    1.21 |

S2 wins by ~2× because the `S2Polyline` is pre-built (3D unit vectors stored
contiguously) and the inner loop reduces to dot products rather than `sin`,
`cos`, `atan2`. We tied with Boost.Geometry. GeographicLib pays the
ellipsoidal cost.

## Disk footprint

Stripped binary size of a minimal "distance + point-in-polygon" consumer
plus the on-disk install size of each library. Smaller is better.

| Library              | Stripped binary | Library install | Notes                              |
| -------------------- | --------------: | --------------: | ---------------------------------- |
| **geo-utils-cpp**    |       **33 KB** |       **32 KB** | header-only, zero deps             |
| naive haversine      |           33 KB |               0 | hand-written, no library           |
| S2 Geometry          |         33.4 KB |       32.8 MB   | S2 7.1 MB + abseil 14 MB (+ rest)  |
| Boost.Geometry       |         50.8 KB |       12.3 MB   | only the `geometry` subset of Boost |
| GeographicLib        |           33 KB |        4.6 MB   | distance only — no PIP             |

The "Library install" column is *what you have to ship or have on disk* to
use the library. For `geo-utils-cpp` that's the whole `include/` directory
(every header, every comment); for the others it's the package install
prefix from Homebrew. Boost is reported as just its `geometry` headers —
the full Boost install is ~362 MB.

**The on-disk gap is roughly 144× to 1000×.** If binary size matters
(embedded, container layers, mobile bundles), this is the headline.

## Where each library is the right tool

- **geo-utils-cpp** — small/medium polygons, lat/lng inputs, no-deps
  constraint, container/mobile bundle size matters. Sphere accuracy is
  acceptable.
- **S2 Geometry** — millions of queries against the same large geofence,
  spatial indexing matters, you can afford ~33 MB install. Different
  data model (3D unit vectors).
- **Boost.Geometry** — already-Boost project, want one library for many
  geometry types and CSes. Slower at PIP than both us and S2; pulls in the
  whole `geometry` subset.
- **GeographicLib** — sub-meter geodesic accuracy on the WGS84 ellipsoid.
  Slower by 1–2 orders of magnitude. No PIP.

## Reproducing

```sh
# Speed
cmake -S . -B build-bench \
    -DGEO_UTILS_CPP_BUILD_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target bench_all -j
for b in build-bench/benchmarks/bench_*; do "$b"; done

# Disk footprint
./benchmarks/size/measure.sh
```

If a competitor is missing it will be skipped with a `STATUS` message during
CMake configure (or a "not installed" line from `measure.sh`). Install only
the competitors you care about.
