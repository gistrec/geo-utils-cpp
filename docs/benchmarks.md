# Benchmarks

`geo-utils-cpp` aims to be **as fast as a hand-written haversine** with a
**fraction of the disk footprint** of the popular alternatives. This page
shows the numbers behind that claim and how to reproduce them.

For build and run instructions see [`benchmarks/README.md`](../benchmarks/README.md).

## TL;DR

- **Speed.** Ties Boost.Geometry's spherical strategy on `distance` /
  `heading` (within noise) and wins clearly on `area`, `path_length`, and
  `contains`. Wins vs S2 on `distance`, `heading`, `area`, `path_length`,
  and on `contains` against tiny polygons (~10 vertices). ~10–30× faster
  than GeographicLib's WGS84 geodesic — but **less accurate**, on a
  sphere. Matches hand-written haversine within noise: zero header-only
  overhead.
- **Disk footprint.** **32 KB** of headers vs **32.8 MB** for S2 (with
  abseil), **12.3 MB** for Boost.Geometry's `geometry` subset alone,
  **4.6 MB** for GeographicLib — a ~144–1000× difference in what you have
  to ship or have on disk.
- **Where competitors win.** As soon as `contains` polygons reach ~100
  vertices, S2's bounding-rectangle prefilter plus a tightly inlined
  edge-crossing routine take over: S2 is ~4× faster than us at N=100 and
  ~37× faster at N=1 000. For `contains` against geofences of ~100+
  vertices queried at high volume, S2 is the right tool.

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

- **S2 vs us:** both on a sphere. Fair on accuracy. `S2Loop` stores a
  bounding rectangle with the loop and uses it as an early-exit predicate
  inside `Contains`; that, plus a tightly inlined edge-crossing routine,
  is what makes its `contains` scale differently from our linear ray-cast.
  Note: this is *not* spatial indexing — that lives in `S2ShapeIndex`,
  which we do not construct here.
- **Boost.Geometry vs us:** uses `cs::spherical_equatorial<degree>`. Fair.
- **GeographicLib vs us:** uses Karney's iterative WGS84 geodesic.
  Slower *and* more accurate. Treat as a trade-off data point, not a
  "we are faster" claim.
- **GeographicLib has no native point-in-polygon.** Real capability gap.
- **S2 has no public initial-bearing API.** Same.

## Speed results

Throughput in million items per second (higher is better). All numbers from
the host described above. Run `./build-bench/benchmarks/bench_*` locally
to reproduce. **Bold** number = column winner, or co-winners within ~5%
(noise-level tie); bold library name = this library (`geo-utils-cpp`).

### `distance_between`

| Library                | N=1 000  | N=100 000 |
| ---------------------- | -------: | --------: |
| **geo-utils-cpp**      | **39.5** |  **25.7** |
| naive haversine        | **37.4** |  **25.7** |
| S2 Geometry            |     15.1 |      10.6 |
| Boost.Geometry         | **38.4** |  **27.0** |
| GeographicLib (WGS84)  |     1.22 |      1.22 |

Tied with naive haversine and Boost.Geometry within noise. S2 is slower
because each call converts lat/lng → 3D `S2Point`. GeographicLib is ~30×
slower (and substantially more accurate on long-distance pairs).

### `heading`

| Library                | N=1 000  | N=100 000 |
| ---------------------- | -------: | --------: |
| **geo-utils-cpp**      | **25.8** |  **15.1** |
| Boost.Geometry         | **26.3** |  **14.3** |
| GeographicLib          |     1.15 |      1.14 |
| S2 Geometry            | _no public bearing API_ |   |

### `contains` (point-in-polygon)

Million queries per second (1 000 query points per iteration).

| Library              | poly N=10 | poly N=100 | poly N=1 000 |
| -------------------- | --------: | ---------: | -----------: |
| **geo-utils-cpp**    |  **15.9** |       2.82 |        0.321 |
| S2 Geometry          |     12.9  |   **11.3** |     **11.9** |
| Boost.Geometry       |      1.85 |       0.23 |        0.024 |
| GeographicLib        | _no native PIP_ |     |              |

This is where the libraries differ most. **S2's `S2Loop::Contains` exits
early via a bounding-rectangle prefilter** for queries clearly outside the
loop, and uses a tightly inlined edge-crossing routine for the rest — so its
throughput is roughly *constant* in vertex count over our test range. Our
implementation is a textbook O(N) ray cast through edges (rhumb-line by
default), so we beat S2 only on tiny polygons (~10 vertices). From ~100
vertices onward S2's prefilter dominates: ~4× faster than us at N=100,
~37× faster at N=1 000. Boost.Geometry's spherical `within` traces
*great-circle* edges — a heavier per-edge predicate that explains its
much lower throughput throughout.

If your workload involves polygons of ~100+ vertices queried at any volume,
S2 wins `contains`. If it's "tiny polygons, or you can't ship a 33 MB
dependency", we beat both S2 and Boost.Geometry, with a ~1000× smaller
install.

### `area` (M polygons/s × vertex count)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    | **69.5** | **66.6** | **65.5** |
| S2 Geometry          |     15.6 |     13.7 |     13.4 |
| Boost.Geometry       |     43.8 |     35.3 |     36.1 |
| GeographicLib        |     1.71 |     1.99 |     2.04 |

We win clearly on `area` — our spherical-triangle accumulation is a
straight-line loop with no allocation; S2's `S2Loop::GetArea` does more work
per vertex, and Boost.Geometry's strategy machinery costs ~1.8×.

### `path_length` (M points/s)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    | **51.3** | **44.9** | **41.3** |
| S2 Geometry          |     39.3 | **45.7** |     36.6 |
| Boost.Geometry       |     43.8 |     39.2 |     38.8 |
| GeographicLib        |     1.50 |     1.25 |     1.20 |

Comfortably ahead of S2 at N=10 (~30 %) and N=1 000 (~13 %); tied at
N=100 within noise. Ahead of Boost.Geometry at every size. GeographicLib
pays the ellipsoidal cost.

## Disk footprint

Stripped binary size of a minimal "distance + point-in-polygon" consumer
plus the on-disk install size of each library. Smaller is better.

| Library              | Stripped binary | Library install | Notes                              |
| -------------------- | --------------: | --------------: | ---------------------------------- |
| **geo-utils-cpp**    |       **33 KB** |       **32 KB** | header-only, zero deps             |
| naive haversine      |       **33 KB** |               0 | hand-written, no library           |
| S2 Geometry          |     **33.4 KB** |       32.8 MB   | S2 7.1 MB + abseil 14 MB (+ rest)  |
| Boost.Geometry       |         50.8 KB |       12.3 MB   | only the `geometry` subset of Boost |
| GeographicLib        |       **33 KB** |        4.6 MB   | distance only — no PIP             |

The "Library install" column is *what you have to ship or have on disk* to
use the library. For `geo-utils-cpp` that's the whole `include/` directory
(every header, every comment); for the others it's the package install
prefix from Homebrew. Boost is reported as just its `geometry` headers —
the full Boost install is ~362 MB.

**The on-disk gap is roughly 144× to 1000×.** If binary size matters
(embedded, container layers, mobile bundles), this is the headline.

## Where each library is the right tool

- **geo-utils-cpp** — tiny polygons, lat/lng inputs, no-deps constraint,
  container/mobile bundle size matters. Sphere accuracy is acceptable.
- **S2 Geometry** — many `contains` queries against polygons of ~100+
  vertices, bounding-rectangle prefilter / spatial indexing (`S2ShapeIndex`)
  matter, and you can afford a ~33 MB install. Different data model
  (3D unit vectors).
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
