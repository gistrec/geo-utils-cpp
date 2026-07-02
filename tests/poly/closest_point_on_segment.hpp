#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include <vector>

#include <geo/poly.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::closest_point_on_segment;
using geo::distance_between;
using geo::distance_to_segment;

TEST(Poly, closest_point_on_segment) {
    const LatLng a(0, 0);
    const LatLng b(0, 90);

    // A point on the segment maps to itself.
    EXPECT_NEAR_LatLng(LatLng(0, 45), closest_point_on_segment(LatLng(0, 45), a, b));

    // Perpendicular foot: on the equator the closest point keeps the
    // longitude, from either side.
    EXPECT_NEAR_LatLng(LatLng(0, 30), closest_point_on_segment(LatLng( 20, 30), a, b));
    EXPECT_NEAR_LatLng(LatLng(0, 30), closest_point_on_segment(LatLng(-20, 30), a, b));

    // Projection falls outside the arc: the nearer endpoint is returned,
    // with its coordinates bit-exact.
    LatLng before = closest_point_on_segment(LatLng(10, -20), a, b);
    EXPECT_EQ(before.lat, a.lat);
    EXPECT_EQ(before.lng, a.lng);
    LatLng after = closest_point_on_segment(LatLng(-10, 100), a, b);
    EXPECT_EQ(after.lat, b.lat);
    EXPECT_EQ(after.lng, b.lng);

    // Degenerate segment: start == end yields start.
    LatLng same = closest_point_on_segment(LatLng(0, 0), LatLng(45, 45), LatLng(45, 45));
    EXPECT_EQ(same.lat, 45.0);
    EXPECT_EQ(same.lng, 45.0);

    // Antipodal endpoints: no unique great circle — the nearer endpoint.
    LatLng anti = closest_point_on_segment(LatLng(10, 20), LatLng(0, 0), LatLng(0, 180));
    EXPECT_EQ(anti.lat, 0.0);
    EXPECT_EQ(anti.lng, 0.0);

    // Point at a pole of the segment's great circle: the whole circle is a
    // quarter-circumference away, so an endpoint is a correct answer.
    LatLng pole = closest_point_on_segment(LatLng(90, 0), a, b);
    EXPECT_EQ(pole.lat, a.lat);
    EXPECT_EQ(pole.lng, a.lng);
    EXPECT_NEAR(distance_between(LatLng(90, 0), pole),
                geo::detail::kPi / 2 * geo::detail::kEarthRadius, 1e-3);
}

TEST(Poly, closest_point_on_segment_high_latitude) {
    // The great circle between two points at latitude 80 bulges poleward:
    // its vertex latitude is atan(tan(80°) / cos(45°)) ≈ 82.89°, not 80.
    // The planar distance_to_segment misses this entirely (it projects in
    // raw lat/lng space and snaps to an endpoint here).
    const LatLng a(80, 0);
    const LatLng b(80, 90);
    const LatLng pole(90, 0);

    LatLng c = closest_point_on_segment(pole, a, b);
    double peak_lat = geo::detail::rad2deg(
        std::atan(std::tan(geo::detail::deg2rad(80.0)) / std::cos(geo::detail::deg2rad(45.0))));
    EXPECT_NEAR(c.lat, peak_lat, 1e-9);  // ≈ 82.8935
    EXPECT_NEAR(c.lng, 45.0, 1e-9);

    // Geodesic distance ≈ 790 km; the planar approximation reports ≈ 1112 km.
    double geodesic = distance_between(pole, c);
    EXPECT_NEAR(geodesic, geo::detail::deg2rad(90.0 - peak_lat) * geo::detail::kEarthRadius, 1e-6);
    EXPECT_LT(geodesic, distance_to_segment(pole, a, b) - 300'000.0);
}

TEST(Poly, closest_point_on_segment_antimeridian) {
    // Segment crossing the antimeridian — meaningless for the planar
    // distance_to_segment, exact here: the foot keeps the longitude.
    LatLng c = closest_point_on_segment(LatLng(5, 180), LatLng(0, 170), LatLng(0, -170));
    EXPECT_NEAR_LatLng(LatLng(0, 180), c);
    EXPECT_NEAR(distance_between(LatLng(5, 180), c),
                geo::detail::deg2rad(5.0) * geo::detail::kEarthRadius, 1e-3);

    // Both endpoints on one side, point on the other.
    LatLng d = closest_point_on_segment(LatLng(0, -179), LatLng(10, 179), LatLng(-10, 179));
    EXPECT_NEAR_LatLng(LatLng(0, 179), d);
}

TEST(Poly, closest_point_on_segment_brute_force) {
    // Property test with a fixed seed: for random segments and points, the
    // returned point must lie on the segment, and no densely sampled point
    // of the arc may be meaningfully closer.
    std::mt19937 rng(20260702u);
    std::uniform_real_distribution<double> rand_lat(-90.0, 90.0);
    std::uniform_real_distribution<double> rand_lng(-180.0, 180.0);
    const int kSamples = 1000;

    for (int trial = 0; trial < 150; ++trial) {
        const LatLng a(rand_lat(rng), rand_lng(rng));
        const LatLng b(rand_lat(rng), rand_lng(rng));
        const LatLng p(rand_lat(rng), rand_lng(rng));
        // Skip near-antipodal segments: the great circle is not unique and
        // the nearer-endpoint convention would not match arc sampling.
        if (geo::angle_between(a, b) > geo::detail::kPi - 1e-3) {
            continue;
        }

        const LatLng c = closest_point_on_segment(p, a, b);
        const double d = distance_between(p, c);

        EXPECT_TRUE(geo::on_path(c, std::vector<LatLng>{a, b}, true, 1e-2))
            << "trial " << trial << ": " << c << " not on " << a << " .. " << b;

        double sampled = std::numeric_limits<double>::infinity();
        for (int k = 0; k <= kSamples; ++k) {
            sampled = std::min(sampled,
                distance_between(p, geo::interpolate(a, b, static_cast<double>(k) / kSamples)));
        }
        EXPECT_LE(d, sampled + 1e-3)
            << "trial " << trial << ": " << p << " -> " << c << " misses a closer arc point";
    }
}
