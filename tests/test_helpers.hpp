#pragma once
#include <cmath>
#include <gtest/gtest.h>
#include <geo/detail/math.hpp>

// Checks that two LatLng values agree within 1e-6 degrees. The longitude
// difference is wrapped to [-180, 180) so that values straddling the
// antimeridian (e.g. 179.9999999 vs -179.9999999, or -180 vs +180) compare
// as equal, and weighted by cos(lat) so the assertion degenerates gracefully
// at poles where longitude is geometrically undefined.
#define EXPECT_NEAR_LatLng(expected, actual)                                            \
    do {                                                                                \
        EXPECT_NEAR((expected).lat, (actual).lat, 1e-6);                                \
        const double _cosLat = std::cos(::geo::detail::deg2rad((expected).lat));        \
        const double _dLng = ::geo::detail::wrap((expected).lng - (actual).lng,         \
                                                 -180.0, 180.0);                        \
        EXPECT_NEAR(_cosLat * _dLng, 0.0, 1e-6)                                         \
            << "expected lng " << (expected).lng << ", actual lng " << (actual).lng;    \
    } while (false)
