#include <gtest/gtest.h>
#include <vector>

#include <geo/poly.hpp>

using geo::LatLng;
using geo::is_closed_polygon;

TEST(Poly, is_closed_polygon) {
    // Empty path is not a polygon.
    EXPECT_FALSE(is_closed_polygon(std::vector<LatLng>{}));

    // A single point is its own first and last point.
    EXPECT_TRUE(is_closed_polygon(std::vector<LatLng>{{28.06025, -82.41030}}));

    // Open polygon from the upstream test suite.
    std::vector<LatLng> poly = {
        {28.06025, -82.41030}, {28.06129, -82.40945}, {28.06206, -82.40917},
        {28.06125, -82.40850}, {28.06035, -82.40834},
    };
    EXPECT_FALSE(is_closed_polygon(poly));

    // Closing the polygon by repeating the first point.
    poly.push_back(poly.front());
    EXPECT_TRUE(is_closed_polygon(poly));
}

TEST(Poly, is_closed_polygon_antimeridian) {
    // 180 and -180 are the same meridian: longitudes compare modulo 360.
    std::vector<LatLng> poly = {{10, 180}, {20, 90}, {10, -180}};
    EXPECT_TRUE(is_closed_polygon(poly));
}
