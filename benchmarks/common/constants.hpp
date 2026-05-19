// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Shared compile-time constants used by every benchmark binary. Centralized
// so a single change here updates all five competitor benchmarks and the
// random-data generator.

#pragma once

#include <cstddef>

namespace geo::bench {

inline constexpr double kPi = 3.14159265358979323846;

// Earth radius in meters. Matches `geo-utils-cpp`'s internal constant; we
// reuse the same value for Boost.Geometry's haversine strategy and the naive
// baseline so every sphere-based library normalizes to identical units.
// S2 uses `S2Earth::RadiusMeters()` (6371010.0); that ~1 m difference
// affects absolute results but not throughput.
inline constexpr double kEarthRadiusMeters = 6371009.0;

// Test polygon: regular n-gon centered over NYC, 5° radius. Stays well
// inside ±80° latitude (where ellipsoidal Inverse is numerically safe) and
// is large enough that 2°-jitter queries hit both inside/outside branches
// of contains().
inline constexpr double kPolyCenterLat = 40.0;
inline constexpr double kPolyCenterLng = -74.0;
inline constexpr double kPolyRadiusDeg = 5.0;

// Query points per `contains` benchmark iteration.
inline constexpr std::size_t kQueriesPerIteration = 1000;

}  // namespace geo::bench
