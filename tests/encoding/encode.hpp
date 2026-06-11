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
