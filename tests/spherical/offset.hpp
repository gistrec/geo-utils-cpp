#include <gtest/gtest.h>

#include <geo/spherical.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::distance_between;
using geo::offset;
using geo::detail::deg2rad;
using geo::detail::kEarthRadius;
using geo::detail::kPi;
using geo::detail::rad2deg;

TEST(Spherical, offset) {
    LatLng up    = { 90.0,    0.0 };
    LatLng down  = {-90.0,    0.0 };
    LatLng front = {  0.0,    0.0 };
    LatLng right = {  0.0,   90.0 };
    LatLng back  = {  0.0, -180.0 };
    LatLng left  = {  0.0,  -90.0 };

    EXPECT_NEAR_LatLng(front, offset(front, 0, 0));
    EXPECT_NEAR_LatLng(up,    offset(front, kPi * kEarthRadius / 2,   0));
    EXPECT_NEAR_LatLng(down,  offset(front, kPi * kEarthRadius / 2, 180));
    EXPECT_NEAR_LatLng(left,  offset(front, kPi * kEarthRadius / 2, -90));
    EXPECT_NEAR_LatLng(right, offset(front, kPi * kEarthRadius / 2,  90));
    EXPECT_NEAR_LatLng(back,  offset(front, kPi * kEarthRadius,       0));
    EXPECT_NEAR_LatLng(back,  offset(front, kPi * kEarthRadius,      90));

    // From left
    EXPECT_NEAR_LatLng(left,  offset(left, 0, 0));
    EXPECT_NEAR_LatLng(up,    offset(left, kPi * kEarthRadius / 2,   0));
    EXPECT_NEAR_LatLng(down,  offset(left, kPi * kEarthRadius / 2, 180));
    EXPECT_NEAR_LatLng(front, offset(left, kPi * kEarthRadius / 2,  90));
    EXPECT_NEAR_LatLng(back,  offset(left, kPi * kEarthRadius / 2, -90));
    EXPECT_NEAR_LatLng(right, offset(left, kPi * kEarthRadius,       0));
    EXPECT_NEAR_LatLng(right, offset(left, kPi * kEarthRadius,      90));

    // NOTE: Heading is undefined at the poles, so we do not test from up/down.
}

TEST(Spherical, offset_lng_normalized) {
    // The raw result longitude (from_lng + d_lng) spans up to ±360° when the
    // path crosses the antimeridian; offset must wrap it into [-180, 180)
    // (the Android SDK normalized in the LatLng constructor, this port does
    // not, so the function has to wrap its own output).
    const double d = 3.0e6;  // 3000 km ≈ 26.98° of arc at the equator
    const double d_deg = rad2deg(d / kEarthRadius);

    // Eastbound across the antimeridian: raw lng ≈ +196.98 → wrapped ≈ -163.02.
    LatLng east = offset(LatLng(0, 170), d, 90);
    EXPECT_NEAR(east.lat, 0.0, 1e-9);
    EXPECT_NEAR(east.lng, 170.0 + d_deg - 360.0, 1e-9);

    // Westbound across the antimeridian: raw lng ≈ -196.98 → wrapped ≈ +163.02.
    LatLng west = offset(LatLng(0, -170), d, -90);
    EXPECT_NEAR(west.lat, 0.0, 1e-9);
    EXPECT_NEAR(west.lng, -170.0 - d_deg + 360.0, 1e-9);

    // Landing exactly on the antimeridian: lng must stay inside [-180, 180),
    // i.e. +180 becomes -180 (the same point).
    LatLng on180 = offset(LatLng(0, 170), deg2rad(10.0) * kEarthRadius, 90);
    EXPECT_GE(on180.lng, -180.0);
    EXPECT_LT(on180.lng,  180.0);
    EXPECT_LT(distance_between(on180, LatLng(0, 180)), 1e-3);
}
