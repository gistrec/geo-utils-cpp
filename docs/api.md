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
  `interpolate`, `distance_between`, `distance_to_segment`,
  `closest_point_on_segment`) are `noexcept`. Out-of-domain inputs are not asserted: `interpolate` with
  `fraction` outside `[0, 1]` extrapolates along the great circle,
  `offset_origin` returns `std::nullopt` when no origin can be computed,
  and pathological inputs may yield NaN. Coordinates are never validated —
  an out-of-range `LatLng` gives unspecified (but memory-safe) results;
  see [LatLng § Validation](#validation). Container-taking functions
  (`area`, `path_length`, `point_at_distance`, `contains`, `on_edge`,
  `on_path`, `closest_point_on_path`) are not
  marked `noexcept` because the generic `Path` contract doesn't
  constrain `operator[]` / `size()` to be `noexcept`; they don't throw
  themselves. `encode`, `decode`, and `simplify` return owning
  containers and can throw `std::bad_alloc` on allocation failure.
- **Include strategy.** Each subsystem has its own header:
  `<geo/latlng.hpp>` (types), `<geo/spherical.hpp>` (distance, heading,
  area), `<geo/poly.hpp>` (point-in-polygon, on-path), `<geo/encoding.hpp>`
  (encoded polylines). The umbrella `<geo/geo.hpp>` pulls all four in for
  convenience.

## LatLng

Represents a geographic coordinate (latitude, longitude) in degrees.

```cpp
#include <geo/latlng.hpp>
```

`geo::LatLng` — a point in geographical coordinates: latitude and longitude.
All coordinates across the entire API are expressed in **degrees**; radians
are never used in public signatures.

- Latitude  ranges between `-90` and `90` degrees, inclusive
- Longitude ranges between `-180` and `180` degrees, inclusive. Note: `180` and `-180` are treated as equal.

```cpp
geo::LatLng northPole{90, 0};
geo::LatLng otherPoint = northPole;
```

### Validation

The constructor stores the values exactly as given — it performs **no
validation, clamping, or wrapping**. Use `is_valid()` to check coordinates
that come from an untrusted source:

```cpp
geo::LatLng{90, 180}.is_valid();    // true  — boundaries are inclusive
geo::LatLng{180, 180}.is_valid();   // false — latitude out of [-90, 90]
geo::LatLng{0, 360}.is_valid();     // false — longitude out of [-180, 180]
geo::LatLng{NAN, 0}.is_valid();     // false — non-finite
```

Passing an out-of-range `LatLng` to `geo::` functions is memory-safe and
does not throw, but the results are **unspecified**: the values feed straight
into spherical trigonometry. For example, `LatLng(180, 180)` — latitude
"wrapped over the pole" — behaves as the direction `(0, 0)` in distance
computations (`distance_between(LatLng(180, 180), LatLng(0, 0)) == 0`), yet
still compares unequal to `LatLng(0, 0)`, and rhumb-line functions may
produce NaN from it. Validate or normalize coordinates at the boundary of
your system instead of relying on any of this behavior.

To normalize, use `normalized()`: it clamps the latitude to `[-90, 90]` and
wraps the longitude to `[-180, 180)` — the same conventions as the Android
Maps SDK constructor. In-range values pass through bit-exactly (longitude
`180` becomes `-180`); NaN components propagate, so garbage stays invalid
rather than turning into a fake coordinate.

```cpp
geo::LatLng{40.7, -74.0}.normalized();  // unchanged
geo::LatLng{0, 185}.normalized();       // {0, -175}
geo::LatLng{91, 540}.normalized();      // {90, -180}
geo::LatLng{NAN, 0}.normalized();       // {NaN, 0} — still !is_valid()
```

### Equality

`operator==` performs an **approximate** comparison with tolerance
`LatLng::kDefaultEpsilon` (= `1e-12` degrees, ≈ 0.1 micrometers on Earth).
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

`Path` is a template parameter accepted by `path_length`, `point_at_distance`,
`area`, `signed_area`, `contains`, `on_edge`, `on_path`,
`closest_point_on_path`, `is_closed_polygon`, `simplify`, and `encode`.
It must be a random-access container of `geo::LatLng` — specifically, it must
support:

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

Returns: `LatLng` — the destination point. The longitude of the result is normalized to `[-180, 180)`.

```cpp
geo::LatLng front{0, 0};

// Quarter-circumference of Earth: π·R/2 ≈ 10,007.5 km (R = 6371009 m).
constexpr double quarter = 10'007'543.4;

auto up    = geo::offset(front, quarter,   0); // {  90,    0}
auto down  = geo::offset(front, quarter, 180); // { -90,    0}
auto left  = geo::offset(front, quarter, -90); // {   0,  -90}
auto right = geo::offset(front, quarter,  90); // {   0,   90}

// Crossing the antimeridian: the longitude comes back normalized.
auto wrapped = geo::offset({0, 170}, 3'000'000, 90); // {0, -163.02}, not {0, 196.98}
```

---

### offset_origin

**`geo::offset_origin(const LatLng& to, double distance, double heading)`** — Returns the origin point that, when travelling `distance` meters at `heading`, arrives at `to`. Returns `std::nullopt` when no origin can be computed — including the degenerate cases at the poles.

- `to` — the destination point
- `distance` — the distance travelled, in meters
- `heading` — the heading in degrees clockwise from north

Returns: `std::optional<LatLng>` — the origin, or `std::nullopt` if unreachable. The longitude of the result is normalized to `[-180, 180)`.

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

> **Note.** For (nearly) antipodal endpoints the great circle is not unique,
> and the result is an arbitrary — though endpoint-correct — path between
> them (same behavior as android-maps-utils).

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

### point_at_distance

**`geo::point_at_distance(const Path& path, double distance)`** — Returns the point that lies `distance` meters along the path from its first vertex — "where is the vehicle after N meters of route". The natural companion of `path_length` and the snap-to-route functions.

- `distance` — meters along the path, measured with the same formula as
  `path_length`. Clamped to the path: values `<= 0` return the first vertex
  and values `>= path_length(path)` return the last one, both with their
  coordinates exactly as given.

Returns: `std::optional<LatLng>` — the point, or `std::nullopt` for an empty path.

```cpp
std::vector<geo::LatLng> route = { {0, 0}, {0, 10}, {10, 10} };

auto midpoint = geo::point_at_distance(route, geo::path_length(route) / 2);
auto in_100km = geo::point_at_distance(route, 100'000.0); // {0, ~0.9}
```

---

### area

**`geo::area(const Path& path)`** — Returns the area of a closed path on Earth, in square meters. The path is implicitly closed (last vertex connects back to the first); equivalent to `std::abs(signed_area(path))`.

Returns: `double` — area in square meters, always ≥ 0. For the sign convention use `signed_area`.

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

**`geo::distance_to_segment(const LatLng& point, const LatLng& start, const LatLng& end)`** — Returns the distance in meters from `point` to the closest point on the line segment `[start, end]`. If the perpendicular foot falls outside the segment, returns the distance to the nearer endpoint.

Returns: `double` — distance in meters, always ≥ 0.

> **Approximation.** The closest point is found by planar projection in raw
> `(lat, lng)` coordinate space (same algorithm as android-maps-utils
> `PolyUtil.distanceToLine`), and the great-circle distance to that point is
> returned. Accurate for short segments away from the poles. Two limitations:
> longitude is not scaled by `cos(lat)`, so the result can overshoot by a few
> percent at high latitudes (around `lat 80°`); and longitudes are used as-is,
> so segments crossing the antimeridian (`±180°`) yield meaningless results —
> a point lying exactly on such a segment can report kilometers of distance.
> For tolerance checks against true geodesic segments, use `on_path`; for the
> geodesically correct closest point and distance, use
> `closest_point_on_segment` / `closest_point_on_path`.

```cpp
geo::LatLng start{28.05359, -82.41632};
geo::LatLng end{28.05310, -82.41634};
geo::LatLng point{28.05342, -82.41594};

// Point lies ~38 m east-northeast of the segment
std::cout << geo::distance_to_segment(point, start, end);
```

---

### closest_point_on_segment

**`geo::closest_point_on_segment(const LatLng& p, const LatLng& start, const LatLng& end)`** — Returns the point of the great-circle segment `[start, end]` closest to `p`.

The geodesically correct counterpart of `distance_to_segment`: the segment is
the minor great-circle arc between its endpoints, computed via 3D vector
projection with no planar approximation — accurate at any latitude and across
the antimeridian. The distance from `p` to the segment is
`distance_between(p, closest_point_on_segment(p, start, end))`.

Conventions:

- When the closest point is a segment endpoint, that endpoint is returned
  with its coordinates exactly as given.
- Equal (`operator==`) endpoints yield `start`.
- Antipodal endpoints do not define a unique great circle; the nearer
  endpoint is returned (the same ambiguity `contains` resolves for
  180°-spanning edges).

Returns: `LatLng` — the closest point of the segment.

```cpp
geo::LatLng a{0, 0};
geo::LatLng b{0, 90};

geo::closest_point_on_segment({20, 30}, a, b);  // {0, 30} — perpendicular foot
geo::closest_point_on_segment({10, -20}, a, b); // {0, 0}  — beyond start, endpoint returned

// Across the antimeridian (meaningless for distance_to_segment):
geo::closest_point_on_segment({5, 180}, {0, 170}, {0, -170}); // {0, 180}
```

---

### closest_point_on_path

**`geo::closest_point_on_path(const LatLng& point, const Path& path)`** — Projects the point onto the closest of the path's great-circle segments — "snap to route". Returns `std::nullopt` for an empty path.

The result is a `geo::PathProjection`:

| Field | Type | Meaning |
|---|---|---|
| `point` | `LatLng` | the closest point of the path |
| `segment` | `std::size_t` | index `i`: the point lies on the segment `path[i]` → `path[i + 1]` (`0` for a single-point path) |
| `distance` | `double` | great-circle distance to `point` in meters; always equals `distance_between(point, result.point)` |

Segments are minor great-circle arcs, as in `on_path(geodesic = true)`. If
several segments are equally close — typically when the closest point is a
shared vertex — the lowest segment index wins, and vertex coordinates are
returned exactly as given. Complexity is O(n) in the number of vertices.

Returns: `std::optional<PathProjection>` — the projection, or `std::nullopt` for an empty path.

```cpp
std::vector<geo::LatLng> route = { {0, 0}, {0, 10}, {10, 10} };
geo::LatLng gps{8.0, 9.5};  // a GPS fix near the second leg

auto snapped = geo::closest_point_on_path(gps, route);
// snapped->segment  == 1
// snapped->point    ≈ {8.0, 10}
// snapped->distance ≈ 55 km
```

---

### is_closed_polygon

**`geo::is_closed_polygon(const Path& poly)`** — Returns whether the path is a closed polygon: non-empty, with equal first and last points. Equality is the approximate `LatLng` comparison (`operator==`), so longitudes are compared modulo 360° — a path from `(10, 180)` ending at `(10, -180)` counts as closed. A single point counts as closed.

Returns: `bool` — `true` if the path is non-empty and its first and last points are equal.

```cpp
std::vector<geo::LatLng> poly = {
    {28.06025, -82.41030}, {28.06129, -82.40945}, {28.06206, -82.40917},
};

std::cout << geo::is_closed_polygon(poly); // false
poly.push_back(poly.front());
std::cout << geo::is_closed_polygon(poly); // true
```

---

### simplify

**`geo::simplify(const Path& poly, double tolerance, bool geodesic = false)`** — Simplifies the given polyline or polygon using the [Douglas–Peucker](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm) decimation algorithm: keeps the vertices that lie farther than `tolerance` meters from the simplified shape, drops the rest. The first and last points are always kept, every returned point is one of the input points (in input order), and the input is not modified.

A closed polygon (in the `is_closed_polygon` sense) is simplified including its closing segment, so the result is a closed polygon too.

- `tolerance` — maximum distance in meters a dropped vertex may lie from the simplified path; larger values drop more points.
- `geodesic` — `false` (default) measures distances with `distance_to_segment`
  (upstream PolyUtil behavior); `true` measures against true great-circle
  segments via `closest_point_on_segment`.

Returns: `std::vector<LatLng>` — the simplified path; empty only for an empty input.

> **Note.** With the default `geodesic = false` the `distance_to_segment`
> approximation limits apply — in particular, polylines crossing the
> antimeridian are mis-measured and barely simplify. Pass `geodesic = true`
> for exact behavior at any latitude and across the antimeridian, at roughly
> twice the cost per vertex; vertices near the tolerance threshold may
> resolve differently between the two modes. Worst-case complexity is O(n²)
> in the number of input points.

```cpp
std::vector<geo::LatLng> route = {
    {28.06025, -82.41030}, {28.06129, -82.40945}, {28.06206, -82.40917},
    {28.06125, -82.40850}, {28.06035, -82.40834}, {28.06038, -82.40924},
};

auto simplified = geo::simplify(route, /*tolerance=*/88.0);
std::cout << simplified.size(); // 4 — two vertices within 88 m are dropped
```

---

## Polyline encoding

Encoder and decoder for the [Encoded Polyline Algorithm Format](https://developers.google.com/maps/documentation/utilities/polylinealgorithm)
used by the Google Maps APIs.

```cpp
#include <geo/encoding.hpp>
```

### encode

**`geo::encode(const Path& path, unsigned precision = 5)`** — Encodes a sequence of LatLngs into an encoded path string. Coordinates are quantized to `10^-precision` degrees, so an encode/decode round-trip is lossy beyond that grid.

- `precision` — decimal digits of the quantization grid. The default `5`
  (about one meter) is the classic Google Maps encoding; `6` is the
  **polyline6** variant used by OSRM, Valhalla, and Mapbox. Valid values are
  `0` to `6`: at `6` every valid coordinate and every point-to-point delta
  still fits `decode`'s 32-bit arithmetic (`7` round-trips only deltas under
  ~107°, counting the first point's offset from `(0, 0)`; `8`+ overflows).

Returns: `std::string` — the encoded polyline; empty for an empty path.

```cpp
std::vector<geo::LatLng> path = { {38.5, -120.2}, {40.7, -120.95}, {43.252, -126.453} };

std::cout << geo::encode(path);    // "_p~iF~ps|U_ulLnnqC_mqNvxq`@"
std::cout << geo::encode(path, 6); // "_izlhA~rlgdF_{geC~ywl@_kwzCn`{nI" (polyline6)
```

---

### decode

**`geo::decode(std::string_view encoded, unsigned precision = 5)`** — Decodes an encoded path string into a sequence of LatLngs on the `10^-precision`-degree grid.

- `precision` — must match the precision the string was encoded with: `5`
  (default) for the classic Google Maps encoding, `6` for polyline6
  (OSRM, Valhalla, Mapbox).

Returns: `std::vector<LatLng>` — the decoded points; empty for an empty string.

> **Note.** The input is assumed to be a well-formed encoded polyline.
> Decoding any string is memory-safe, but malformed input yields
> unspecified coordinates; a string truncated mid-point yields the points
> decoded so far and drops the incomplete trailing point.

```cpp
auto path = geo::decode("_p~iF~ps|U_ulLnnqC_mqNvxq`@");

std::cout << path.size(); // 3
std::cout << path[0];     // LatLng(38.5, -120.2)
```

---

## Constants

| Symbol | Value | Description |
|---|---|---|
| `geo::kDefaultTolerance` | `0.1` | Default tolerance in meters for `on_edge` / `on_path` |
