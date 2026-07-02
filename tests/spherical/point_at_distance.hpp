#include <gtest/gtest.h>
#include <vector>

#include <geo/spherical.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::distance_between;
using geo::path_length;
using geo::point_at_distance;

TEST(Spherical, point_at_distance) {
    // Empty path: no point.
    EXPECT_FALSE(point_at_distance(std::vector<LatLng>{}, 100.0).has_value());

    // Single point: always that point, whatever the distance.
    std::vector<LatLng> one = { {45, 45} };
    EXPECT_EQ(point_at_distance(one, 0.0)->lat, 45.0);
    EXPECT_EQ(point_at_distance(one, 1e7)->lng, 45.0);

    // Along the equator: distance maps linearly to longitude.
    std::vector<LatLng> equator = { {0, 0}, {0, 10} };
    const double len = path_length(equator);
    EXPECT_NEAR_LatLng(LatLng(0, 2.5), *point_at_distance(equator, len * 0.25));
    EXPECT_NEAR_LatLng(LatLng(0, 5.0), *point_at_distance(equator, len * 0.5));

    // Clamping: <= 0 returns the first vertex, >= length the last — both
    // with the original coordinates bit-exact.
    EXPECT_EQ(point_at_distance(equator, 0.0)->lng, 0.0);
    EXPECT_EQ(point_at_distance(equator, -5.0)->lng, 0.0);
    EXPECT_EQ(point_at_distance(equator, len * 2)->lng, 10.0);

    // Multi-segment route: distances land on the right leg.
    std::vector<LatLng> route = { {0, 0}, {0, 10}, {10, 10} };
    const double leg0 = distance_between(route[0], route[1]);
    EXPECT_NEAR_LatLng(route[1], *point_at_distance(route, leg0));
    auto second_leg = *point_at_distance(route, leg0 + distance_between(route[1], route[2]) / 2);
    EXPECT_NEAR(second_leg.lng, 10.0, 1e-9);
    EXPECT_NEAR(second_leg.lat, 5.0, 0.01);

    // The full length reaches the last vertex.
    EXPECT_NEAR_LatLng(route[2], *point_at_distance(route, path_length(route)));

    // Repeated vertices (zero-length segments) are skipped safely.
    std::vector<LatLng> repeated = { {0, 0}, {0, 0}, {0, 10} };
    EXPECT_NEAR_LatLng(LatLng(0, 5), *point_at_distance(repeated, path_length(repeated) / 2));

    // Across the antimeridian: the midpoint of a 170 -> -170 hop is lng 180.
    std::vector<LatLng> am = { {0, 170}, {0, -170} };
    EXPECT_NEAR_LatLng(LatLng(0, 180), *point_at_distance(am, path_length(am) / 2));
}
