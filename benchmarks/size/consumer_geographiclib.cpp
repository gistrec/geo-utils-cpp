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
    char* end;
    GeographicLib::Geodesic::WGS84().Inverse(
        std::strtod(argv[1], &end), std::strtod(argv[2], &end),
        std::strtod(argv[3], &end), std::strtod(argv[4], &end),
        s12);
    std::printf("%.1f -1\n", s12);
    return 0;
}
