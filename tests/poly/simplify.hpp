#include <gtest/gtest.h>
#include <cstddef>
#include <utility>
#include <vector>

#include <geo/encoding.hpp>
#include <geo/poly.hpp>
#include <geo/spherical.hpp>

using geo::LatLng;
using geo::decode;
using geo::is_closed_polygon;
using geo::path_length;
using geo::simplify;

namespace {

// Checks the upstream PolyUtil.simplify invariants: endpoints are preserved,
// every output point is one of the input points (in input order), and the
// simplified path is not longer than the original.
void expect_simplified(const std::vector<LatLng>& line, const std::vector<LatLng>& simplified) {
    ASSERT_FALSE(simplified.empty());
    EXPECT_EQ(simplified.front(), line.front());
    EXPECT_EQ(simplified.back(), line.back());

    std::size_t pos = 0;
    for (const auto& point : simplified) {
        bool found = false;
        while (pos < line.size()) {
            if (line[pos++] == point) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << point << " is not an input point (or out of order)";
    }

    EXPECT_LE(path_length(simplified), path_length(line));
}

}  // namespace

TEST(Poly, simplify_smallPaths) {
    EXPECT_TRUE(simplify(std::vector<LatLng>{}, 10.0).empty());

    std::vector<LatLng> one = {{28.06025, -82.41030}};
    auto simplified_one = simplify(one, 10.0);
    ASSERT_EQ(simplified_one.size(), 1U);
    EXPECT_EQ(simplified_one[0], one[0]);

    std::vector<LatLng> two = {{28.06025, -82.41030}, {28.06035, -82.40834}};
    auto simplified_two = simplify(two, 10.0);
    ASSERT_EQ(simplified_two.size(), 2U);
    EXPECT_EQ(simplified_two[0], two[0]);
    EXPECT_EQ(simplified_two[1], two[1]);
}

TEST(Poly, simplify_line) {
    // 95-point polyline from the upstream test suite.
    static const char kLine[] =
        "elfjD~a}uNOnFN~Em@fJv@tEMhGDjDe@hG^nF??@lA?n@IvAC`Ay@A{@DwCA{CF_EC{CEi@PBTFDJBJ?V?n@?D@?A@?@?F?F?"
        "LAf@?n@@`@@T@~@FpA?fA?p@?r@?vAH`@OR@^ETFJCLD?JA^?J?P?fAC`B@d@?b@A\\@`@Ad@@\\?`@?f@?V?H?DD@DDBBDBD?"
        "D?B?B@B@@@B@B@B@D?D?JAF@H@FCLADBDBDCFAN?b@Af@@x@@";
    const std::vector<LatLng> line = decode(kLine);
    ASSERT_EQ(line.size(), 95U);

    // Expected sizes match the upstream test suite.
    const std::pair<double, std::size_t> expected[] = {
        {5, 20}, {10, 14}, {15, 10}, {20, 8}, {50, 6}, {500, 3}, {1000, 2},
    };
    for (const auto& [tolerance, size] : expected) {
        auto simplified = simplify(line, tolerance);
        EXPECT_EQ(simplified.size(), size) << "tolerance = " << tolerance;
        expect_simplified(line, simplified);
    }
}

TEST(Poly, simplify_triangle) {
    // Open triangle from the upstream test suite.
    std::vector<LatLng> triangle = {
        {28.06025, -82.41030}, {28.06129, -82.40945}, {28.06206, -82.40917},
        {28.06125, -82.40850}, {28.06035, -82.40834}, {28.06038, -82.40924},
    };
    ASSERT_FALSE(is_closed_polygon(triangle));

    auto simplified = simplify(triangle, 88.0);
    EXPECT_EQ(simplified.size(), 4U);
    expect_simplified(triangle, simplified);

    // Closing the triangle keeps the simplified size: the closing segment is
    // included in the simplification.
    triangle.push_back(triangle.front());
    ASSERT_TRUE(is_closed_polygon(triangle));

    auto simplified_closed = simplify(triangle, 88.0);
    EXPECT_EQ(simplified_closed.size(), 4U);
    expect_simplified(triangle, simplified_closed);
}

TEST(Poly, simplify_oval) {
    // Oval polygon from the upstream test suite.
    static const char kOval[] =
        "}wgjDxw_vNuAd@}AN{A]w@_Au@kAUaA?{@Ke@@_@C]D[FULWFOLSNMTOVOXO\\I\\CX?VJXJTDTNXTVVLVJ`@FXA\\AVLZBTAT"
        "BZ@ZAT?\\?VFT@XGZ";
    std::vector<LatLng> oval = decode(kOval);
    ASSERT_FALSE(is_closed_polygon(oval));

    auto simplified = simplify(oval, 10.0);
    EXPECT_EQ(simplified.size(), 13U);
    expect_simplified(oval, simplified);

    // Closed oval: same result.
    oval.push_back(oval.front());
    ASSERT_TRUE(is_closed_polygon(oval));

    auto simplified_closed = simplify(oval, 10.0);
    EXPECT_EQ(simplified_closed.size(), 13U);
    expect_simplified(oval, simplified_closed);
}
