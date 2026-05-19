# API Reference

All public symbols live in the `geo::` namespace. Helpers under `geo::detail::`
are internal and not part of the supported API.

## Conventions

- **Units.** Coordinates are in degrees (latitude, longitude); distances
  in meters; angles in degrees unless otherwise noted.
- **Earth model.** Spherical, mean radius 6371009 m — the same model
  Google Maps geometry utilities use. See [benchmarks.md](benchmarks.md)
  for the trade-off vs ellipsoidal libraries like GeographicLib.
- **Precision.** Floating-point arithmetic; precision degrades near the
  poles and for antipodal pairs.
- **Thread safety.** All functions are pure (no global mutable state, no
  caching) and safe to call concurrently from multiple threads.
- **Error handling.** Scalar functions (`heading`, `offset`,
  `interpolate`, `distance_between`, `distance_to_segment`) are
  `noexcept`. Out-of-domain inputs are not asserted: `interpolate` with
  `fraction` outside `[0, 1]` extrapolates along the great circle,
  `offset_origin` returns `std::nullopt` when no origin can be computed,
  and pathological inputs may yield NaN. Container-taking functions
  (`area`, `path_length`, `contains`, `on_edge`, `on_path`) are not
  marked `noexcept` because the generic `Path` contract doesn't
  constrain `operator[]` / `size()` to be `noexcept`; they don't throw
  themselves.
- **Include strategy.** Each subsystem has its own header:
  `<geo/latlng.hpp>` (types), `<geo/spherical.hpp>` (distance, heading,
  area), `<geo/poly.hpp>` (point-in-polygon, on-path). The umbrella
  `<geo/geo.hpp>` pulls all three in for convenience.

## LatLng

Represents a geographic coordinate (latitude, longitude) in degrees.

```cpp
#include <geo/latlng.hpp>
```

`geo::LatLng` — a point in geographical coordinates: latitude and longitude.

- Latitude  ranges between `-90` and `90` degrees, inclusive
- Longitude ranges between `-180` and `180` degrees, inclusive. Note: `180` and `-180` are treated as equal.

```cpp
geo::LatLng northPole{90, 0};
geo::LatLng otherPoint = northPole;
```

### Equality

`operator==` performs an **approximate** comparison with tolerance
`LatLng::kDefaultEpsilon` (= `1e-12` degrees, ≈ 0.1 nanometers on Earth).
Longitudes are compared modulo 360°, so `LatLng(0, 180) == LatLng(0, -180)`.

For custom tolerance — e.g. comparing computation results at meter scale —
use `approx_equal`:

```cpp
geo::LatLng a{40.7128, -74.0060};
geo::LatLng b{40.71280001, -74.00599999};

a == b;                    // false (off by ~1e-7°, exceeds default 1e-12)
a.approx_equal(b, 1e-5);   // true (1e-5° ≈ 1 m on equator)
```

---

## Path

A series of connected coordinates in an ordered sequence.

`Path` is a template parameter accepted by `path_length`, `area`, `signed_area`,
`contains`, `on_edge`, and `on_path`. It must be a random-access container of
`geo::LatLng` — specifically, it must support:

- `path.size()` returning a size in elements
- `path[i]` returning a `LatLng` (or something convertible) for `0 ≤ i < size`

This includes `std::vector`, `std::array`, `std::span` (C++20), and
`std::deque`. Forward-only containers like `std::list`, and `std::initializer_list`
(no `operator[]`), are **not** supported — wrap them in a `std::vector` first.

```cpp
std::vector<geo::LatLng> aroundNorthPole = { {89, 0}, {89, 120}, {89, -120} };
std::array<geo::LatLng, 1U> northPole = { {90, 0} };
```

> **Note.** A braced-init-list cannot be passed directly to a `Path`
> template parameter — it has no `operator[]` and no deducible type.
> Wrap it in a container:
>
> ```cpp
> // Won't compile:
> geo::area({{0, 0}, {0, 10}, {10, 0}});
>
> // Wrap in a vector:
> geo::area(std::vector<geo::LatLng>{{0, 0}, {0, 10}, {10, 0}});
> ```

---

## Spherical functions

Spherical geometry utilities for computing angles, distances, and areas.

```cpp
#include <geo/spherical.hpp>
```

### heading

**`geo::heading(const LatLng& from, const LatLng& to)`** — Returns the heading from one LatLng to another. Headings are expressed in degrees clockwise from North within the range `[-180, 180)`.

- `from` — the starting point
- `to` — the destination point

Returns: `double` — heading in degrees clockwise from north, in `[-180, 180)`.

```cpp
geo::LatLng equator{0,  0}; // on the equator at lng=0
geo::LatLng east{0, 90};    // 90° east along the equator

std::cout << geo::heading(equator, east); // +90 (due east)
std::cout << geo::heading(east, equator); // -90 (due west)
```

---

### offset

