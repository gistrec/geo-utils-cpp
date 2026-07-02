#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include <geo/latlng.hpp>

using geo::LatLng;

TEST(LatLng, normalized) {
    // In-range coordinates pass through bit-exactly.
    LatLng nyc = LatLng(40.7128, -74.0060).normalized();
    EXPECT_EQ(nyc.lat, 40.7128);
    EXPECT_EQ(nyc.lng, -74.0060);
    LatLng sw = LatLng(-90, -180).normalized();
    EXPECT_EQ(sw.lat, -90.0);
    EXPECT_EQ(sw.lng, -180.0);

    // Longitude 180 wraps to -180 (same meridian, canonical form).
    EXPECT_EQ(LatLng(0, 180).normalized().lng, -180.0);

    // Latitude clamps.
    EXPECT_EQ(LatLng(90.0001, 0).normalized().lat, 90.0);
    EXPECT_EQ(LatLng(-90.0001, 0).normalized().lat, -90.0);
    EXPECT_EQ(LatLng(180, 0).normalized().lat, 90.0);

    // Longitude wraps by full turns.
    EXPECT_DOUBLE_EQ(LatLng(0, 185).normalized().lng, -175.0);
    EXPECT_DOUBLE_EQ(LatLng(0, -185).normalized().lng, 175.0);
    EXPECT_DOUBLE_EQ(LatLng(0, 200).normalized().lng, -160.0);
    EXPECT_DOUBLE_EQ(LatLng(0, -200).normalized().lng, 160.0);
    EXPECT_DOUBLE_EQ(LatLng(0, 360).normalized().lng, 0.0);
    EXPECT_DOUBLE_EQ(LatLng(0, 540).normalized().lng, -180.0);
    EXPECT_DOUBLE_EQ(LatLng(0, -540).normalized().lng, -180.0);

    // Any finite input normalizes to a valid coordinate.
    EXPECT_TRUE(LatLng(1234.5, -6789.0).normalized().is_valid());
    EXPECT_TRUE(LatLng(-1e15, 1e15).normalized().is_valid());

    // Infinite latitude clamps; infinite longitude has no meaningful wrap.
    constexpr double inf = std::numeric_limits<double>::infinity();
    EXPECT_EQ(LatLng(inf, 0).normalized().lat, 90.0);
    EXPECT_EQ(LatLng(-inf, 0).normalized().lat, -90.0);
    EXPECT_TRUE(std::isnan(LatLng(0, inf).normalized().lng));

    // NaN propagates: normalizing garbage does not invent a coordinate.
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_TRUE(std::isnan(LatLng(nan, 0).normalized().lat));
    EXPECT_TRUE(std::isnan(LatLng(0, nan).normalized().lng));
    EXPECT_FALSE(LatLng(nan, nan).normalized().is_valid());
}
