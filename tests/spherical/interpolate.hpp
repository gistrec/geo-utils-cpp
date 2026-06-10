#include <gtest/gtest.h>
#include <vector>

#include <geo/spherical.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::interpolate;
using geo::distance_between;

TEST(Spherical, interpolate) {
    LatLng up    = { 90.0,    0.0 };
    LatLng down  = {-90.0,    0.0 };
    LatLng front = {  0.0,    0.0 };
    LatLng back  = {  0.0, -180.0 };
    LatLng left  = {  0.0,  -90.0 };

    EXPECT_NEAR_LatLng(up,   interpolate(up,     up, 1 / 2.0));
    EXPECT_NEAR_LatLng(down, interpolate(down, down, 1 / 2.0));
    EXPECT_NEAR_LatLng(left, interpolate(left, left, 1 / 2.0));

    // Between front and up
    EXPECT_NEAR_LatLng(LatLng(1, 0),  interpolate(front, up,  1 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(1, 0),  interpolate(up, front, 89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(89, 0), interpolate(front, up, 89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(89, 0), interpolate(up, front,  1 / 90.0));

    // Between front and down
    EXPECT_NEAR_LatLng(LatLng(-1, 0),  interpolate(front, down,  1 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(-1, 0),  interpolate(down, front, 89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(-89, 0), interpolate(front, down, 89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(-89, 0), interpolate(down, front,  1 / 90.0));

    // Between left and back
    EXPECT_NEAR_LatLng(LatLng(0, -91),  interpolate(left, back,   1 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(0, -91),  interpolate(back, left,  89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(0, -179), interpolate(left, back, 89 / 90.0));
    EXPECT_NEAR_LatLng(LatLng(0, -179), interpolate(back, left,  1 / 90.0));

    // geodesic crosses pole
    EXPECT_NEAR_LatLng(up,   interpolate(LatLng(45, 0),  LatLng(45, 180),  1 / 2.0));
    EXPECT_NEAR_LatLng(down, interpolate(LatLng(-45, 0), LatLng(-45, 180), 1 / 2.0));

    // boundary values for fraction, between left and back
    EXPECT_NEAR_LatLng(left, interpolate(left, back, 0.0));
    EXPECT_NEAR_LatLng(back, interpolate(left, back, 1.0));

    // two nearby points, separated by ~4m, for which the Slerp algorithm is not stable and we
    // have to fall back to linear interpolation.
    LatLng nearA(-37.756891, 175.325262);
    LatLng nearB(-37.756853, 175.325242);

    LatLng interpolateResult = interpolate(nearA, nearB, 0.5);
    LatLng goldenResult(-37.756872, 175.325252);
    EXPECT_NEAR(interpolateResult.lat, goldenResult.lat, 1e-6);
    EXPECT_NEAR(interpolateResult.lng, goldenResult.lng, 1e-6);

    // fraction=1.0 must return 'to', not 'from'
    LatLng endResult = interpolate(nearA, nearB, 1.0);
    EXPECT_NEAR(endResult.lat, nearB.lat, 1e-6);
    EXPECT_NEAR(endResult.lng, nearB.lng, 1e-6);

    // Regression: two nearby points straddling the antimeridian (~22cm apart).
    // The linear fallback must wrap the longitude difference; before the fix
    // the midpoint came out near lng=0 — ~20,000 km away from both points.
    // Assert via distances to stay agnostic about the ±180 representation.
    LatLng amA(0.0,  179.999999);
    LatLng amB(0.0, -179.999999);
    LatLng amMid = interpolate(amA, amB, 0.5);
    EXPECT_NEAR(amMid.lat, 0.0, 1e-9);
    EXPECT_LT(distance_between(amA, amMid), 0.15);  // ~0.11 m expected
    EXPECT_LT(distance_between(amB, amMid), 0.15);
}
