#include <gtest/gtest.h>

#include <geo/detail/math.hpp>

using namespace geo::detail;

TEST(Math, mod_positive) {
    EXPECT_NEAR(mod(7.0, 3.0),  1.0, 1e-10);
    EXPECT_NEAR(mod(6.0, 3.0),  0.0, 1e-10);
    EXPECT_NEAR(mod(0.5, 1.0),  0.5, 1e-10);
}

TEST(Math, mod_negative_x) {
    // Must return non-negative result
    EXPECT_NEAR(mod(-1.0,  3.0), 2.0, 1e-10);
    EXPECT_NEAR(mod(-3.0,  3.0), 0.0, 1e-10);
    EXPECT_NEAR(mod(-7.0,  3.0), 2.0, 1e-10);
}

TEST(Math, mod_wrap_longitude) {
    // Typical use: wrapping longitude into [0, 360)
    EXPECT_NEAR(mod(360.0, 360.0),   0.0, 1e-10);
    EXPECT_NEAR(mod(370.0, 360.0),  10.0, 1e-10);
    EXPECT_NEAR(mod(-10.0, 360.0), 350.0, 1e-10);
}

TEST(Math, wrap) {
    // wrap uses mod internally — verify it returns correct results for negative input
    EXPECT_NEAR(wrap(-181.0, -180.0, 180.0), 179.0, 1e-10);
    EXPECT_NEAR(wrap( 181.0, -180.0, 180.0),-179.0, 1e-10);
    EXPECT_NEAR(wrap(   0.0, -180.0, 180.0),   0.0, 1e-10);
    EXPECT_NEAR(wrap(-180.0, -180.0, 180.0),-180.0, 1e-10);
}

TEST(Math, mercator_inverse_mercator) {
    // Equator maps to 0
    EXPECT_NEAR(mercator(0.0),         0.0, 1e-10);
    EXPECT_NEAR(inverse_mercator(0.0), 0.0, 1e-10);

    // Round-trip: inverse_mercator(mercator(lat)) == lat
    EXPECT_NEAR(inverse_mercator(mercator(deg2rad( 45.0))), deg2rad( 45.0), 1e-10);
    EXPECT_NEAR(inverse_mercator(mercator(deg2rad(-30.0))), deg2rad(-30.0), 1e-10);
}

TEST(Math, hav_arc_hav) {
    EXPECT_NEAR(hav(0),                    0.0, 1e-10);
    EXPECT_NEAR(hav(kPi / 2), 0.5, 1e-10);
    EXPECT_NEAR(hav(kPi),     1.0, 1e-10);

    EXPECT_NEAR(arc_hav(0),   0.0,                  1e-10);
    EXPECT_NEAR(arc_hav(0.5), kPi / 2, 1e-10);
    EXPECT_NEAR(arc_hav(1),   kPi,     1e-10);

    // Round-trip: arc_hav(hav(x)) == x for x in [0, π]
    EXPECT_NEAR(arc_hav(hav(1.2)), 1.2, 1e-10);
}

TEST(Math, arc_hav_clamp) {
    // Floating-point rounding can push the haversine value slightly outside [0,1].
    // arc_hav must clamp rather than pass a bad value to sqrt/asin and return NaN.
    EXPECT_NEAR(arc_hav( 1.0 + 1e-15),  kPi, 1e-9); // x > 1 → clamp to π
    EXPECT_NEAR(arc_hav(-1e-15),        0.0,              1e-9); // x < 0 → clamp to 0
}

TEST(Math, sin_from_hav) {
    EXPECT_NEAR(sin_from_hav(0.0), 0.0, 1e-10); // sin(0) == 0
    EXPECT_NEAR(sin_from_hav(0.5), 1.0, 1e-10); // sin(π/2) == 1
    EXPECT_NEAR(sin_from_hav(1.0), 0.0, 1e-10); // sin(π) == 0
}

TEST(Math, hav_from_sin) {
    EXPECT_NEAR(hav_from_sin(0.0), 0.0, 1e-10); // hav(asin(0)) == 0
    EXPECT_NEAR(hav_from_sin(1.0), 0.5, 1e-10); // hav(π/2) == 0.5

    // Round-trip: sin_from_hav(hav_from_sin(x)) == x for x in [0, 1]
    EXPECT_NEAR(sin_from_hav(hav_from_sin(0.7)), 0.7, 1e-10);
}

TEST(Math, hav_from_sin_clamp) {
    // Floating-point rounding can push |x| slightly above 1 (e.g. the
    // sin_delta_bearing quotient in poly.hpp). hav_from_sin must clamp rather
    // than feed a negative value to sqrt and return NaN — a NaN cross-track
    // silently turns is_on_segment_gc into a false positive.
    EXPECT_NEAR(hav_from_sin(std::nextafter( 1.0,  2.0)), 0.5, 1e-10); // x > 1  → hav(π/2)
    EXPECT_NEAR(hav_from_sin(std::nextafter(-1.0, -2.0)), 0.5, 1e-10); // x < -1 → hav(π/2)
}

TEST(Math, sin_sum_from_hav) {
    // sin(0 + 0) == 0
    EXPECT_NEAR(sin_sum_from_hav(0.0, 0.0), 0.0, 1e-10);
    // sin(π/2 + 0) == 1
    EXPECT_NEAR(sin_sum_from_hav(0.5, 0.0), 1.0, 1e-10);
    // sin(π/3 + π/3) == sin(2π/3) == √3/2
    EXPECT_NEAR(sin_sum_from_hav(0.25, 0.25), sqrt(3.0) / 2.0, 1e-10);
}

TEST(Math, hav_distance) {
    // Distance from a point to itself is 0
    EXPECT_NEAR(hav_distance(0, 0, 0), 0.0, 1e-10);

    // front(0,0) → right(0,90°): dLng = π/2, hav(0) + hav(π/2)*1*1 = 0.5
    EXPECT_NEAR(hav_distance(0, 0, kPi / 2), 0.5, 1e-10);

    // front(0,0) → up(90°,0): hav(π/2) + 0 = 0.5
    EXPECT_NEAR(hav_distance(0, kPi / 2, 0), 0.5, 1e-10);
}
