// Emscripten/embind wrapper. Arrays cross the boundary as plain JS arrays of numbers
// (flat [lat, lng, lat, lng, ...]); every function is a thin forward to demo_api.hpp.
#include "demo_api.hpp"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>
#include <vector>

namespace {

using emscripten::val;

demo::Flat in(const val& array) {
    return emscripten::convertJSArrayToNumberVector<double>(array);
}

val out(const demo::Flat& flat) {
    val array = val::array();
    for (std::size_t i = 0; i < flat.size(); ++i) {
        array.set(i, flat[i]);
    }
    return array;
}

}  // namespace

EMSCRIPTEN_BINDINGS(geo_demo) {
    using namespace emscripten;

    function("version", &demo::version);
    function("distanceBetween", &demo::distance_between);
    function("heading", &demo::heading);
    function("offset", +[](double lat, double lng, double d, double h) { return out(demo::offset(lat, lng, d, h)); });
    function("interpolate", +[](double a, double b, double c, double d, double f) {
        return out(demo::interpolate(a, b, c, d, f));
    });
    function("pathLength", +[](const val& path) { return demo::path_length(in(path)); });
    function("pointAtDistance", +[](const val& path, double d) { return out(demo::point_at_distance(in(path), d)); });
    function("area", +[](const val& path) { return demo::area(in(path)); });
    function("contains", +[](double lat, double lng, const val& poly, bool geodesic) {
        return demo::contains(lat, lng, in(poly), geodesic);
    });
    function("onEdge", +[](double lat, double lng, const val& poly, bool geodesic, double tol) {
        return demo::on_edge(lat, lng, in(poly), geodesic, tol);
    });
    function("distanceToEdge", +[](double lat, double lng, const val& poly) {
        return demo::distance_to_edge(lat, lng, in(poly));
    });
    function("encode", +[](const val& path, unsigned precision) { return demo::encode(in(path), precision); });
    function("decode", +[](const std::string& s, unsigned precision) { return out(demo::decode(s, precision)); });
    function("simplify", +[](const val& path, double tol, bool geodesic) {
        return out(demo::simplify(in(path), tol, geodesic));
    });
    function("closestPointOnPath", +[](double lat, double lng, const val& path) {
        return out(demo::closest_point_on_path(lat, lng, in(path)));
    });
    function("bounds", +[](const val& path) { return out(demo::bounds(in(path))); });
}
