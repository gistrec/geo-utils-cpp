#include <gtest/gtest.h>

#include <limits>
#include <vector>

#include <geo/bounds.hpp>
#include <geo/poly.hpp>
#include "../test_helpers.hpp"

using geo::LatLng;
using geo::LatLngBounds;

TEST(Bounds, is_valid) {
    EXPECT_TRUE (LatLngBounds({-10, -20}, {10, 20}).is_valid());
    EXPECT_TRUE (LatLngBounds({-10, 170}, {10, -170}).is_valid());  // crosses AM
    EXPECT_TRUE (LatLngBounds({0, 0}, {0, 0}).is_valid());          // degenerate point
    EXPECT_FALSE(LatLngBounds({10, 0}, {-10, 0}).is_valid());       // lat inverted
    EXPECT_FALSE(LatLngBounds({0, 200}, {10, 0}).is_valid());       // lng out of range
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(LatLngBounds({nan, 0}, {10, 0}).is_valid());
}

TEST(Bounds, contains) {
    LatLngBounds box({-10, -20}, {10, 20});
    EXPECT_TRUE (box.contains({0, 0}));
    EXPECT_TRUE (box.contains({-10, -20}));  // corners inclusive
    EXPECT_TRUE (box.contains({10, 20}));
    EXPECT_TRUE (box.contains({0, 20}));     // edges inclusive
    EXPECT_FALSE(box.contains({10.0001, 0}));
    EXPECT_FALSE(box.contains({0, 20.0001}));
    EXPECT_FALSE(box.contains({0, 180}));

    // Bounds crossing the antimeridian: sw.lng > ne.lng.
    LatLngBounds am({-10, 170}, {10, -170});
    EXPECT_TRUE (am.contains({0, 175}));
    EXPECT_TRUE (am.contains({0, 180}));
    EXPECT_TRUE (am.contains({0, -180}));    // 180 and -180 interchangeable
    EXPECT_TRUE (am.contains({0, -175}));
    EXPECT_FALSE(am.contains({0, 0}));
    EXPECT_FALSE(am.contains({0, 160}));
    EXPECT_FALSE(am.contains({11, 175}));

    // Equal longitudes describe a single meridian, not the whole circle.
    LatLngBounds meridian({-10, 30}, {10, 30});
    EXPECT_TRUE (meridian.contains({0, 30}));
    EXPECT_FALSE(meridian.contains({0, 30.0001}));
    EXPECT_FALSE(meridian.contains({0, 29.9999}));
}

TEST(Bounds, extend) {
    // Points already inside leave the bounds bit-exact.
    LatLngBounds box({-10, -20}, {10, 20});
    box.extend({0, 0});
    EXPECT_EQ(box.southwest.lat, -10.0);
    EXPECT_EQ(box.southwest.lng, -20.0);
    EXPECT_EQ(box.northeast.lat, 10.0);
    EXPECT_EQ(box.northeast.lng, 20.0);

    // Latitude extends independently of longitude.
    box.extend({15, 0});
    EXPECT_EQ(box.northeast.lat, 15.0);
    box.extend({-25, 0});
    EXPECT_EQ(box.southwest.lat, -25.0);

    // Longitude extends toward the nearer side.
    box.extend({0, 30});   // 10 east of ne vs 310 west of sw
    EXPECT_EQ(box.northeast.lng, 30.0);
    box.extend({0, -40});  // 20 west of sw vs 290 east of ne
    EXPECT_EQ(box.southwest.lng, -40.0);

    // Extending across the antimeridian flips sw/ne longitude order.
    LatLngBounds pacific({0, 160}, {10, 175});
    pacific.extend({5, -170});  // 15 east of 175 vs 330 west of 160
    EXPECT_EQ(pacific.northeast.lng, -170.0);
    EXPECT_TRUE(pacific.contains({5, 180}));
    EXPECT_FALSE(pacific.contains({5, 0}));

    // Reaching the 180 meridian eastward stores +180 (not -180), keeping
    // the eastward span minimal.
    LatLngBounds east({0, 90}, {10, 170});
    east.extend({5, -180});
    EXPECT_EQ(east.northeast.lng, 180.0);
    EXPECT_NEAR(east.lng_span(), 90.0, 1e-12);

    // ...and reaching it westward stores -180.
    LatLngBounds west({0, -170}, {10, -90});
    west.extend({5, 180});
    EXPECT_EQ(west.southwest.lng, -180.0);
    EXPECT_NEAR(west.lng_span(), 90.0, 1e-12);
}

