#include <gtest/gtest.h>
#include <string>
#include <vector>

#include <geo/encoding.hpp>

using geo::LatLng;
using geo::encode;

TEST(Encoding, encode) {
    // Empty.
    EXPECT_EQ(encode(std::vector<LatLng>{}), "");

    // Single zero point: zig-zag 0 encodes as '?' for both coordinates.
    EXPECT_EQ(encode(std::vector<LatLng>{ {0, 0} }), "??");

    // Reference example from the Encoded Polyline Algorithm Format docs.
    std::vector<LatLng> path = { {38.5, -120.2}, {40.7, -120.95}, {43.252, -126.453} };
    EXPECT_EQ(encode(path), "_p~iF~ps|U_ulLnnqC_mqNvxq`@");

    // Reference example for a single negative value (-179.9832104).
    std::vector<LatLng> negative = { {0, -179.9832104} };
    EXPECT_EQ(encode(negative), "?`~oia@");

    // Quantization: differences below 1e-5 degrees collapse to the same string.
    std::vector<LatLng> a = { {38.5, -120.2} };
    std::vector<LatLng> b = { {38.500000004, -120.199999996} };
    EXPECT_EQ(encode(a), encode(b));
}

TEST(Encoding, encode_precision) {
    // polyline6 reference vector from the mapbox/polyline test suite: the
    // same three points as the classic example, on the 1e-6 grid.
    std::vector<LatLng> path = { {38.5, -120.2}, {40.7, -120.95}, {43.252, -126.453} };
    EXPECT_EQ(encode(path, 6), "_izlhA~rlgdF_{geC~ywl@_kwzCn`{nI");

    // The default precision is 5 — same string as the classic example.
    EXPECT_EQ(encode(path, 5), encode(path));

    // At precision 6, 1e-5-sized differences no longer collapse.
    std::vector<LatLng> a = { {38.5, -120.2} };
    std::vector<LatLng> b = { {38.500004, -120.199996} };
    EXPECT_EQ(encode(a), encode(b));
    EXPECT_NE(encode(a, 6), encode(b, 6));
}
