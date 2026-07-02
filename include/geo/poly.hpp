// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Portions of this file are based on Google Maps Android Utils:
// https://github.com/googlemaps/android-maps-utils
//
// Original work:
// Copyright 2013 Google Inc.
// Licensed under the Apache License, Version 2.0
//
// This file has been modified from the original work,
// including a port from Java to C++.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "detail/math.hpp"
#include "latlng.hpp"
#include "spherical.hpp"

namespace geo {

inline constexpr double kDefaultTolerance = 0.1;  // meters

namespace detail {

// Returns tan(latitude-at-lng3) on the great circle (lat1, 0) to (lat2, lng2).
[[nodiscard]] inline double tan_lat_gc(double lat1, double lat2, double lng2, double lng3) noexcept {
    return (std::tan(lat1) * std::sin(lng2 - lng3) + std::tan(lat2) * std::sin(lng3)) / std::sin(lng2);
}

// Returns mercator(latitude-at-lng3) on the Rhumb line (lat1, 0) to (lat2, lng2).
[[nodiscard]] inline double mercator_lat_rhumb(double lat1, double lat2, double lng2, double lng3) noexcept {
    return (mercator(lat1) * (lng2 - lng3) + mercator(lat2) * lng3) / lng2;
}

// Computes whether the vertical segment (lat3, lng3) to South Pole intersects
// the segment (lat1, 0) to (lat2, lng2). Longitudes are offset so lng1 == 0.
[[nodiscard]] inline bool intersects(double lat1, double lat2, double lng2, double lat3, double lng3, bool geodesic) noexcept {
    if ((lng3 >= 0 && lng3 >= lng2) || (lng3 < 0 && lng3 < lng2)) {
        return false;
    }
    if (lat3 <= -kPi / 2) {
        return false;
    }
    if (lat1 <= -kPi / 2 || lat2 <= -kPi / 2 || lat1 >= kPi / 2 || lat2 >= kPi / 2) {
        return false;
    }
    // An edge spanning exactly 180° of longitude (lng2 == -kPi after wrapping)
    // has an ambiguous direction — two equal great-circle arcs connect its
    // endpoints. Never count it as an intersection (upstream PolyUtil
    // convention); without this, tan_lat_gc divides by sin(-π) ≈ -1.2e-16.
    if (lng2 <= -kPi) {
        return false;
    }
    double linear_lat = (lat1 * (lng2 - lng3) + lat2 * lng3) / lng2;
    if (lat1 >= 0 && lat2 >= 0 && lat3 < linear_lat) {
        return false;
    }
    if (lat1 <= 0 && lat2 <= 0 && lat3 >= linear_lat) {
        return true;
    }
    if (lat3 >= kPi / 2) {
        return true;
    }
    return geodesic
        ? std::tan(lat3) >= tan_lat_gc(lat1, lat2, lng2, lng3)
        : mercator(lat3) >= mercator_lat_rhumb(lat1, lat2, lng2, lng3);
}

// Returns sin(initial bearing from (lat1,lng1) to (lat3,lng3) minus initial
// bearing from (lat1,lng1) to (lat2,lng2)).
[[nodiscard]] inline double sin_delta_bearing(double lat1, double lng1, double lat2, double lng2, double lat3, double lng3) noexcept {
    double sin_lat1 = std::sin(lat1);
    double cos_lat2 = std::cos(lat2);
    double cos_lat3 = std::cos(lat3);
    double lat31 = lat3 - lat1;
    double lng31 = lng3 - lng1;
    double lat21 = lat2 - lat1;
    double lng21 = lng2 - lng1;
    double a = std::sin(lng31) * cos_lat3;
    double c = std::sin(lng21) * cos_lat2;
    double b = std::sin(lat31) + 2 * sin_lat1 * cos_lat3 * hav(lng31);
    double d = std::sin(lat21) + 2 * sin_lat1 * cos_lat2 * hav(lng21);
    double denom = (a * a + b * b) * (c * c + d * d);
    return denom <= 0 ? 1 : (a * d - b * c) / std::sqrt(denom);
}

[[nodiscard]] inline bool is_on_segment_gc(double lat1, double lng1, double lat2, double lng2, double lat3, double lng3, double hav_tolerance) noexcept {
    double hav_dist13 = hav_distance(lat1, lat3, lng1 - lng3);
    if (hav_dist13 <= hav_tolerance) {
        return true;
    }
    double hav_dist23 = hav_distance(lat2, lat3, lng2 - lng3);
    if (hav_dist23 <= hav_tolerance) {
        return true;
    }
    double sin_bearing = sin_delta_bearing(lat1, lng1, lat2, lng2, lat3, lng3);
    double sin_dist13 = sin_from_hav(hav_dist13);
    double hav_cross_track = hav_from_sin(sin_dist13 * sin_bearing);
    if (hav_cross_track > hav_tolerance) {
        return false;
    }
    double hav_dist12 = hav_distance(lat1, lat2, lng1 - lng2);
    double term = hav_dist12 + hav_cross_track * (1 - 2 * hav_dist12);
    if (hav_dist13 > term || hav_dist23 > term) {
        return false;
    }
    if (hav_dist12 < 0.74) {
        return true;
    }
    double cos_cross_track = 1 - 2 * hav_cross_track;
    double hav_along_track13 = (hav_dist13 - hav_cross_track) / cos_cross_track;
    double hav_along_track23 = (hav_dist23 - hav_cross_track) / cos_cross_track;
    double sin_sum_along_track = sin_sum_from_hav(hav_along_track13, hav_along_track23);
    return sin_sum_along_track > 0;
}

// A direction on the unit sphere in Cartesian coordinates.
struct Vec3 {
    double x;
    double y;
    double z;
};

[[nodiscard]] inline Vec3 to_unit_vec(const LatLng& point) noexcept {
    double lat = deg2rad(point.lat);
    double lng = deg2rad(point.lng);
    double cos_lat = std::cos(lat);
    return {cos_lat * std::cos(lng), cos_lat * std::sin(lng), std::sin(lat)};
}

// Scale-invariant: v does not have to be normalized.
[[nodiscard]] inline LatLng vec_to_latlng(const Vec3& v) noexcept {
    return LatLng(rad2deg(std::atan2(v.z, std::sqrt(v.x * v.x + v.y * v.y))),
                  rad2deg(std::atan2(v.y, v.x)));
}

[[nodiscard]] inline Vec3 cross(const Vec3& u, const Vec3& v) noexcept {
    return {u.y * v.z - u.z * v.y,
            u.z * v.x - u.x * v.z,
            u.x * v.y - u.y * v.x};
}

[[nodiscard]] inline double dot(const Vec3& u, const Vec3& v) noexcept {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

// Angle between two directions, in radians. Scale-invariant and, unlike an
// acos of the dot product, stable for both small and near-pi angles.
[[nodiscard]] inline double vec_angle(const Vec3& u, const Vec3& v) noexcept {
    Vec3 c = cross(u, v);
    return std::atan2(std::sqrt(dot(c, c)), dot(u, v));
}

// Which point of the minor great-circle arc a->b is closest to p: the
// perpendicular projection of p onto the arc's great circle (written to proj,
// not normalized), or one of the endpoints. All inputs are unit vectors.
enum class ArcNearest { kProjection, kStart, kEnd };

[[nodiscard]] inline ArcNearest nearest_on_arc(const Vec3& a, const Vec3& b, const Vec3& p, Vec3& proj) noexcept {
    // The nearer endpoint (larger dot product == smaller angle) is also the
    // endpoint nearer to the projection along the circle, so it doubles as
    // the answer whenever the projection falls outside the arc.
    const Vec3 n = cross(a, b);
    const double n2 = dot(n, n);
    // n2 == sin^2(arc length). Degenerate arcs — endpoints closer than
    // ~1e-12 rad or that close to antipodal — have no usable great-circle
    // normal: for identical endpoints the arc is a point, for antipodal ones
    // the connecting great circle is not unique (same ambiguity contains()
    // resolves for 180-degree edges by convention). Use the nearer endpoint.
    if (n2 <= 1e-24) {
        return dot(p, a) >= dot(p, b) ? ArcNearest::kStart : ArcNearest::kEnd;
    }
    const double k = dot(p, n) / n2;
    proj = {p.x - k * n.x, p.y - k * n.y, p.z - k * n.z};
    // p at a pole of the great circle: the whole circle is 90 degrees away,
    // so the nearer endpoint is as close as any point of the arc.
    if (dot(proj, proj) <= 1e-24) {
        return dot(p, a) >= dot(p, b) ? ArcNearest::kStart : ArcNearest::kEnd;
    }
    // proj lies within the minor arc iff it is on b's side of the plane
    // spanned by a and n, and on a's side of the plane spanned by b and n.
    if (dot(cross(a, proj), n) >= 0 && dot(cross(proj, b), n) >= 0) {
        return ArcNearest::kProjection;
    }
    return dot(p, a) >= dot(p, b) ? ArcNearest::kStart : ArcNearest::kEnd;
}

// Computes whether a given point lies on or near a polyline within a tolerance.
template <typename Path>
[[nodiscard]] bool on_edge_or_path(const LatLng& point, const Path& poly, bool closed, bool geodesic, double tolerance_earth) {
    std::size_t size = poly.size();
    if (size == 0U) {
        return false;
    }

    double tolerance = tolerance_earth / kEarthRadius;
    double hav_tolerance = hav(tolerance);
    double lat3 = deg2rad(point.lat);
    double lng3 = deg2rad(point.lng);
    const auto& start = poly[closed ? size - 1 : 0];
    double lat1 = deg2rad(start.lat);
    double lng1 = deg2rad(start.lng);

    if (geodesic) {
        for (const auto& val : poly) {
            double lat2 = deg2rad(val.lat);
            double lng2 = deg2rad(val.lng);
            if (is_on_segment_gc(lat1, lng1, lat2, lng2, lat3, lng3, hav_tolerance)) {
                return true;
            }
            lat1 = lat2;
            lng1 = lng2;
        }
    } else {
        // Project to mercator space where rhumb segments are straight lines,
        // then compute the geodesic distance to the closest point on the
        // segment. Approximate, but the error is small for small tolerances.
        double min_acceptable = lat3 - tolerance;
        double max_acceptable = lat3 + tolerance;
        double y1 = mercator(lat1);
        double y3 = mercator(lat3);
        double x_try[3];
        for (const auto& val : poly) {
            double lat2 = deg2rad(val.lat);
            double y2 = mercator(lat2);
            double lng2 = deg2rad(val.lng);
            if (std::max(lat1, lat2) >= min_acceptable && std::min(lat1, lat2) <= max_acceptable) {
                double x2 = wrap(lng2 - lng1, -kPi, kPi);
                double x3_base = wrap(lng3 - lng1, -kPi, kPi);
                x_try[0] = x3_base;
                x_try[1] = x3_base + 2 * kPi;
                x_try[2] = x3_base - 2 * kPi;

                for (auto x3 : x_try) {
                    double dy = y2 - y1;
                    double len2 = x2 * x2 + dy * dy;
                    double t = len2 <= 0 ? 0 : std::clamp((x3 * x2 + (y3 - y1) * dy) / len2, 0.0, 1.0);
                    double x_closest = t * x2;
                    double y_closest = y1 + t * dy;
                    double lat_closest = inverse_mercator(y_closest);
                    double hav_dist = hav_distance(lat3, lat_closest, x3 - x_closest);
                    if (hav_dist < hav_tolerance) {
                        return true;
                    }
                }
            }
            lat1 = lat2;
            lng1 = lng2;
            y1 = y2;
        }
    }
    return false;
}

}  // namespace detail

/**
 * Computes whether the given point lies inside the specified polygon.
 * The polygon is always considered closed. Inside is defined as not containing
 * the South Pole. Edges are great circle arcs if geodesic is true, rhumb lines otherwise.
 */
template <typename Path>
[[nodiscard]] bool contains(const LatLng& point, const Path& polygon, bool geodesic = false) {
    std::size_t size = polygon.size();
    if (size == 0) {
        return false;
    }
    double lat3 = detail::deg2rad(point.lat);
    double lng3 = detail::deg2rad(point.lng);
    const auto& last = polygon[size - 1];
    double lat1 = detail::deg2rad(last.lat);
    double lng1 = detail::deg2rad(last.lng);

    std::size_t n_intersect = 0;

    for (const auto& val : polygon) {
        double d_lng3 = detail::wrap(lng3 - lng1, -detail::kPi, detail::kPi);
        if (lat3 == lat1 && d_lng3 == 0) {
            return true;
        }

        double lat2 = detail::deg2rad(val.lat);
        double lng2 = detail::deg2rad(val.lng);

        if (detail::intersects(lat1, lat2, detail::wrap(lng2 - lng1, -detail::kPi, detail::kPi), lat3, d_lng3, geodesic)) {
            ++n_intersect;
        }
        lat1 = lat2;
        lng1 = lng2;
    }
    return (n_intersect & 1) != 0;
}

/**
 * Computes whether the given point lies on or near the edge of a polygon,
 * within a specified tolerance in meters. The polygon is implicitly closed.
 */
template <typename Path>
[[nodiscard]] bool on_edge(const LatLng& point, const Path& polygon, bool geodesic = true, double tolerance = kDefaultTolerance) {
    return detail::on_edge_or_path(point, polygon, true, geodesic, tolerance);
}

/**
 * Computes whether the given point lies on or near a polyline, within a
 * specified tolerance in meters. The polyline is not closed.
 */
template <typename Path>
[[nodiscard]] bool on_path(const LatLng& point, const Path& polyline, bool geodesic = true, double tolerance = kDefaultTolerance) {
    return detail::on_edge_or_path(point, polyline, false, geodesic, tolerance);
}

/**
 * Computes the distance between the point p and the line segment
 * (start, end), in meters.
 *
 * Approximation: the closest point is found by planar projection in raw
 * (lat, lng) coordinate space, then the great-circle distance to it is
 * returned. Accurate for short segments away from the poles. Limitations:
 * - longitude is not scaled by cos(lat), so the projection skews at high
 *   latitudes and the result can overshoot by a few percent around lat 80°;
 * - longitudes are used as-is: a segment crossing the antimeridian (±180°)
 *   is treated as spanning nearly the whole globe and yields meaningless
 *   results (a point on such a segment can report a distance of kilometers).
 * For tolerance checks against true geodesic segments use on_path instead.
 */
[[nodiscard]] inline double distance_to_segment(const LatLng& p, const LatLng& start, const LatLng& end) noexcept {
    if (start == end) {
        return distance_between(end, p);
    }
    double s0lat = detail::deg2rad(p.lat);
    double s0lng = detail::deg2rad(p.lng);
    double s1lat = detail::deg2rad(start.lat);
    double s1lng = detail::deg2rad(start.lng);
    double s2lat = detail::deg2rad(end.lat);
    double s2lng = detail::deg2rad(end.lng);
    double s2s1lat = s2lat - s1lat;
    double s2s1lng = s2lng - s1lng;
    double u = ((s0lat - s1lat) * s2s1lat + (s0lng - s1lng) * s2s1lng)
             / (s2s1lat * s2s1lat + s2s1lng * s2s1lng);
    if (u <= 0) {
        return distance_between(p, start);
    }
    if (u >= 1) {
        return distance_between(p, end);
    }
    LatLng su(start.lat + u * (end.lat - start.lat), start.lng + u * (end.lng - start.lng));
    return distance_between(p, su);
}

/**
 * Returns the point of the great-circle segment [start, end] closest to p.
 *
 * The geodesically correct counterpart of distance_to_segment: the segment is
 * the minor great-circle arc between its endpoints, with no planar
 * approximation — accurate at any latitude and across the antimeridian. The
 * distance from p to the segment is distance_between(p, closest_point_on_segment(...)).
 *
 * Conventions: when the closest point is a segment endpoint, that endpoint is
 * returned with its coordinates exactly as given. Equal (operator==)
 * endpoints yield start. Antipodal endpoints do not define a unique great
 * circle — the nearer endpoint is returned.
 */
[[nodiscard]] inline LatLng closest_point_on_segment(const LatLng& p, const LatLng& start, const LatLng& end) noexcept {
    if (start == end) {
        return start;
    }
    detail::Vec3 proj{0.0, 0.0, 0.0};
    const detail::ArcNearest nearest = detail::nearest_on_arc(
        detail::to_unit_vec(start), detail::to_unit_vec(end), detail::to_unit_vec(p), proj);
    if (nearest == detail::ArcNearest::kStart) {
        return start;
    }
    if (nearest == detail::ArcNearest::kEnd) {
        return end;
    }
    return detail::vec_to_latlng(proj);
}

/**
 * The result of projecting a point onto a path: the closest point, the index
 * of the segment it lies on (from path[segment] to path[segment + 1]; 0 for
 * a single-point path), and the great-circle distance to it in meters —
 * always equal to distance_between(point, result.point).
 */
struct PathProjection {
    LatLng point;
    std::size_t segment;
    double distance;
};

/**
 * Projects the point onto the closest of the path's great-circle segments
 * ("snap to route"). Segments are minor great-circle arcs, as in
 * on_path(geodesic = true), and the result is geodesically correct at any
 * latitude and across the antimeridian — unlike distance_to_segment.
 *
 * Returns std::nullopt for an empty path. When several segments are equally
 * close — typically when the closest point is a shared vertex — the lowest
 * segment index wins. O(n) in the number of vertices.
 */
template <typename Path>
[[nodiscard]] std::optional<PathProjection> closest_point_on_path(const LatLng& point, const Path& path) {
    const std::size_t size = path.size();
    if (size == 0) {
        return std::nullopt;
    }
    const LatLng first(path[0].lat, path[0].lng);
    if (size == 1) {
        return PathProjection{first, 0, distance_between(point, first)};
    }

    // Vertices are converted once and shared between adjacent segments, so
    // equally-close candidates at a shared vertex compare bit-identically and
    // the strict < keeps the earliest segment.
    const detail::Vec3 p = detail::to_unit_vec(point);
    detail::Vec3 prev = detail::to_unit_vec(first);

    double best_angle = std::numeric_limits<double>::infinity();
    std::size_t best_segment = 0;
    detail::ArcNearest best_nearest = detail::ArcNearest::kStart;
    detail::Vec3 best_proj{0.0, 0.0, 0.0};

    for (std::size_t i = 1; i < size; ++i) {
        const detail::Vec3 cur = detail::to_unit_vec(LatLng(path[i].lat, path[i].lng));
        detail::Vec3 proj{0.0, 0.0, 0.0};
        const detail::ArcNearest nearest = detail::nearest_on_arc(prev, cur, p, proj);
        const detail::Vec3& candidate =
            nearest == detail::ArcNearest::kProjection ? proj
            : nearest == detail::ArcNearest::kStart    ? prev
                                                       : cur;
        const double angle = detail::vec_angle(p, candidate);
        if (angle < best_angle) {
            best_angle = angle;
            best_segment = i - 1;
            best_nearest = nearest;
            best_proj = proj;
        }
        prev = cur;
    }

    // Endpoint results reuse the original vertex coordinates exactly.
    const LatLng best_point =
        best_nearest == detail::ArcNearest::kStart
            ? LatLng(path[best_segment].lat, path[best_segment].lng)
        : best_nearest == detail::ArcNearest::kEnd
            ? LatLng(path[best_segment + 1].lat, path[best_segment + 1].lng)
            : detail::vec_to_latlng(best_proj);
    return PathProjection{best_point, best_segment, distance_between(point, best_point)};
}

/**
 * Returns whether the path is a closed polygon: non-empty, with equal first
 * and last points. Equality is the approximate LatLng comparison, with
 * longitudes compared modulo 360°.
 */
template <typename Path>
[[nodiscard]] bool is_closed_polygon(const Path& poly) {
    std::size_t size = poly.size();
    if (size == 0) {
        return false;
    }
    const auto& first = poly[0];
    const auto& last = poly[size - 1];
    return LatLng(first.lat, first.lng) == LatLng(last.lat, last.lng);
}

/**
 * Simplifies the given polyline or polygon using the Douglas-Peucker
 * decimation algorithm: keeps the vertices that lie farther than tolerance
 * meters from the simplified shape, drops the rest. The first and last
 * points are always kept, and every returned point is one of the input
 * points. A closed polygon (is_closed_polygon) is simplified including its
 * closing segment.
 *
 * By default distances are measured with distance_to_segment (upstream
 * PolyUtil behavior), so its planar-approximation limits apply — in
 * particular for segments crossing the antimeridian. Pass geodesic = true
 * to measure against true great-circle segments via
 * closest_point_on_segment: exact at any latitude and across the
 * antimeridian, at two to three times the cost per vertex. Results of the
 * two modes can differ for vertices near the tolerance threshold.
 * Worst-case complexity is O(n^2).
 */
template <typename Path>
[[nodiscard]] std::vector<LatLng> simplify(const Path& poly, double tolerance, bool geodesic = false) {
    std::size_t n = poly.size();
    if (n == 0) {
        return {};
    }

    // Work on a copy: for a closed polygon the last point is nudged slightly
    // off the first one so Douglas-Peucker "sees" the closing segment
    // (upstream PolyUtil trick); the output is filtered from the original
    // points, so the nudge never leaks into the result.
    std::vector<LatLng> working;
    working.reserve(n);
    for (const auto& point : poly) {
        working.emplace_back(point.lat, point.lng);
    }
    if (is_closed_polygon(working)) {
        constexpr double offset = 1e-11;
        working.back() = LatLng(working.back().lat + offset, working.back().lng + offset);
    }

    std::vector<bool> keep(n, false);
    keep.front() = true;
    keep.back() = true;

    if (n > 2) {
        std::vector<std::pair<std::size_t, std::size_t>> stack;
        stack.emplace_back(0, n - 1);
        while (!stack.empty()) {
            const auto [start, end] = stack.back();
            stack.pop_back();

            double max_dist = 0;
            std::size_t max_idx = 0;
            for (std::size_t i = start + 1; i < end; ++i) {
                double dist = geodesic
                    ? distance_between(working[i],
                          closest_point_on_segment(working[i], working[start], working[end]))
                    : distance_to_segment(working[i], working[start], working[end]);
                if (dist > max_dist) {
                    max_dist = dist;
                    max_idx = i;
                }
            }
            if (max_dist > tolerance) {
                keep[max_idx] = true;
                stack.emplace_back(start, max_idx);
                stack.emplace_back(max_idx, end);
            }
        }
    }

    std::vector<LatLng> result;
    for (std::size_t i = 0; i < n; ++i) {
        if (keep[i]) {
            const auto& point = poly[i];
            result.emplace_back(point.lat, point.lng);
        }
    }
    return result;
}

}  // namespace geo
