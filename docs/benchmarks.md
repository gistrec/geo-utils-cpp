# Benchmarks

`geo-utils-cpp` is a small, dependency-free lat/lng geometry library that
stays close to hand-written spherical math while avoiding a full geometry
framework dependency. This page shows the benchmark results, methodology,
and trade-offs against S2 Geometry, Boost.Geometry, and GeographicLib.

For build and run instructions see [`benchmarks/README.md`](../benchmarks/README.md).

## TL;DR

- **Speed.** Matches Boost.Geometry's spherical strategy and hand-written
  haversine on `distance` / `heading`; wins on `area`; loses `contains`
  and `path_length` to S2, which pays a hidden `lat/lng → S2Point`
  conversion in real workloads. ~30× faster than GeographicLib's WGS84
  geodesic — but on a sphere, less accurate.
- **Deployment footprint.** Header-only, zero deps — nothing to add to
  your build's dependency tree, and no `.so`/`.dylib` to ship alongside
  the binary.
- **When each library wins.** S2 wins on `contains` and on several
  algorithm-only speed tests, especially if your data already lives as
  `S2Point` end-to-end. geo-utils-cpp is best-in-class on `area`, and on
  lat/lng-input workloads where adding a geometry framework to your
  build isn't an option. Full per-library guidance is in [Where each library is the right tool](#where-each-library-is-the-right-tool).

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
- **GeographicLib `PolygonArea` is timed differently** for `area` /
  `path_length`. It's an incremental accumulator: `AddPoint` itself does
  the per-vertex geodesic `Inverse()` call, and `Compute()` only
  finalizes the closing edge. We therefore measure the full
  `PolygonArea + N×AddPoint + Compute()` pattern inside the timed loop
  — that's what computing the area of an N-vertex polygon actually
  costs in GeographicLib. The other libraries pre-build native types
  outside the loop; for GeographicLib there is no separable "pre-build"
  step to lift.
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
| GeographicLib          |     1.25 |      1.24 |

We tie naive haversine and Boost.Geometry's spherical strategy within
noise — zero overhead from being a library. The "naive" baseline is a
deliberately textbook haversine (recomputes `* π / 180` per call, no
trig caching); `geo-utils-cpp`'s actual implementation is hand-optimized
(cached `deg2rad` factors, combined `arc_hav` reductions), so "ties
naive" really means "the optimized library version is no slower than a
hand-rolled one-liner — overhead is zero". S2 is **2× faster at small N**
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
| Boost.Geometry         |     22.5 |      14.7 |
| GeographicLib          |     1.16 |      1.16 |
| S2 Geometry            |        — |         — |

S2 has no public initial-bearing API.

### `contains` (point-in-polygon)

Million queries per second (1 000 query points per iteration).

> **Apples-to-apples caveat for this op.** The four libraries do *not*
> run the same algorithm here. `geo-utils-cpp` uses an O(N) ray-cast over
> rhumb-line edges (its `geodesic=false` default — cheaper but less
> accurate near the poles). Boost.Geometry's `bg::within` with the
> spherical CS auto-selects `strategy::within::spherical_winding`, which
> traces *great-circle* edges and is materially more expensive per edge.
> S2 wins partly through algorithm (3D edge-crossing) and partly through
> structure (a bounding-rectangle prefilter on the loop). The Boost gap
> below therefore reflects algorithm choice as much as raw speed; see the
> commentary after the table.

| Library              | poly N=10 | poly N=100 | poly N=1 000 |
| -------------------- | --------: | ---------: | -----------: |
| **geo-utils-cpp**    |      16.2 |       2.87 |        0.329 |
| S2 Geometry          |  **26.7** |   **18.0** |     **21.2** |
| Boost.Geometry       |      1.91 |       0.234 |       0.024 |
| GeographicLib        |         — |          — |            — |

GeographicLib has no native point-in-polygon predicate.

Practical takeaway: if `contains` is the hot path and your data already
lives as `S2Point` end-to-end, S2 is the right tool — throughput is
roughly constant in N because of the prefilter. For lat/lng-input
workloads we beat Boost.Geometry by ~10× while remaining header-only.

### `area` (M polygons/s × vertex count)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    | **69.9** | **67.2** | **67.7** |
| S2 Geometry          |     16.3 |     14.0 |     13.9 |
| Boost.Geometry       |     45.0 |     36.2 |     36.6 |
| GeographicLib        |     1.75 |     2.04 |     2.07 |

N = vertices per polygon.

