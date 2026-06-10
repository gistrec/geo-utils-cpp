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
#include <optional>

#include "detail/math.hpp"
#include "latlng.hpp"

namespace geo {

/**
 * Returns the heading from one LatLng to another LatLng. Headings are
 * expressed in degrees clockwise from North within the range [-180, 180).
 */
[[nodiscard]] inline double heading(const LatLng& from, const LatLng& to) noexcept {
    double from_lat = detail::deg2rad(from.lat);
    double to_lat = detail::deg2rad(to.lat);
    double d_lng = detail::deg2rad(to.lng - from.lng);
    double cos_to_lat = std::cos(to_lat);
    double sin_to_lat = std::sin(to_lat);
    double cos_from_lat = std::cos(from_lat);
    double sin_from_lat = std::sin(from_lat);
    double h = std::atan2(
        std::sin(d_lng) * cos_to_lat,
        cos_from_lat * sin_to_lat - sin_from_lat * cos_to_lat * std::cos(d_lng));

    return detail::wrap(detail::rad2deg(h), -180.0, 180.0);
}

/**
 * Returns the LatLng resulting from moving a distance from an origin
 * in the specified heading (expressed in degrees clockwise from north).
 */
[[nodiscard]] inline LatLng offset(const LatLng& from, double distance, double heading_deg) noexcept {
    distance /= detail::kEarthRadius;
    double heading_rad = detail::deg2rad(heading_deg);
    double from_lat = detail::deg2rad(from.lat);
    double from_lng = detail::deg2rad(from.lng);
    double cos_distance = std::cos(distance);
    double sin_distance = std::sin(distance);
    double sin_from_lat = std::sin(from_lat);
    double cos_from_lat = std::cos(from_lat);
    double sin_lat = cos_distance * sin_from_lat + sin_distance * cos_from_lat * std::cos(heading_rad);
    double d_lng = std::atan2(
        sin_distance * cos_from_lat * std::sin(heading_rad),
        cos_distance - sin_from_lat * sin_lat);

    return LatLng(detail::rad2deg(std::asin(std::clamp(sin_lat, -1.0, 1.0))),
                  detail::rad2deg(from_lng + d_lng));
}

/**
 * Returns the location of origin when provided with a destination,
 * meters travelled and original heading. Returns nullopt when no solution exists.
 */
[[nodiscard]] inline std::optional<LatLng> offset_origin(const LatLng& to, double distance, double heading_deg) noexcept {
    double heading_rad = detail::deg2rad(heading_deg);
    distance /= detail::kEarthRadius;
    double n1 = std::cos(distance);
    double n2 = std::sin(distance) * std::cos(heading_rad);
    double n3 = std::sin(distance) * std::sin(heading_rad);
    double n4 = std::sin(detail::deg2rad(to.lat));
    // Rewrite n4 = n1*sin(φ) + n2*cos(φ) as r*sin(φ + α) = n4,
    // where r = sqrt(n1²+n2²), α = atan2(n2, n1).
    // Solving via asin avoids dividing by n1, which causes catastrophic
    // cancellation when n1 ≈ 0 (i.e. distance ≈ π/2·R).
    double r = std::sqrt(n1 * n1 + n2 * n2);
    if (r < 1e-10) return std::nullopt;
    double sin_arg = n4 / r;
    // When the true solution sits exactly on the domain boundary (|n4| == r,
    // e.g. destination at a pole), rounding in r can push n4 / r marginally
    // outside [-1, 1] and a strict check would reject an existing solution.
    // Clamp the FP noise; reject only genuinely unreachable destinations.
    constexpr double kAsinDomainEps = 1e-9;
    if (sin_arg < -1.0 - kAsinDomainEps || sin_arg > 1.0 + kAsinDomainEps) return std::nullopt;
    sin_arg = std::clamp(sin_arg, -1.0, 1.0);
    double alpha = std::atan2(n2, n1);
    double from_lat_radians = std::asin(sin_arg) - alpha;
    if (from_lat_radians < -detail::kPi / 2 || from_lat_radians > detail::kPi / 2) {
        from_lat_radians = detail::kPi - std::asin(sin_arg) - alpha;
    }
    if (from_lat_radians < -detail::kPi / 2 || from_lat_radians > detail::kPi / 2) return std::nullopt;
    double from_lng_radians = detail::deg2rad(to.lng) -
        std::atan2(n3, n1 * std::cos(from_lat_radians) - n2 * std::sin(from_lat_radians));
    return LatLng(detail::rad2deg(from_lat_radians), detail::rad2deg(from_lng_radians));
}

/**
 * Returns the angle between two LatLngs, in radians. This is the same as the
 * distance on the unit sphere.
 */
[[nodiscard]] inline double angle_between(const LatLng& from, const LatLng& to) noexcept {
    double lat1 = detail::deg2rad(from.lat);
    double lng1 = detail::deg2rad(from.lng);
    double lat2 = detail::deg2rad(to.lat);
    double lng2 = detail::deg2rad(to.lng);
    return detail::arc_hav(detail::hav_distance(lat1, lat2, lng1 - lng2));
}

/**
 * Returns the distance between two LatLngs, in meters.
 */
[[nodiscard]] inline double distance_between(const LatLng& from, const LatLng& to) noexcept {
    return angle_between(from, to) * detail::kEarthRadius;
}

/**
 * Returns the LatLng which lies the given fraction of the way between the
 * origin and the destination (spherical linear interpolation).
 */
[[nodiscard]] inline LatLng interpolate(const LatLng& from, const LatLng& to, double fraction) noexcept {
    double from_lat = detail::deg2rad(from.lat);
    double from_lng = detail::deg2rad(from.lng);
    double to_lat = detail::deg2rad(to.lat);
    double to_lng = detail::deg2rad(to.lng);
    double cos_from_lat = std::cos(from_lat);
    double cos_to_lat = std::cos(to_lat);
    double angle = angle_between(from, to);
    double sin_angle = std::sin(angle);
    if (sin_angle < 1e-6) {
        return LatLng(
            from.lat + fraction * (to.lat - from.lat),
            from.lng + fraction * (to.lng - from.lng));
    }
    double a = std::sin((1.0 - fraction) * angle) / sin_angle;
    double b = std::sin(fraction * angle) / sin_angle;
    double x = a * cos_from_lat * std::cos(from_lng) + b * cos_to_lat * std::cos(to_lng);
    double y = a * cos_from_lat * std::sin(from_lng) + b * cos_to_lat * std::sin(to_lng);
    double z = a * std::sin(from_lat) + b * std::sin(to_lat);
    double lat = std::atan2(z, std::sqrt(x * x + y * y));
    double lng = std::atan2(y, x);
    return LatLng(detail::rad2deg(lat), detail::rad2deg(lng));
}

/**
 * Returns the length of the given path, in meters, on Earth.
 */
template <typename Path>
[[nodiscard]] double path_length(const Path& path) {
    const std::size_t size = path.size();
    if (size < 2U) {
        return 0.0;
    }
    double length = 0.0;
    double prev_lat = detail::deg2rad(path[0].lat);
    double prev_lng = detail::deg2rad(path[0].lng);
    for (std::size_t i = 1; i < size; ++i) {
        double lat = detail::deg2rad(path[i].lat);
        double lng = detail::deg2rad(path[i].lng);
        length += detail::arc_hav(detail::hav_distance(prev_lat, lat, prev_lng - lng));
        prev_lat = lat;
        prev_lng = lng;
    }
    return length * detail::kEarthRadius;
}

/**
 * Returns the signed area of a closed path on Earth, in square meters.
 *
 * Sign convention: counter-clockwise when viewed from outside the "inside"
 * face yields a positive result; clockwise yields a negative result.
 * "Inside" is the surface that does not contain the South Pole.
 */
template <typename Path>
[[nodiscard]] double signed_area(const Path& path) {
    std::size_t size = path.size();
    if (size < 3U) { return 0.0; }
    double total = 0.0;
    const auto& last = path[size - 1];
    double prev_tan_lat = std::tan((detail::kPi / 2 - detail::deg2rad(last.lat)) / 2);
    double prev_lng = detail::deg2rad(last.lng);
    // For each edge, accumulate the signed area of the polar triangle (formed
    // by the North Pole and that edge). Formula derived from "Area of a
    // spherical triangle given two edges and the included angle"
    // (Todhunter, "Spherical Trigonometry", page 71, section 103, point 2).
    for (const auto& point : path) {
        double tan_lat = std::tan((detail::kPi / 2 - detail::deg2rad(point.lat)) / 2);
        double lng = detail::deg2rad(point.lng);
        double delta_lng = lng - prev_lng;
        double t = tan_lat * prev_tan_lat;
        total += 2 * std::atan2(t * std::sin(delta_lng), 1 + t * std::cos(delta_lng));
        prev_tan_lat = tan_lat;
        prev_lng = lng;
    }
    return total * (detail::kEarthRadius * detail::kEarthRadius);
}

/**
 * Returns the area of a closed path on Earth.
 */
template <typename Path>
[[nodiscard]] double area(const Path& path) {
    return std::abs(signed_area(path));
}

}  // namespace geo
