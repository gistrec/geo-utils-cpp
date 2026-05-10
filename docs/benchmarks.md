# Benchmarks

`geo-utils-cpp` aims to be **as fast as a hand-written haversine** with a
**fraction of the disk footprint** of the popular alternatives. This page
shows the numbers behind that claim and how to reproduce them.

For build and run instructions see [`benchmarks/README.md`](../benchmarks/README.md).

## TL;DR

- **Speed.** On the algorithm itself (native types pre-built): ties
  Boost.Geometry's spherical strategy on `distance` / `heading` /
  `path_length` (within noise), wins clearly on `area`, and matches a
  hand-written haversine on `distance`. S2 Geometry's 3D-cartesian model
  is **faster** than us on `distance`, `path_length`, and `contains` — but
  it pays a per-call `lat/lng → S2Point` conversion in real-world
  lat/lng-input workloads (not counted in these algorithm-only numbers).
  ~20–30× faster than GeographicLib's WGS84 geodesic — but **less
  accurate**, on a sphere.
- **Disk footprint.** **36 KB** of headers vs **32.8 MB** for S2 (with
  abseil), **12.3 MB** for Boost.Geometry's `geometry` subset alone,
  **4.6 MB** for GeographicLib — a ~130–900× difference in what you have
  to ship or have on disk.
- **Where competitors win.** S2 wins on `contains` (its 3D edge-crossing
  plus a bounding-rectangle prefilter make throughput roughly constant in
  vertex count) and on `distance` / `path_length` once you exclude
  conversion. If those are your hot path *and* you can ship a 33 MB
  dependency, S2 is the right tool. **Where we win.** `area` (allocation-
  free triangle-fan accumulator), and any workload where shipping a
  multi-MB dependency is a non-starter.

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
Each library's **native point/geometry types are pre-built outside the timed
loop**. The timed work is the per-call computation (`bg::distance`,
`S2Loop::Contains`, `pa.Compute()`, etc.) — this isolates algorithmic cost
from `lat/lng → native-type` plumbing. `geo-utils-cpp`'s API takes lat/lng
directly, so it has nothing to pre-build; this is a real API-shape advantage
but is not what the speed numbers below measure.

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
| **geo-utils-cpp**      |     40.5 |  **28.2** |
| naive haversine        |     38.3 |      26.0 |
| S2 Geometry            | **82.9** |  **29.1** |
| Boost.Geometry         |     39.8 |  **28.8** |
| GeographicLib (WGS84)  |     1.25 |      1.24 |

We tie naive haversine and Boost.Geometry's spherical strategy within
noise — zero overhead from being a library. S2 is **2× faster at small N**
because once the input is `S2Point` the per-pair distance reduces to a dot
product / `acos`, cheaper than haversine; the gap closes at N=100 000 where
all three become memory-bandwidth bound. Note: in real-world workloads
where the input *is* lat/lng, S2 also pays a per-call lat/lng→S2Point
conversion that isn't counted here. GeographicLib is ~25× slower (and
substantially more accurate on long-distance pairs).

### `heading`

| Library                | N=1 000  | N=100 000 |
| ---------------------- | -------: | --------: |
| **geo-utils-cpp**      | **24.9** |  **15.5** |
| Boost.Geometry         |     22.5 |  **14.7** |
| GeographicLib          |     1.16 |      1.16 |
| S2 Geometry            | _no public bearing API_ |   |

### `contains` (point-in-polygon)

Million queries per second (1 000 query points per iteration).

| Library              | poly N=10 | poly N=100 | poly N=1 000 |
| -------------------- | --------: | ---------: | -----------: |
| **geo-utils-cpp**    |      16.2 |       2.87 |        0.329 |
| S2 Geometry          |  **26.7** |   **18.0** |     **21.2** |
| Boost.Geometry       |      1.91 |       0.234 |       0.024 |
| GeographicLib        | _no native PIP_ |     |              |

