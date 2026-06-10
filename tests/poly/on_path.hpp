#include <gtest/gtest.h>
#include <vector>

#include <geo/poly.hpp>

using geo::LatLng;
using geo::on_path;

TEST(Poly, on_path) {
    // Empty
    std::vector<LatLng> empty;
    EXPECT_FALSE(on_path(LatLng(0, 0), empty,  true));
    EXPECT_FALSE(on_path(LatLng(0, 0), empty, false));

    // One point.
    std::vector<LatLng> one = { {1, 2} };
    EXPECT_TRUE(on_path(LatLng(1, 2), one,  true));
    EXPECT_TRUE(on_path(LatLng(1, 2), one, false));

    EXPECT_FALSE(on_path(LatLng(3, 5), one,  true));
    EXPECT_FALSE(on_path(LatLng(3, 5), one, false));

    // Endpoints
    std::vector<LatLng> endpoints = { {1, 2}, {3, 5} };
    for (const auto & point : { LatLng(1, 2), LatLng(3, 5) }) {
        EXPECT_TRUE(on_path(point, endpoints,  true));
        EXPECT_TRUE(on_path(point, endpoints, false));
    }
    EXPECT_FALSE(on_path(LatLng(0, 0), endpoints,  true));
    EXPECT_FALSE(on_path(LatLng(0, 0), endpoints, false));

    double small = 5e-7; // About 5cm on equator, half the default tolerance.
    double big   = 2e-6; // About 10cm on equator, double the default tolerance.

    // On equator.
    std::vector<LatLng> equator = { {0, 90}, {0, 180} };
    for (const auto & point : { LatLng(0, 90-small), LatLng(0, 90+small), LatLng(0-small, 90), LatLng(0, 135), LatLng(small, 135) }) {
        EXPECT_TRUE(on_path(point, equator,  true));
    }
    for (const auto & point : { LatLng(0, 90 - big), LatLng(0, 0), LatLng(0, -90), LatLng(big, 135) }) {
        EXPECT_FALSE(on_path(point, equator, false));
    }

    // Ends on same latitude.
    std::vector<LatLng> sameLatitude = { {-45, -180}, {-45, -small} };
    for (const auto & point : { LatLng(-45, 180+small), LatLng(-45, 180-small), LatLng(-45-small, 180-small), LatLng(-45, 0) }) {
        EXPECT_TRUE(on_path(point, sameLatitude,  true));
    }
    for (const auto & point : { LatLng(-45, big), LatLng(-45, 180-big), LatLng(-45+big, -90), LatLng(-45, 90) }) {
        EXPECT_FALSE(on_path(point, sameLatitude, false));
    }

    // Meridian.
    std::vector<LatLng> meridian = { {-10, 30}, {45, 30} };
    for (const auto & point : { LatLng(10, 30 - small), LatLng(20, 30 + small), LatLng(-10 - small, 30 + small) }) {
        EXPECT_TRUE(on_path(point, meridian,  true));
    }
    for (const auto & point : { LatLng(-10 - big, 30), LatLng(10, -150), LatLng(0, 30 - big) }) {
        EXPECT_FALSE(on_path(point, meridian, false));
    }

    // Slanted close to meridian, close to North pole.
    std::vector<LatLng> northPole = { {0, 0}, {90 - small, 0 + big} };
    for (const auto & point : { LatLng(1, 0 + small), LatLng(2, 0 - small), LatLng(90 - small, -90), LatLng(90 - small, 10) }) {
        EXPECT_TRUE(on_path(point, northPole, true));
    }
    for (const auto & point : { LatLng(-big, 0), LatLng(90 - big, 180), LatLng(10, big) }) {
        EXPECT_FALSE(on_path(point, northPole, false));
    }

    // Arc > 120 deg.
    std::vector<LatLng> poly = { {0, 0}, {0, 179.999} };
    for (const auto & point : { LatLng(0, 90), LatLng(0, small), LatLng(0, 179), LatLng(small, 90) }) {
        EXPECT_TRUE(on_path(point, poly, true));
    }
    for (const auto & point : { LatLng(0, -90), LatLng(small, -100), LatLng(0, 180), LatLng(0, -big), LatLng(90, 0), LatLng(-90, 180) }) {
        EXPECT_FALSE(on_path(point, poly, false));
    }

    std::vector<LatLng> poly2 = { {10, 5}, {30, 15} };
    for (const auto & point : { LatLng(10+2*big, 5+big), LatLng(10+big, 5+big/2), LatLng(30-2*big, 15-big) }) {
        EXPECT_TRUE(on_path(point, poly2, true));
    }
    for (const auto & point : { LatLng(20, 10), LatLng(10-big, 5-big/2), LatLng(30+2*big, 15+big), LatLng(10+2*big, 5), LatLng(10, 5+big) }) {
        EXPECT_FALSE(on_path(point, poly2, false));
    }

    std::vector<LatLng> poly3 = { {90 - small, 0}, {0, 180 - small / 2} };
    for (const auto & point : { LatLng(big, -180 + small / 2), LatLng(big, 180 - small / 4), LatLng(big, 180 - small) }) {
        EXPECT_TRUE(on_path(point, poly3, true));
    }
    for (const auto & point : { LatLng(-big, -180 + small / 2), LatLng(-big, 180), LatLng(-big, 180 - small) }) {
        EXPECT_FALSE(on_path(point, poly3, false));
    }

    // Reaching close to North pole.
    std::vector<LatLng> closeToNorthPole = { {80, 0}, {80, 180 - small} };

    for (const auto & point : { LatLng(90 - small, -90), LatLng(90, -135), LatLng(80 - small, 0), LatLng(80 + small, 0) }) {
        EXPECT_TRUE(on_path(point, closeToNorthPole, true));
    }
    for (const auto & point : { LatLng(80, 90), LatLng(79, big) }) {
        EXPECT_FALSE(on_path(point, closeToNorthPole, true));
    }

    for (const auto & point : { LatLng(79, big), LatLng(90 - small, -90), LatLng(90, -135) }) {
        EXPECT_FALSE(on_path(point, closeToNorthPole, false));
    }
}

