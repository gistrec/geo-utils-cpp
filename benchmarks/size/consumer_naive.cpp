// Minimal consumer: distance + point-in-polygon, hand-written, no library.
// Establishes a "no-dependency floor" against which every library's overhead
// can be measured.

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

struct LL {
    double lat;
    double lng;
};

double distance(LL a, LL b) {
    constexpr double kR = 6371009.0;
    constexpr double kPi = 3.14159265358979323846;
    const double lat1 = a.lat * kPi / 180.0;
    const double lat2 = b.lat * kPi / 180.0;
    const double dlat = (b.lat - a.lat) * kPi / 180.0;
    const double dlng = (b.lng - a.lng) * kPi / 180.0;
    const double s1 = std::sin(dlat * 0.5);
    const double s2 = std::sin(dlng * 0.5);
    const double h = s1 * s1 + std::cos(lat1) * std::cos(lat2) * s2 * s2;
    return 2.0 * kR * std::asin(std::sqrt(h));
}

// Planar ray-cast — fine for a tiny bbox at ~40°N. NOT correct in general
// for spherical polygons, but representative of "the simplest thing a
// developer would write before reaching for a library".
bool contains(LL p, const std::vector<LL>& poly) {
    bool inside = false;
    const std::size_t n = poly.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        if ((poly[i].lng > p.lng) != (poly[j].lng > p.lng) &&
            p.lat < (poly[j].lat - poly[i].lat) * (p.lng - poly[i].lng) /
                            (poly[j].lng - poly[i].lng) +
                        poly[i].lat) {
            inside = !inside;
        }
    }
    return inside;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) return 1;
    LL a{std::atof(argv[1]), std::atof(argv[2])};
    LL b{std::atof(argv[3]), std::atof(argv[4])};
    std::vector<LL> poly = {
        {40.7, -74.1}, {40.8, -74.1}, {40.8, -74.0}, {40.7, -74.0},
    };
    std::printf("%.1f %d\n", distance(a, b), static_cast<int>(contains(a, poly)));
    return 0;
}
