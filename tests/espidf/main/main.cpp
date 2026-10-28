// One call into the library, compiled and linked with the IDF toolchain.
#include <cstdio>

#include <geo/geo.hpp>

extern "C" void app_main() {
    const geo::LatLng nyc{40.7128, -74.0060};
    const geo::LatLng london{51.5074, -0.1278};
    std::printf("NYC-London: %.0f m\n", geo::distance_between(nyc, london));
}
