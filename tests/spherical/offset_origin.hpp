#include <gtest/gtest.h>

#include <geo/spherical.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::distance_between;
using geo::offset;
using geo::offset_origin;
using geo::detail::kEarthRadius;
using geo::detail::kPi;

TEST(Spherical, offset_origin) {
    LatLng front = {  0.0,    0.0 };

    // Zero distance: origin equals destination
    {
        auto r = offset_origin(front, 0, 0);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(front, r.value());
    }

    // Valid cardinal recoveries
    {
        auto r = offset_origin(LatLng(  0,  45), kPi * kEarthRadius / 4,  90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(front, r.value());
    }
    {
        auto r = offset_origin(LatLng(  0, -45), kPi * kEarthRadius / 4, -90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(front, r.value());
    }
    {
        auto r = offset_origin(LatLng( 45,   0), kPi * kEarthRadius / 4,   0);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(front, r.value());
    }
    {
        auto r = offset_origin(LatLng(-45,   0), kPi * kEarthRadius / 4, 180);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(front, r.value());
    }

    // No-solution cases (Issue #3): destination unreachable with given distance and heading.
    EXPECT_FALSE(offset_origin(LatLng(80, 0), kPi * kEarthRadius / 4, 180).has_value());
    EXPECT_FALSE(offset_origin(LatLng(80, 0), kPi * kEarthRadius / 4,  90).has_value());

    // Longitude regression: offset({0,30}, 45° arc, east) = {0,75},
    // so the inverse must recover lng=30, not just lat.
    {
        auto r = offset_origin(LatLng(0, 75), kPi * kEarthRadius / 4, 90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR(r->lat,  0.0, 1e-6);
        EXPECT_NEAR(r->lng, 30.0, 1e-6);
    }

    // Same check heading west: offset({0,-30}, 45° arc, west) = {0,-75}
    {
        auto r = offset_origin(LatLng(0, -75), kPi * kEarthRadius / 4, -90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR(r->lat,   0.0, 1e-6);
        EXPECT_NEAR(r->lng, -30.0, 1e-6);
    }

    // Round-trip at distance = π/2·R (quarter sphere).
    // At this distance n1 = cos(π/2) ≈ 0, causing catastrophic cancellation in
    // the original a = (n4 - n2·b)/n1 formula. Two valid origins exist at this
    // distance; verify the returned one actually maps back to 'to'.
    {
        const double half_pi_R = kPi / 2 * kEarthRadius;
        LatLng to = offset(LatLng(30.0, 20.0), half_pi_R, 45.0);
        auto r = offset_origin(to, half_pi_R, 45.0);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR_LatLng(to, offset(r.value(), half_pi_R, 45.0));
    }

    // Regression: destination at the North Pole, heading 0. Mathematically
    // sin_arg == 1 exactly, but rounding in r = sqrt(n1² + n2²) can push
    // n4 / r marginally above 1; a strict domain check used to reject such
    // queries as nullopt depending on how r rounded for the given distance
    // (e.g. frac 0.1 and 0.4 failed while 0.2 and 0.3 worked).
    // Tolerances are looser than elsewhere: asin is ill-conditioned at the
    // domain boundary, so 1 ulp of input noise moves the result by ~0.1 m.
    // Verify via round-trip distance: longitude is degenerate at the pole.
    for (double frac : {0.1, 0.2, 0.25, 0.3, 0.4}) {
        const double d = frac * kPi * kEarthRadius;
        auto r = offset_origin(LatLng(90, 0), d, 0);
        ASSERT_TRUE(r.has_value()) << "frac=" << frac;
        EXPECT_NEAR(r->lat, 90.0 - frac * 180.0, 1e-5) << "frac=" << frac;
        EXPECT_LT(distance_between(LatLng(90, 0), offset(r.value(), d, 0)), 0.5) << "frac=" << frac;
    }

    // South Pole, heading 180 — exercises the sin_arg < -1 side of the same issue.
    {
        const double d = 0.1 * kPi * kEarthRadius;
        auto r = offset_origin(LatLng(-90, 0), d, 180);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR(r->lat, -72.0, 1e-5);
        EXPECT_LT(distance_between(LatLng(-90, 0), offset(r.value(), d, 180)), 0.5);
    }

    // The domain-boundary tolerance must not resurrect genuinely unreachable
    // destinations: here sin_arg ≈ 1.005, far beyond FP noise.
    EXPECT_FALSE(offset_origin(LatLng(89, 0), 0.1 * kEarthRadius, 90).has_value());
}
