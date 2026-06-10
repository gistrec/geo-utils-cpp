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