`contains` is where the libraries differ most. **S2's `S2Loop::Contains`
exits early via a bounding-rectangle prefilter** and uses a tightly
inlined 3D edge-crossing routine — so its algorithmic throughput is
roughly *constant* in vertex count. Our implementation is a textbook O(N)
ray cast through edges (rhumb-line by default), so we lose to S2 even at
N=10 once the per-query lat/lng→S2Point conversion is excluded. With
real-world lat/lng inputs (where S2 *would* pay that conversion per
query), the gap closes for tiny polygons. Boost.Geometry's spherical
`within` traces *great-circle* edges — a heavier per-edge predicate that
explains its much lower throughput throughout.

If your workload involves `contains` queries at any volume and you can
ship a 33 MB dependency, S2 wins. If you can't ship that — we still beat
Boost.Geometry by ~10× and ship at ~1/900 the size.

### `area` (M polygons/s × vertex count)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    | **69.9** | **67.2** | **67.7** |
| S2 Geometry          |     16.3 |     14.0 |     13.9 |
| Boost.Geometry       |     45.0 |     36.2 |     36.6 |
| GeographicLib        |     1.75 |     2.04 |     2.07 |

We win clearly on `area` — our spherical-triangle accumulation is a
straight-line loop with no allocation; S2's `S2Loop::GetArea` does more
work per vertex, and Boost.Geometry's strategy machinery costs ~1.8×.

### `path_length` (M points/s)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    |     54.0 |     46.2 |     41.7 |
| S2 Geometry          | **105.1** | **96.7** | **91.6** |
| Boost.Geometry       |     48.7 | **43.5** | **40.2** |
| GeographicLib        |     1.53 |     1.27 |     1.23 |

S2 wins on `path_length` algorithmically (~2×) — once the input is
`S2Point`, segment length is a fast cartesian computation. We tie
Boost.Geometry within noise. GeographicLib pays the ellipsoidal cost.
Note: a lat/lng-input workload would push the S2 column down by the
per-call conversion cost, which is not counted here.

## Disk footprint

Stripped binary size of a minimal "distance + point-in-polygon" consumer
plus the on-disk install size of each library. Smaller is better.

| Library              | Stripped binary | Library install | Notes                              |
| -------------------- | --------------: | --------------: | ---------------------------------- |
| **geo-utils-cpp**    |       **33 KB** |       **36 KB** | header-only, zero deps             |
| naive haversine      |       **33 KB** |               0 | hand-written, no library           |
| S2 Geometry          |     **33.4 KB** |       32.8 MB   | S2 7.1 MB + abseil 14 MB (+ rest)  |
| Boost.Geometry       |         50.8 KB |       12.3 MB   | only the `geometry` subset of Boost |
| GeographicLib        |       **33 KB** |        4.6 MB   | distance only — no PIP             |

The "Library install" column is *what you have to ship or have on disk* to
use the library. For `geo-utils-cpp` that's the whole `include/` directory
(every header, every comment); for the others it's the package install
prefix from Homebrew. Boost is reported as just its `geometry` headers —
the full Boost install is ~362 MB.

**The on-disk gap is roughly 130× to 900×.** If binary size matters
(embedded, container layers, mobile bundles), this is the headline.

## Where each library is the right tool

- **geo-utils-cpp** — lat/lng-native API, no-deps constraint,
  container/mobile bundle size matters, `area` is hot, sphere accuracy is
  acceptable. Best when you'd otherwise be paying conversion cost per call.
- **S2 Geometry** — `contains` / `distance` / `path_length` are the hot
  path, you can afford a ~33 MB install, and you're willing to keep data
  as `S2Point` end-to-end (otherwise the lat/lng→S2Point conversion eats
  the algorithmic win). Spatial indexing (`S2ShapeIndex`) available for
  bigger workloads.
- **Boost.Geometry** — already-Boost project, want one library for many
  geometry types and CSes. Ties us on most operations; loses on `area`
  and on `contains`.
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
