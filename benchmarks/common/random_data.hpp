// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Deterministic random data shared by every benchmark binary, so that all
// libraries compete on identical inputs.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <geo/latlng.hpp>

#include "constants.hpp"

namespace geo::bench {

inline constexpr std::uint64_t kSeed = 0xC0FFEEull;

// Returns `n` random points uniformly distributed over the globe. Latitude is
// clamped to [-80, 80] to keep all libraries on safe ground (S2 is fine near
// the poles, but ellipsoidal Inverse can be numerically iffy for nearly
// antipodal pairs).
inline std::vector<LatLng> random_points(std::size_t n, std::uint64_t seed = kSeed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> lat(-80.0, 80.0);
    std::uniform_real_distribution<double> lng(-180.0, 180.0);
    std::vector<LatLng> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.emplace_back(lat(rng), lng(rng));
    }
    return out;
}

// Returns a regular n-gon centered at (clat, clng) with the given radius
// (degrees). Vertices are emitted counter-clockwise when viewed with
// latitude on the y-axis and longitude on the x-axis (the standard map
// orientation, looking down from above), so the first vertex is due
// north of the center and successive vertices march west — that's
// positive signed area in our convention.
inline std::vector<LatLng> regular_polygon(std::size_t n, double clat, double clng, double radius_deg) {
    std::vector<LatLng> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        double a = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
        out.emplace_back(clat + radius_deg * std::cos(a),
                         clng - radius_deg * std::sin(a));
    }
    return out;
}

// Query points clustered around the polygon center. Roughly half land
// inside, half outside — exercises both branches of contains().
inline std::vector<LatLng> queries_around(double clat, double clng, double radius_deg,
                                          std::size_t n, std::uint64_t seed = kSeed + 1) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> jitter(-2.0, 2.0);
    std::vector<LatLng> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        // Clamp to valid lat/lng so callers picking large radii can't produce
        // out-of-domain inputs that some libraries silently NaN on.
        const double lat = std::clamp(clat + radius_deg * jitter(rng), -90.0, 90.0);
        const double lng = std::clamp(clng + radius_deg * jitter(rng), -180.0, 180.0);
        out.emplace_back(lat, lng);
    }
    return out;
}

// Sugar over `regular_polygon` and `queries_around` using the shared test
// polygon constants from constants.hpp. Use these in benchmarks instead of
// hard-coding (40.0, -74.0, 5.0) per-file.
inline std::vector<LatLng> bench_polygon(std::size_t n) {
    return regular_polygon(n, kPolyCenterLat, kPolyCenterLng, kPolyRadiusDeg);
}

inline std::vector<LatLng> bench_queries() {
    return queries_around(kPolyCenterLat, kPolyCenterLng, kPolyRadiusDeg,
                          kQueriesPerIteration);
}

}  // namespace geo::bench
