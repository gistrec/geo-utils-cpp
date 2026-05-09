// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Reference baseline: a textbook haversine implementation, no library at all.
// Tells us how much overhead `geo-utils-cpp` adds over the simplest possible
// hand-written code (ideally: zero — we are inlined headers).

#include <benchmark/benchmark.h>

#include <cmath>
#include <cstddef>

#include "random_data.hpp"

namespace {

constexpr double kEarthRadius = 6371009.0;
constexpr double kPi = 3.14159265358979323846;

inline double naive_distance(geo::LatLng a, geo::LatLng b) noexcept {
    const double lat1 = a.lat * kPi / 180.0;
    const double lat2 = b.lat * kPi / 180.0;
    const double dlat = (b.lat - a.lat) * kPi / 180.0;
    const double dlng = (b.lng - a.lng) * kPi / 180.0;
    const double s_lat = std::sin(dlat * 0.5);
    const double s_lng = std::sin(dlng * 0.5);
    const double h = s_lat * s_lat + std::cos(lat1) * std::cos(lat2) * s_lng * s_lng;
    return 2.0 * kEarthRadius * std::asin(std::sqrt(h));
}

}  // namespace

static void BM_Naive_DistanceBetween(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            benchmark::DoNotOptimize(naive_distance(pts[i], pts[i + 1]));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Naive_DistanceBetween)->Arg(1000)->Arg(100000);