TEST(Bounds, center) {
    EXPECT_NEAR_LatLng(LatLng(0, 0), LatLngBounds({-10, -20}, {10, 20}).center());
    EXPECT_NEAR_LatLng(LatLng(5, 15), LatLngBounds({0, 10}, {10, 20}).center());

    // Antimeridian-crossing bounds: the center is on the far side.
    EXPECT_NEAR_LatLng(LatLng(0, 180), LatLngBounds({-10, 170}, {10, -170}).center());
    EXPECT_NEAR_LatLng(LatLng(0, -175), LatLngBounds({-10, 170}, {10, -160}).center());
}

TEST(Bounds, intersects) {
    LatLngBounds box({-10, -20}, {10, 20});
    EXPECT_TRUE (box.intersects(LatLngBounds({0, 0}, {30, 40})));      // overlap
    EXPECT_TRUE (box.intersects(LatLngBounds({-5, -5}, {5, 5})));      // nested
    EXPECT_TRUE (box.intersects(LatLngBounds({10, 20}, {30, 40})));    // touching corner
    EXPECT_FALSE(box.intersects(LatLngBounds({20, 0}, {30, 10})));     // disjoint lat
    EXPECT_FALSE(box.intersects(LatLngBounds({0, 30}, {10, 40})));     // disjoint lng
    EXPECT_FALSE(box.intersects(LatLngBounds({-10, 170}, {10, -170})));

    // Antimeridian-crossing vs plain bounds.
    LatLngBounds am({-10, 170}, {10, -170});
    EXPECT_TRUE (am.intersects(LatLngBounds({0, 175}, {5, 178})));     // nested in AM box
    EXPECT_TRUE (am.intersects(LatLngBounds({0, -175}, {5, -160})));   // overlaps east side
    EXPECT_TRUE (am.intersects(LatLngBounds({0, 100}, {5, 175})));     // overlaps west side
    EXPECT_FALSE(am.intersects(LatLngBounds({0, -100}, {5, 100})));
    EXPECT_TRUE (am.intersects(LatLngBounds({-10, -170}, {10, 170}))); // complement, shares edges
}

TEST(Bounds, bounds_of_path) {
    // Empty path: no bounds.
    EXPECT_FALSE(geo::bounds(std::vector<LatLng>{}).has_value());

    // Single point: degenerate bounds containing exactly that point.
    auto single = geo::bounds(std::vector<LatLng>{ {45, 45} });
    ASSERT_TRUE(single.has_value());
    EXPECT_TRUE(single->contains({45, 45}));
    EXPECT_EQ(single->lng_span(), 0.0);

    // A small polygon: every vertex and interior point is inside; the
    // bounds work as a prefilter for contains().
    std::vector<LatLng> tri = { {0, 0}, {10, 12}, {20, 5} };
    auto tri_bounds = geo::bounds(tri);
    ASSERT_TRUE(tri_bounds.has_value());
    EXPECT_EQ(tri_bounds->southwest.lat, 0.0);
    EXPECT_EQ(tri_bounds->southwest.lng, 0.0);
    EXPECT_EQ(tri_bounds->northeast.lat, 20.0);
    EXPECT_EQ(tri_bounds->northeast.lng, 12.0);
    for (const auto& p : { LatLng(10, 11), LatLng(19, 5), LatLng(1, 1) }) {
        EXPECT_TRUE(geo::contains(p, tri) ? tri_bounds->contains(p) : true);
    }
    EXPECT_FALSE(tri_bounds->contains({30, 5}));   // outside bounds => outside polygon

    // A route crossing the antimeridian gets tight (not world-spanning) bounds.
    auto am = geo::bounds(std::vector<LatLng>{ {10, 170}, {20, -170} });
    ASSERT_TRUE(am.has_value());
    EXPECT_NEAR(am->lng_span(), 20.0, 1e-12);
    EXPECT_TRUE(am->contains({15, 180}));
    EXPECT_FALSE(am->contains({15, 0}));
}
