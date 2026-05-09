// Minimal consumer: distance + point-in-polygon, geo-utils-cpp.
// Reads four doubles (lat1 lng1 lat2 lng2) from argv so the optimizer can't
// pre-compute the answer, prints "<distance_m> <inside?>".

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <geo/geo.hpp>

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    geo::LatLng a(std::atof(argv[1]), std::atof(argv[2]));
    geo::LatLng b(std::atof(argv[3]), std::atof(argv[4]));
    std::vector<geo::LatLng> poly = {
        {40.7, -74.1}, {40.8, -74.1}, {40.8, -74.0}, {40.7, -74.0},
    };
    std::printf("%.1f %d\n", geo::distance_between(a, b),
                static_cast<int>(geo::contains(a, poly)));
    return 0;
}