TEST(Poly, on_path_closing_segment_excluded) {
    // Same triangle as in on_edge_closing_segment: (0,10) lies only on the
    // implicit closing segment (0,20)->(0,0), which on_path must NOT include
    // — that is the only difference between on_path and on_edge.
    std::vector<LatLng> triangle = { {0, 0}, {5, 10}, {0, 20} };
    EXPECT_FALSE(on_path(LatLng(0, 10), triangle,  true));
    EXPECT_FALSE(on_path(LatLng(0, 10), triangle, false));

    // Points on real (non-closing) segments are still found.
    EXPECT_TRUE(on_path(LatLng(0, 0),  triangle,  true));
    EXPECT_TRUE(on_path(LatLng(0, 20), triangle, false));
}

TEST(Poly, on_path_repeated_vertex) {
    // A zero-length segment (repeated vertex) must neither crash nor match
    // far-away points (exercises the denom <= 0 branch of sin_delta_bearing).
    std::vector<LatLng> repeated = { {0, 0}, {0, 5}, {0, 5}, {0, 10} };
    for (const auto & point : { LatLng(0, 2.5), LatLng(0, 5), LatLng(0, 7.5) }) {
        EXPECT_TRUE(on_path(point, repeated,  true));
        EXPECT_TRUE(on_path(point, repeated, false));
    }
    EXPECT_FALSE(on_path(LatLng(1, 5), repeated,  true));
    EXPECT_FALSE(on_path(LatLng(1, 5), repeated, false));

    // Degenerate polyline of two identical points behaves like a single point.
    std::vector<LatLng> dup = { {7, 3}, {7, 3} };
    EXPECT_TRUE (on_path(LatLng(7, 3),        dup, true));
    EXPECT_FALSE(on_path(LatLng(7, 3.000002), dup, true));
}

TEST(Poly, on_path_tolerance_meters) {
    // Tolerance is expressed in meters: (0.0081, 5) is ~900.7 m north of the
    // equator segment, so it is off the path at 500 m but on it at 1000 m.
    std::vector<LatLng> equator = { {0, 0}, {0, 10} };
    LatLng point(0.0081, 5);
    EXPECT_FALSE(on_path(point, equator,  true,  500.0));
    EXPECT_FALSE(on_path(point, equator, false,  500.0));
    EXPECT_TRUE (on_path(point, equator,  true, 1000.0));
    EXPECT_TRUE (on_path(point, equator, false, 1000.0));
}

TEST(Poly, on_path_geodesic_parameter) {
    // A constant-latitude segment is a Rhumb line but not a great circle arc.
    // (60, 15) lies exactly on the Rhumb path between (60,0) and (60,30)
    // but is ~95 km from the corresponding great circle arc.
    std::vector<LatLng> segment = { {60.0, 0.0}, {60.0, 30.0} };
    LatLng midpoint(60.0, 15.0);

    // Rhumb (geodesic=false): point IS on the constant-latitude path
    EXPECT_TRUE(on_path(midpoint, segment, false));

    // Great circle (geodesic=true): point is ~95 km from the arc
    EXPECT_FALSE(on_path(midpoint, segment, true));
}
