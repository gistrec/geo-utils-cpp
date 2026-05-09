// Minimal consumer: distance + point-in-polygon, geo-utils-cpp.
// Reads four doubles (lat1 lng1 lat2 lng2) from argv so the optimizer can't
// pre-compute the answer, prints "<distance_m> <inside?>".

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <geo/poly.hpp>
#include <geo/spherical.hpp>

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    char* end;
    geo::LatLng a(std::strtod(argv[1], &end), std::strtod(argv[2], &end));
    geo::LatLng b(std::strtod(argv[3], &end), std::strtod(argv[4], &end));
    std::vector<geo::LatLng> poly = {
        {40.7, -74.1}, {40.8, -74.1}, {40.8, -74.0}, {40.7, -74.0},
    };
    std::printf("%.1f %d\n", geo::distance_between(a, b),
                static_cast<int>(geo::contains(a, poly)));
    return 0;
}
