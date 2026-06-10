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
 * Computes the spherical distance between the point p and the line segment
 * (start, end), in meters.
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

}  // namespace geo
