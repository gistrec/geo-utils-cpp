// Native reference: runs a fixed set of inputs through the same demo_api.hpp that the
// WASM build wraps, and prints them as JSON. test/verify.mjs feeds the identical inputs to
// the WebAssembly module and compares.
#include "../demo_api.hpp"

#include <cstdio>
#include <string>

namespace {

const demo::Flat kRoute = {37.7749, -122.4194, 37.8044, -122.2712, 37.8716, -122.2727, 38.5816, -121.4944};
const demo::Flat kPolygon = {37.80, -122.45, 37.80, -122.40, 37.76, -122.40, 37.76, -122.45};
const demo::Flat kAnti = {-17.0, 179.5, -17.5, -179.5, -18.0, 179.8};
const std::string kPoly5 = "_p~iF~ps|U_ulLnnqC_mqNvxq`@";  // Google's documentation example

void print_array(const char* key, const demo::Flat& v, bool comma = true) {
    std::printf("  \"%s\": [", key);
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::printf("%s%.17g", i ? ", " : "", v[i]);
    }
    std::printf("]%s\n", comma ? "," : "");
}

void print_number(const char* key, double v, bool comma = true) {
    std::printf("  \"%s\": %.17g%s\n", key, v, comma ? "," : "");
}

}  // namespace

int main() {
    std::printf("{\n");
    std::printf("  \"version\": \"%s\",\n", demo::version().c_str());
    print_number("distance_sf_ny", demo::distance_between(37.7749, -122.4194, 40.7128, -74.0060));
    print_number("heading_sf_ny", demo::heading(37.7749, -122.4194, 40.7128, -74.0060));
    print_array("offset", demo::offset(37.7749, -122.4194, 10000.0, 45.0));
    print_array("interpolate", demo::interpolate(37.7749, -122.4194, 40.7128, -74.0060, 0.25));
    print_number("path_length", demo::path_length(kRoute));
    print_array("point_at_distance", demo::point_at_distance(kRoute, 20000.0));
    print_number("area_polygon", demo::area(kPolygon));
    print_number("contains_inside", demo::contains(37.78, -122.42, kPolygon, false));
    print_number("contains_outside", demo::contains(37.90, -122.42, kPolygon, false));
    print_number("distance_to_edge", demo::distance_to_edge(37.78, -122.42, kPolygon));
    std::printf("  \"encode5\": \"%s\",\n", demo::encode(kRoute, 5).c_str());
    std::printf("  \"encode6\": \"%s\",\n", demo::encode(kRoute, 6).c_str());
    print_array("decode5", demo::decode(kPoly5, 5));
    print_array("decode6_roundtrip", demo::decode(demo::encode(kRoute, 6), 6));
    print_array("simplify_100m", demo::simplify(kRoute, 100.0, false));
    print_array("closest", demo::closest_point_on_path(37.83, -122.30, kRoute));
    print_array("bounds_anti", demo::bounds(kAnti));
    print_number("path_length_anti", demo::path_length(kAnti));
    print_number("area_anti", demo::area(kAnti), false);
    std::printf("}\n");
    return 0;
}
