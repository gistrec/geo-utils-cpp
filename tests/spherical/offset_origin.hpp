#include <gtest/gtest.h>

#include <geo/spherical.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::offset;
using geo::offset_origin;
using geo::detail::kEarthRadius;
using geo::detail::kPi;
using geo::detail::rad2deg;

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
}

TEST(Spherical, offset_origin_lng_normalized) {
    // The raw origin longitude (to.lng - atan2(...)) spans up to ±360° when
    // the origin lies across the antimeridian from the destination; it must
    // come back wrapped into [-180, 180).
    const double d = 3.0e6;  // 3000 km ≈ 26.98° of arc at the equator
    const double d_deg = rad2deg(d / kEarthRadius);

    // Travelling east to (0,-170): the origin lies west across the
    // antimeridian, raw lng ≈ -196.98 → wrapped ≈ +163.02.
    {
        auto r = offset_origin(LatLng(0, -170), d, 90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR(r->lat, 0.0, 1e-9);
        EXPECT_NEAR(r->lng, -170.0 - d_deg + 360.0, 1e-9);
        // And the wrapped origin still maps back onto the destination.
        EXPECT_NEAR_LatLng(LatLng(0, -170), offset(r.value(), d, 90));
    }

    // Travelling west to (0,170): the origin lies east across the
    // antimeridian, raw lng ≈ +196.98 → wrapped ≈ -163.02.
    {
        auto r = offset_origin(LatLng(0, 170), d, -90);
        ASSERT_TRUE(r.has_value());
        EXPECT_NEAR(r->lat, 0.0, 1e-9);
        EXPECT_NEAR(r->lng, 170.0 + d_deg - 360.0, 1e-9);
    }
}
