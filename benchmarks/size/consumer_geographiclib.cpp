// Minimal consumer: distance only, GeographicLib.
// GeographicLib does not provide a native point-in-polygon predicate, so we
// emit "-1" in that column to make the omission explicit (rather than
// inflating its size by adding a hand-rolled PIP that no caller would
// actually depend on).

#include <cstdio>
#include <cstdlib>

#include <GeographicLib/Geodesic.hpp>

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    double s12 = 0.0;
    GeographicLib::Geodesic::WGS84().Inverse(
        std::atof(argv[1]), std::atof(argv[2]),
        std::atof(argv[3]), std::atof(argv[4]),
        s12);
    std::printf("%.1f -1\n", s12);
    return 0;
}
