#include <gtest/gtest.h>
#include <vector>

#include <geo/poly.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::closest_point_on_path;
using geo::distance_between;

TEST(Poly, closest_point_on_path) {
    // Empty path: no closest point.
    std::vector<LatLng> empty;
    EXPECT_FALSE(closest_point_on_path(LatLng(0, 0), empty).has_value());

    // Single point: the point itself, segment index 0.
    std::vector<LatLng> one = { {45, 45} };
    auto r1 = closest_point_on_path(LatLng(0, 0), one);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->point.lat, 45.0);
    EXPECT_EQ(r1->point.lng, 45.0);
    EXPECT_EQ(r1->segment, 0U);
    EXPECT_DOUBLE_EQ(r1->distance, distance_between(LatLng(0, 0), r1->point));

    // An L-shaped route: equator leg, then a meridian leg.
    std::vector<LatLng> route = { {0, 0}, {0, 10}, {10, 10} };

    // Clearly nearest to the first leg.
    auto r2 = closest_point_on_path(LatLng(2, 5), route);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->segment, 0U);
    EXPECT_NEAR_LatLng(LatLng(0, 5), r2->point);
    EXPECT_DOUBLE_EQ(r2->distance, distance_between(LatLng(2, 5), r2->point));

    // Clearly nearest to the second leg: the foot keeps ~the latitude.
    auto r3 = closest_point_on_path(LatLng(8, 9.5), route);
    ASSERT_TRUE(r3.has_value());
    EXPECT_EQ(r3->segment, 1U);
    EXPECT_NEAR(r3->point.lng, 10.0, 1e-9);
    EXPECT_NEAR(r3->point.lat, 8.0, 0.01);
    EXPECT_DOUBLE_EQ(r3->distance, distance_between(LatLng(8, 9.5), r3->point));

    // A point of the route itself: distance ~0, first matching segment.
    auto r4 = closest_point_on_path(geo::interpolate(route[0], route[1], 0.3), route);
    ASSERT_TRUE(r4.has_value());
    EXPECT_EQ(r4->segment, 0U);
    EXPECT_LT(r4->distance, 1e-6);
}

TEST(Poly, closest_point_on_path_shared_vertex_tie) {
    // Both legs are closest at their shared vertex: the tie goes to the
    // lower segment index, and the vertex coordinates come back bit-exact.
    std::vector<LatLng> route = { {0, 0}, {0, 10}, {10, 10} };
    auto r = closest_point_on_path(LatLng(-5, 15), route);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->segment, 0U);
    EXPECT_EQ(r->point.lat, 0.0);
    EXPECT_EQ(r->point.lng, 10.0);
    EXPECT_DOUBLE_EQ(r->distance, distance_between(LatLng(-5, 15), r->point));
}

TEST(Poly, closest_point_on_path_antimeridian) {
    // Route crossing the antimeridian; the probe sits right on lng 180.
    std::vector<LatLng> route = { {0, 170}, {0, -170}, {5, -170} };
    auto r = closest_point_on_path(LatLng(1, 180), route);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->segment, 0U);
    EXPECT_NEAR_LatLng(LatLng(0, 180), r->point);
    EXPECT_NEAR(r->distance, geo::detail::deg2rad(1.0) * geo::detail::kEarthRadius, 1e-3);
}
