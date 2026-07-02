#include <gtest/gtest.h>

#include <limits>

#include <geo/latlng.hpp>
#include <geo/spherical.hpp>

using geo::LatLng;

// is_valid must be constexpr — usable in constant-evaluation contexts.
static_assert( LatLng(0.0, 0.0).is_valid());
static_assert(!LatLng(91.0, 0.0).is_valid());

TEST(LatLng, is_valid) {
    // Interior and boundary values are valid; both ±90 and ±180 are inclusive.
    EXPECT_TRUE(LatLng(  0,    0).is_valid());
    EXPECT_TRUE(LatLng( 90,    0).is_valid());
    EXPECT_TRUE(LatLng(-90,    0).is_valid());
    EXPECT_TRUE(LatLng(  0,  180).is_valid());
    EXPECT_TRUE(LatLng(  0, -180).is_valid());
    EXPECT_TRUE(LatLng( 90,  180).is_valid());
    EXPECT_TRUE(LatLng(-90, -180).is_valid());
    EXPECT_TRUE(LatLng( 40.7128, -74.0060).is_valid());

    // Out-of-range latitude or longitude.
    EXPECT_FALSE(LatLng(  90.0001,    0).is_valid());
    EXPECT_FALSE(LatLng( -90.0001,    0).is_valid());
    EXPECT_FALSE(LatLng( 180,       180).is_valid());
    EXPECT_FALSE(LatLng(   0,  180.0001).is_valid());
    EXPECT_FALSE(LatLng(   0, -180.0001).is_valid());
    EXPECT_FALSE(LatLng(   0,       360).is_valid());

    // Non-finite components are invalid.
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(LatLng(nan,    0).is_valid());
    EXPECT_FALSE(LatLng(  0,  nan).is_valid());
    EXPECT_FALSE(LatLng(inf,    0).is_valid());
    EXPECT_FALSE(LatLng(  0, -inf).is_valid());
}

TEST(LatLng, out_of_range_is_stored_as_given) {
    // The constructor performs no validation, clamping, or wrapping:
    // out-of-range input is stored exactly and nothing throws.
    LatLng weird(180.0, 180.0);
    EXPECT_EQ(weird.lat, 180.0);
    EXPECT_EQ(weird.lng, 180.0);
    EXPECT_FALSE(weird.is_valid());

    // Downstream math interprets such values through spherical trigonometry:
    // latitude 180° is "wrapped over the pole", so (180, 180) points in the
    // same direction as (0, 0) on the sphere...
    EXPECT_NEAR(geo::distance_between(weird, LatLng(0, 0)), 0.0, 1e-3);

    // ...yet compares unequal to it. Results for invalid coordinates are
    // unspecified and must not be relied upon — check is_valid() first.
    EXPECT_FALSE(weird == LatLng(0, 0));
}
