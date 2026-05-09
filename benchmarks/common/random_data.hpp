// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Deterministic random data shared by every benchmark binary, so that all
// libraries compete on identical inputs.

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <geo/latlng.hpp>

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
// (degrees). Vertices are emitted counter-clockwise (positive signed area
// in our convention).
inline std::vector<LatLng> regular_polygon(std::size_t n, double clat, double clng, double radius_deg) {
    constexpr double kPi = 3.14159265358979323846;
    std::vector<LatLng> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        double a = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
        out.emplace_back(clat + radius_deg * std::cos(a),
                         clng + radius_deg * std::sin(a));
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
        out.emplace_back(clat + radius_deg * jitter(rng),
                         clng + radius_deg * jitter(rng));
    }
    return out;
}

}  // namespace geo::bench