**`geo::offset(const LatLng& from, double distance, double heading)`** — Returns the LatLng resulting from moving a distance from an origin in the specified heading (degrees clockwise from north).

- `from` — the starting point
- `distance` — the distance to travel, in meters
- `heading` — the heading in degrees clockwise from north

Returns: `LatLng` — the destination point.

```cpp
geo::LatLng front{0, 0};

// Quarter-circumference of Earth: π·R/2 ≈ 10,007.5 km (R = 6371009 m).
constexpr double quarter = 10'007'543.4;

auto up    = geo::offset(front, quarter,   0); // {  90,    0}
auto down  = geo::offset(front, quarter, 180); // { -90,    0}
auto left  = geo::offset(front, quarter, -90); // {   0,  -90}
auto right = geo::offset(front, quarter,  90); // {   0,   90}
```

---

### offset_origin

**`geo::offset_origin(const LatLng& to, double distance, double heading)`** — Returns the origin point that, when travelling `distance` meters at `heading`, arrives at `to`. Returns `std::nullopt` when no origin can be computed — including the degenerate cases at the poles.

- `to` — the destination point
- `distance` — the distance travelled, in meters
- `heading` — the heading in degrees clockwise from north

Returns: `std::optional<LatLng>` — the origin, or `std::nullopt` if unreachable.

```cpp
geo::LatLng start{40.0, -74.0};
constexpr double distance = 5'000'000.0;  // 5,000 km
constexpr double heading  = 60.0;         // east-northeast

// Round-trip: offset_origin undoes offset.
auto destination = geo::offset(start, distance, heading);
auto origin = geo::offset_origin(destination, distance, heading);
assert(origin.has_value() && start.approx_equal(origin.value(), 1e-6));

// nullopt at the pole with quarter-circumference east heading — all
// directions are degenerate there, no origin solution exists.
auto pole = geo::offset_origin(geo::LatLng{90, 0}, 10'007'543.4, 90.0);
assert(!pole.has_value());
```

---

### interpolate

**`geo::interpolate(const LatLng& from, const LatLng& to, double fraction)`** — Returns the LatLng which lies the given fraction of the way between the origin and the destination (spherical linear interpolation).

- `from` — the starting point
- `to` — the destination point
- `fraction` — typically in `[0, 1]`; values outside extrapolate along the same great-circle arc

Returns: `LatLng` — the interpolated point on the great-circle arc from `from` to `to`.

```cpp
geo::LatLng up{90, 0};
geo::LatLng front{0, 0};

// The arc from equator to pole spans 90°, so fraction 1/90 → 1° latitude.
assert(geo::LatLng{1,  0} == geo::interpolate(front, up,  1 / 90.0));
assert(geo::LatLng{89, 0} == geo::interpolate(front, up, 89 / 90.0));
```

---

### angle_between

**`geo::angle_between(const LatLng& from, const LatLng& to)`** — Returns the central angle (great-circle arc) between two points, in radians.

Returned in radians, not degrees, because the value equals the great-circle
arc length on the unit sphere — multiply by Earth's mean radius (6371009 m)
to get meters. This is exactly how `distance_between` is implemented.

Returns: `double` — central angle in radians, in `[0, π]`.

```cpp
geo::LatLng pole{90, 0};
geo::LatLng equator{0, 0};

double angle  = geo::angle_between(pole, equator);  // π/2 ≈ 1.5708
double meters = angle * 6371009.0;                  // ≈ 1.001e+07 (quarter-circumference)
```

---

### distance_between

**`geo::distance_between(const LatLng& from, const LatLng& to)`** — Returns the great-circle distance between two points, in meters.

