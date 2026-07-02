#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include <vector>

#include <geo/encoding.hpp>

using geo::LatLng;
using geo::decode;
using geo::encode;

TEST(Encoding, decode) {
    // Empty.
    EXPECT_TRUE(decode("").empty());

    // Reference example from the Encoded Polyline Algorithm Format docs.
    auto path = decode("_p~iF~ps|U_ulLnnqC_mqNvxq`@");
    ASSERT_EQ(path.size(), 3U);
    EXPECT_TRUE(path[0].approx_equal(LatLng(38.5, -120.2), 1e-9));
    EXPECT_TRUE(path[1].approx_equal(LatLng(40.7, -120.95), 1e-9));
    EXPECT_TRUE(path[2].approx_equal(LatLng(43.252, -126.453), 1e-9));

    // Truncated mid-chunk: the incomplete trailing point is dropped.
    auto cut_in_lat = decode("_p~iF~ps|U_ul");
    ASSERT_EQ(cut_in_lat.size(), 1U);
    EXPECT_TRUE(cut_in_lat[0].approx_equal(LatLng(38.5, -120.2), 1e-9));

    // Truncated between chunks: lat decoded but lng missing — point dropped.
    auto cut_after_lat = decode("_p~iF~ps|U_ulL");
    ASSERT_EQ(cut_after_lat.size(), 1U);
    EXPECT_TRUE(cut_after_lat[0].approx_equal(LatLng(38.5, -120.2), 1e-9));
}

TEST(Encoding, decode_adversarial_overflow) {
    // Malformed input with three consecutive lat deltas of +2^30-1: the raw
    // sum exceeds INT32_MAX. Decoding must stay well-defined (unsigned
    // wrap-around, matching the Java original) — under UBSan this test used
    // to abort with signed-overflow UB. Coordinates are unspecified for
    // malformed input; only the point count and termination are guaranteed.
    std::string adversarial;
    for (int i = 0; i < 3; ++i) {
        geo::detail::encode_value(0x3FFFFFFF, adversarial);  // lat delta
        geo::detail::encode_value(0, adversarial);           // lng delta
    }
    EXPECT_EQ(decode(adversarial).size(), 3U);
}

TEST(Encoding, decode_precision) {
    // polyline6 reference vector from the mapbox/polyline test suite.
    auto path = decode("_izlhA~rlgdF_{geC~ywl@_kwzCn`{nI", 6);
    ASSERT_EQ(path.size(), 3U);
    EXPECT_TRUE(path[0].approx_equal(LatLng(38.5, -120.2), 1e-9));
    EXPECT_TRUE(path[1].approx_equal(LatLng(40.7, -120.95), 1e-9));
    EXPECT_TRUE(path[2].approx_equal(LatLng(43.252, -126.453), 1e-9));

    // The default precision is 5.
    auto classic = decode("_p~iF~ps|U_ulLnnqC_mqNvxq`@");
    auto explicit5 = decode("_p~iF~ps|U_ulLnnqC_mqNvxq`@", 5);
    ASSERT_EQ(classic.size(), explicit5.size());
    for (std::size_t i = 0; i < classic.size(); ++i) {
        EXPECT_EQ(classic[i], explicit5[i]);
    }
}

TEST(Encoding, encode_decode_roundtrip) {
    std::vector<LatLng> path = {
        {0, 0}, {90, 180}, {-90, -180}, {1.00001, -1.00001},
        {59.93863, 30.31413}, {-33.86882, 151.20929},
    };
    auto decoded = decode(encode(path));
    ASSERT_EQ(decoded.size(), path.size());
    for (std::size_t i = 0; i < path.size(); ++i) {
        // Quantization to 1e-5 degrees: round-trip error is at most 5e-6.
        EXPECT_TRUE(decoded[i].approx_equal(path[i], 1e-5)) << decoded[i];
    }

    // Precision 6: the round-trip error shrinks to 5e-7. Unlike 7+, every
    // valid coordinate and every point-to-point delta (up to ±360° of
    // longitude) still fits the decoder's 32-bit arithmetic.
    auto decoded6 = decode(encode(path, 6), 6);
    ASSERT_EQ(decoded6.size(), path.size());
    for (std::size_t i = 0; i < path.size(); ++i) {
        EXPECT_TRUE(decoded6[i].approx_equal(path[i], 1e-6)) << decoded6[i];
    }

    // Precision 7 round-trips paths whose deltas stay within ~107°  — that
    // includes the first point, whose deltas are taken from (0, 0); larger
    // deltas overflow the 32-bit zig-zag.
    std::vector<LatLng> nearby = {
        {38.5, -100.2}, {40.7, -100.95}, {43.252, -106.453}, {-38.5, -20.2},
    };
    auto decoded7 = decode(encode(nearby, 7), 7);
    ASSERT_EQ(decoded7.size(), nearby.size());
    for (std::size_t i = 0; i < nearby.size(); ++i) {
        EXPECT_TRUE(decoded7[i].approx_equal(nearby[i], 1e-7)) << decoded7[i];
    }
}
