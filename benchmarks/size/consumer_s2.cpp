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
    const auto a = S2LatLng::FromDegrees(std::atof(argv[1]), std::atof(argv[2])).ToPoint();
    const auto b = S2LatLng::FromDegrees(std::atof(argv[3]), std::atof(argv[4])).ToPoint();
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