Returns: `double` — distance in meters, in `[0, π·R]` (i.e. up to half Earth's circumference).

```cpp
geo::LatLng up{90, 0};
geo::LatLng down{-90, 0};

std::cout << geo::distance_between(up, down); // ~20,015 km (π·R, half Earth's circumference)
```

---

### path_length

**`geo::path_length(const Path& path)`** — Returns the total length of the polyline (sum of great-circle distances between consecutive points), in meters.

Returns: `double` — total length in meters; `0` for a path with fewer than 2 points.

```cpp
std::vector<geo::LatLng> path = { {0, 0}, {90, 0}, {0, 90} };
std::cout << geo::path_length(path); // ~20,015 km (π·R, half Earth's circumference)
```

---

### area

**`geo::area(const Path& path)`** — Returns the area of a closed path on Earth, in square meters. The path is implicitly closed (last vertex connects back to the first); equivalent to `std::abs(signed_area(path))`.

Returns: `double` — signed area in square meters; positive for CCW, negative for CW.

```cpp
// Lune bounded by meridians 0 and 90 — one quarter of the Earth's surface.
std::vector<geo::LatLng> path = { {0, 90}, {-90, 0}, {0, 0}, {90, 0}, {0, 90} };
std::cout << geo::area(path); // ~1.275e+14 m² (π·R², one quarter of Earth's surface)
```

---

### signed_area

**`geo::signed_area(const Path& path)`** — Returns the signed area of a closed path on Earth, in square meters.

Sign convention: **counter-clockwise** when viewed from outside the
"inside" face of the polygon yields a **positive** result; clockwise yields a
negative result. "Inside" is the surface that does not contain the South Pole,
so for a small polygon in the northern hemisphere CCW means CCW as seen from
above the North Pole.

Returns: `double`

```cpp
// Triangle in the northern hemisphere, CCW when viewed from above the North Pole
std::vector<geo::LatLng> ccw = { {0, 0}, {0, 10}, {10, 0}, {0, 0} };
std::vector<geo::LatLng> cw  = { {0, 0}, {10, 0}, {0, 10}, {0, 0} };

assert(geo::signed_area(ccw) >  0);
assert(geo::signed_area(cw)  <  0);
assert(geo::signed_area(ccw) == -geo::signed_area(cw));
```

---

## Polygon functions

Utilities for computations involving polygons and polylines.

```cpp
#include <geo/poly.hpp>
```

> **Note on `geodesic` defaults.** `contains` defaults to rhumb-line
> edges (cheaper, fine for polygons well inside one hemisphere);
> `on_edge` and `on_path` default to great-circle edges (more accurate,
> especially near the poles). Pass `geodesic` explicitly when in doubt.

### contains

**`geo::contains(const LatLng& point, const Path& polygon, bool geodesic = false)`** — Returns whether the given point lies inside the specified polygon. The polygon is always considered closed. The South Pole is always outside.

- `geodesic` — `false` (default) for rhumb-line edges, `true` for great-circle edges. See the note at the top of this section.

Returns: `bool` — `true` if `point` is inside the polygon, `false` otherwise.

```cpp
std::vector<geo::LatLng> aroundNorthPole = { {89, 0}, {89, 120}, {89, -120} };

std::cout << geo::contains(geo::LatLng{90, 0},  aroundNorthPole); // true
std::cout << geo::contains(geo::LatLng{-90, 0}, aroundNorthPole); // false
```

---

### on_edge

**`geo::on_edge(const LatLng& point, const Path& polygon, bool geodesic = true, double tolerance = geo::kDefaultTolerance)`** — Returns whether the given point lies on or near a polygon edge (including the closing segment between the last and first vertices), within `tolerance` meters.

- `geodesic` — `true` (default) for great-circle edges, `false` for rhumb-line edges. See the note at the top of this section.
- `tolerance` — maximum distance in meters between `point` and the nearest edge to still count as "on"; defaults to `geo::kDefaultTolerance` (0.1 m).

Returns: `bool` — `true` if `point` is within `tolerance` of any edge.

```cpp
std::vector<geo::LatLng> equator = { {0, 90}, {0, 180} };

std::cout << geo::on_edge(geo::LatLng{0, 90 - 5e-7}, equator); // true
std::cout << geo::on_edge(geo::LatLng{0, 90 - 2e-6}, equator); // false
```

---

### on_path

**`geo::on_path(const LatLng& point, const Path& polyline, bool geodesic = true, double tolerance = geo::kDefaultTolerance)`** — Returns whether the given point lies on or near a polyline, within `tolerance` meters. The closing segment between the first and last points is **not** included — that's the only difference vs `on_edge`.

- `geodesic` — `true` (default) for great-circle edges, `false` for rhumb-line edges. See the note at the top of this section.
- `tolerance` — maximum distance in meters; defaults to `geo::kDefaultTolerance` (0.1 m).

Returns: `bool` — `true` if `point` is within `tolerance` of any segment of the polyline.

```cpp
std::vector<geo::LatLng> polyline = { {0, 0}, {0, 10}, {10, 10}, {10, 0} };

std::cout << geo::on_path(geo::LatLng{0, 5}, polyline); // true  — on first segment
std::cout << geo::on_path(geo::LatLng{5, 0}, polyline); // false — closing edge (10,0)→(0,0) is excluded
```

---

### distance_to_segment

**`geo::distance_to_segment(const LatLng& point, const LatLng& start, const LatLng& end)`** — Returns the distance in meters from `point` to the closest point on the line segment `[start, end]` on the sphere. If the perpendicular foot falls outside the segment, returns the distance to the nearer endpoint.

Returns: `double` — distance in meters, always ≥ 0.

```cpp
geo::LatLng start{28.05359, -82.41632};
geo::LatLng end{28.05310, -82.41634};
geo::LatLng point{28.05342, -82.41594};

// Point lies ~38 m east-northeast of the segment
std::cout << geo::distance_to_segment(point, start, end);
```

---

## Constants

| Symbol | Value | Description |
|---|---|---|
| `geo::kDefaultTolerance` | `0.1` | Default tolerance in meters for `on_edge` / `on_path` |