We win clearly on `area` — our spherical-triangle accumulation is a tight
loop with no allocation; S2's `S2Loop::GetArea` does more work per
vertex, and Boost.Geometry's strategy machinery costs ~1.8×.

### `path_length` (M points/s)

| Library              |     N=10 |    N=100 | N=1 000  |
| -------------------- | -------: | -------: | -------: |
| **geo-utils-cpp**    |     54.0 |     46.2 |     41.7 |
| S2 Geometry          | **105.1** | **96.7** | **91.6** |
| Boost.Geometry       |     48.7 |     43.5 |     40.2 |
| GeographicLib        |     1.53 |     1.27 |     1.23 |

S2 wins on `path_length` algorithmically (~2×) — once the input is
`S2Point`, segment length is a fast cartesian computation. We tie
Boost.Geometry within noise. GeographicLib pays the ellipsoidal cost.
Note: a lat/lng-input workload would push the S2 column down by the
per-call conversion cost, which is not counted here.

## Deployment footprint

A geometry library shows up in two places: at **build time** (what your
CMake has to find, what your container image has to install) and at
**runtime** (what has to sit alongside the binary so it can load the
library at startup). `geo-utils-cpp` is header-only with no deps, so
both are zero beyond the headers themselves.

The table below covers the runtime side — stripped binary size of a
minimal "distance + point-in-polygon" consumer, dynamically linked
against each library. Smaller is better.

| Library              | Stripped binary | Notes                                          |
| -------------------- | --------------: | ---------------------------------------------- |
| **geo-utils-cpp**    |       **33 KB** | header-only — nothing else to ship             |
| naive haversine      |       **33 KB** | hand-written, no library                       |
| S2 Geometry          |         33.4 KB | dynamic linking; `libs2.dylib` required at runtime  |
| Boost.Geometry       |         50.8 KB | header-only — nothing else to ship             |
| GeographicLib        |           33 KB | dynamic linking; `libGeographicLib.dylib` required; distance only — no PIP |

**Bold** marks the entries that ship nothing beyond the binary, not
similarity of the size column itself.

These numbers are for **dynamic linking**: the binary itself stays small
because the library code lives in the shared object that has to be
present at runtime alongside the binary. Static linking shifts the cost
the other way — the binary grows, but only by the symbols the linker
actually keeps (with `-ffunction-sections -Wl,--gc-sections` or LTO,
unused code is pruned). For header-only libraries (`geo-utils-cpp`,
Boost.Geometry) there's nothing to ship beyond the binary in either mode.

The `benchmarks/size/measure.sh` script supports `STATIC=1` if you want
to repeat the comparison for statically linked binaries on your own host
— numbers will depend on which symbols your code actually pulls in.

## Where each library is the right tool

- **geo-utils-cpp** — lat/lng-native API, no-deps constraint, `area` is
  hot, sphere accuracy is acceptable. Best when adding a geometry
  framework to your build (S2 + abseil, Boost.Geometry, GeographicLib)
  isn't an option, or when you'd otherwise be paying a
  lat/lng→native-type conversion on every call.
- **S2 Geometry** — `contains` / `distance` / `path_length` are the hot
  path, and you're willing to keep data as `S2Point` end-to-end
  (otherwise the lat/lng→S2Point conversion eats the algorithmic win).
  Spatial indexing (`S2ShapeIndex`) available for bigger workloads.
- **Boost.Geometry** — already-Boost project, want one library for many
  geometry types and CSes. Ties us on most operations; loses on `area`
  and on `contains`.
- **GeographicLib** — sub-meter geodesic accuracy on the WGS84 ellipsoid.
  Slower by 1–2 orders of magnitude. No PIP.

## Reproducing

Every benchmark is registered with `Repetitions(5)->ReportAggregatesOnly(true)`,
so output shows `_mean` / `_median` / `_stddev` / `_cv` per data point —
that's what the numbers in the tables above are (median rows). Use the
standard Google Benchmark CLI flags (`--benchmark_repetitions=N`,
`--benchmark_min_time=...`, etc.) to override locally.

See [`benchmarks/README.md`](../benchmarks/README.md) for build
prerequisites and target names.

```sh
# Speed
cmake -S . -B build-bench \
    -DGEO_UTILS_CPP_BUILD_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target bench_all -j
for b in build-bench/benchmarks/bench_*; do "$b"; done

# Deployment footprint
./benchmarks/size/measure.sh
```

If a competitor is missing it will be skipped with a `STATUS` message during
CMake configure (or a "not installed" line from `measure.sh`). Install only
the competitors you care about.
