// Minimal consumer: distance + point-in-polygon, S2 Geometry.

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include <s2/s1angle.h>
#include <s2/s2earth.h>
#include <s2/s2latlng.h>
#include <s2/s2loop.h>
#include <s2/s2point.h>

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    char* end;
    const auto a = S2LatLng::FromDegrees(std::strtod(argv[1], &end), std::strtod(argv[2], &end)).ToPoint();
    const auto b = S2LatLng::FromDegrees(std::strtod(argv[3], &end), std::strtod(argv[4], &end)).ToPoint();
    std::vector<S2Point> verts = {
        S2LatLng::FromDegrees(40.7, -74.1).ToPoint(),
        S2LatLng::FromDegrees(40.8, -74.1).ToPoint(),
        S2LatLng::FromDegrees(40.8, -74.0).ToPoint(),
        S2LatLng::FromDegrees(40.7, -74.0).ToPoint(),
    };
    auto loop = std::make_unique<S2Loop>(verts);
    loop->Normalize();
    const double d = S2Earth::ToMeters(S1Angle(a, b));
    std::printf("%.1f %d\n", d, static_cast<int>(loop->Contains(a)));
    return 0;
}
