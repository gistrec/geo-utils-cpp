// Minimal consumer: distance + point-in-polygon, Boost.Geometry.

#include <cstdio>
#include <cstdlib>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

namespace bg = boost::geometry;
using Pt   = bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>;
using Poly = bg::model::polygon<Pt>;

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    char* end;
    Pt a(std::strtod(argv[2], &end), std::strtod(argv[1], &end));  // (lng, lat)
    Pt b(std::strtod(argv[4], &end), std::strtod(argv[3], &end));
    Poly poly;
    bg::append(poly.outer(), Pt{-74.1, 40.7});
    bg::append(poly.outer(), Pt{-74.1, 40.8});
    bg::append(poly.outer(), Pt{-74.0, 40.8});
    bg::append(poly.outer(), Pt{-74.0, 40.7});
    bg::append(poly.outer(), Pt{-74.1, 40.7});
    bg::correct(poly);

    bg::strategy::distance::haversine<double> hav(6371009.0);
    const double d = bg::distance(a, b, hav);
    std::printf("%.1f %d\n", d, static_cast<int>(bg::within(a, poly)));
    return 0;
}
