# Changelog

## v1.2.1

Tooling-only release: ships the single-header amalgamation as a release
asset. The library is unchanged — headers are identical to v1.2.0 apart
from the version macros, so package-manager consumers have nothing to
update (registry submissions deliberately stay at 1.2.0).

### Added

- **Single-header `geo.hpp`** attached to this and every future release:
  the whole library as one self-contained file, same API as
  `#include <geo/geo.hpp>`. Generated deterministically by the new
  `tools/amalgamate.py`; validated by building the end-to-end example
  against it and by running the full test suite with the multi-header
  layout shimmed to the amalgamation (76/76).
- Release automation (`release-assets.yml`): pushing a release tag creates
  a draft GitHub release with the notes extracted from `CHANGELOG.md` and
  `geo.hpp` attached; publishing stays a human decision. This shape is
  required by the repository's immutable releases — assets can only be
  attached before publishing, and GitHub Actions release events do not
  fire for draft activity.
- A `single-header-smoke` CI job guards the amalgamation on every PR.

### Unchanged (downstream-compatible)

- Every header except the `<geo/version.hpp>` macros is byte-identical to
  v1.2.0; no API, behavior, or packaging changes.

## v1.2.0

Route tooling (snap-to-route, `point_at_distance`), `LatLngBounds`,
polyline6 encoding, a geodesic `simplify` mode, and a `decode()` hardening
fix found by the new fuzzing setup. The public API is extended; existing
code keeps compiling unchanged.

### Added

- `geo::closest_point_on_segment` / `geo::closest_point_on_path` (with the
  `PathProjection` result struct) — the geodesically correct counterpart of
  `distance_to_segment`: snap a point to a route and get the closest point,
  the segment index, and the distance. Exact at any latitude and across the
  antimeridian; endpoint/vertex results keep the original coordinates
  bit-exactly.
- `geo::point_at_distance` — the point N meters along a path from its first
  vertex; uses the same length formula as `path_length`, clamps to the path.
- `geo::LatLngBounds` and `geo::bounds(path)` (new `<geo/bounds.hpp>`) — a
  lat/lng rectangle with an eastward longitude span, so antimeridian-crossing
  bounds are first-class: `contains`, `extend`, `center`, `intersects`,
  `lng_span`, `is_valid`. `bounds(path)` folds a path into its bounds — a
  cheap prefilter in front of `contains()` / `on_path()`.
- `encode` / `decode` precision parameter (default 5, byte-identical to the
  previous behavior). Pass 6 for the polyline6 grid used by OSRM, Valhalla,
  and Mapbox — the highest precision whose coordinates and point-to-point
  deltas always fit the decoder's 32-bit arithmetic.
- `simplify(poly, tolerance, geodesic = false)` — the new `geodesic = true`
  mode measures vertices against true great-circle segments via
  `closest_point_on_segment` (exact across the antimeridian and at high
  latitudes); the default remains bit-compatible with upstream PolyUtil.
- `LatLng::normalized()` — latitude clamped to [-90, 90], longitude wrapped
  to [-180, 180) (Android Maps SDK conventions); in-range values pass
  through bit-exactly, NaN propagates instead of becoming a fake coordinate.
- `<geo/version.hpp>` — `GEO_UTILS_CPP_VERSION_MAJOR` / `_MINOR` / `_PATCH`,
  a single comparable `GEO_UTILS_CPP_VERSION` usable in `#if`, and
  `GEO_UTILS_CPP_VERSION_STRING`; the test suite pins them to the CMake
  project version.
- `examples/gps_track.cpp` — an end-to-end pipeline on a real 95-point
  track: decode → bounds → geodesic simplify → snap a GPS fix →
  `point_at_distance` → re-encode as polyline6.

### Fixed

- `decode()`: signed-integer-overflow UB on malformed input — a 24-byte
  adversarial string could push the int32 delta accumulator past INT32_MAX.
  Accumulation now wraps in unsigned arithmetic, matching the Java
  original's int semantics bit for bit; well-formed input decodes
  identically. Found by the new libFuzzer harness before it even landed.
- `decode()` divides by the grid factor instead of multiplying by its
  reciprocal — exact on every precision grid (≤ 1 ulp change at the
  default precision).

### Docs

- `docs/api.md` covers the whole new surface; `docs/benchmarks.md` gains
  tables for the route ops (including `point_at_distance` beating
  `S2Polyline::Interpolate` by 1.6–1.9×, and the measured 1.3–2.6× cost of
  geodesic `simplify`).
- `docs/getting-started.md`: fixed a stale "0.1 nanometers" tolerance claim
  (it is ≈ 0.1 micrometers) and a dangling link to a nonexistent `conan/`
  directory; the build-options table now lists the benchmark and fuzzer
  toggles.
- `RELEASING.md` — the full release checklist, registry procedures
  included.

### CI & tests

- New ASan/UBSan job (any sanitizer report fails the build) and a
  60-second libFuzzer smoke test over `geo::decode`
  (`GEO_UTILS_CPP_BUILD_FUZZERS`, Clang only).
- `-Wall -Wextra -Wpedantic -Werror` enforced on the GCC/Clang jobs.

### Unchanged (downstream-compatible)

- Everything shipped in v1.1.x: existing functions, namespaces, headers,
  and the CMake `geo::utils` target keep working without source changes.
  `distance_to_segment` and default-mode `simplify` results are
  bit-identical to v1.1.0.

## v1.1.0

New polyline utilities (encoding, simplification), a `LatLng` validity check,
and several correctness fixes around the antimeridian and the poles. The
public API is extended; existing code keeps compiling unchanged.

### Added

- `geo::encode` / `geo::decode` — the Google Encoded Polyline Algorithm
  Format, in the new `<geo/encoding.hpp>` header (also pulled in by the
  umbrella `<geo/geo.hpp>`).
- `geo::simplify` — Douglas–Peucker polyline/polygon simplification with a
  tolerance in meters.
- `geo::is_closed_polygon` — checks that a path is non-empty and its first
  and last points are equal (antimeridian-aware).
- `geo::LatLng::is_valid()` — `constexpr` check that latitude is in
  [-90, 90] and longitude in [-180, 180], both finite. The constructor
  still stores values exactly as given — no validation, clamping, or
  wrapping; see the new "Validation" section in `docs/api.md`.

### Fixed

- `contains()`: polygon edges spanning exactly 180° of longitude no longer
  flip the point-in-polygon parity arbitrarily (upstream PolyUtil
  convention: such edges never intersect the test ray).
- `offset_origin()`: no longer rejects solutions exactly on the `asin`
  domain boundary (e.g. destinations at a pole).
- `interpolate()`: the linear fallback for nearby points now wraps the
  longitude difference, so points straddling the antimeridian interpolate
  across it instead of the long way around the globe.
- `offset()` / `offset_origin()`: the output longitude is normalized to
  [-180, 180).

### Docs

- `LatLng`: coordinate units (degrees everywhere) and valid ranges are now
  documented in the header itself; out-of-range behavior is specified as
  memory-safe but unspecified.
- `distance_to_segment`: documented the planar-projection approximation and
  its limits (high latitudes, antimeridian-crossing segments).
- `area`: the return-value description no longer claims a sign convention
  (it returns `std::abs(signed_area(...))`).

### Unchanged (downstream-compatible)

- Everything shipped in v1.0.x: existing functions, namespaces, headers,
  and the CMake `geo::utils` target keep working without source changes.

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
