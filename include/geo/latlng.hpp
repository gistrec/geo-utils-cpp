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

#include <cmath>
#include <ostream>

namespace geo {

/**
 * A point in geographical coordinates: latitude and longitude, in degrees.
 * Every geo:: function takes and returns coordinates in degrees; radians are
 * never used in the public API.
 *
 * Valid ranges:
 * - latitude:  [-90, 90]
 * - longitude: [-180, 180]; 180 and -180 denote the same meridian
 *
 * The constructor stores the values exactly as given — no validation,
 * clamping, or wrapping (use is_valid() to check coordinates from untrusted
 * sources). Passing an out-of-range LatLng to geo:: functions is memory-safe
 * and does not throw, but the results are unspecified: the values feed
 * straight into spherical trigonometry, so e.g. LatLng(180, 180) — latitude
 * "wrapped over the pole" — behaves as the direction (0, 0) in distance
 * computations while still comparing unequal to LatLng(0, 0).
 */
struct LatLng {
    // Default tolerance used by operator==. Roughly 0.1 micrometers (111 nm)
    // on Earth — tighter than any practical computation, but loose enough for
    // round-trips through deg2rad/rad2deg and similar identities.
    static constexpr double kDefaultEpsilon = 1e-12;

    double lat;
    double lng;

    constexpr LatLng(double lat, double lng) noexcept
        : lat(lat), lng(lng) {}

    LatLng(const LatLng&) noexcept = default;
    LatLng& operator=(const LatLng&) noexcept = default;

    /**
     * Returns whether this is a valid geographic coordinate: latitude in
     * [-90, 90] and longitude in [-180, 180], both finite. NaN and infinite
     * components are invalid.
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return lat >= -90.0 && lat <= 90.0 && lng >= -180.0 && lng <= 180.0;
    }

    /**
     * Approximate equality with a custom tolerance (in degrees, applied to both
     * latitude and longitude). Longitudes are compared modulo 360 so that 180°
     * and -180° are treated as equal (same meridian).
     */
    [[nodiscard]] bool approx_equal(const LatLng& other, double eps = kDefaultEpsilon) const noexcept {
        if (std::fabs(lat - other.lat) >= eps) return false;
        double diff = std::fabs(std::fmod(lng - other.lng, 360.0));
        if (diff > 180.0) diff = 360.0 - diff;
        return diff < eps;
    }

    [[nodiscard]] bool operator==(const LatLng& other) const noexcept {
        return approx_equal(other);
    }

    [[nodiscard]] bool operator!=(const LatLng& other) const noexcept {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const LatLng& p) {
        return os << "LatLng(" << p.lat << ", " << p.lng << ")";
    }
};

}  // namespace geo
