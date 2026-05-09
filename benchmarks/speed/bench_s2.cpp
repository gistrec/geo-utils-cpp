// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// S2 Geometry equivalents. S2 uses a sphere, same as us — apples-to-apples
// comparison on accuracy. Where the API forces a different shape (input is
// 3D unit vector S2Point, not lat/lng), we include the conversion in the
// timed loop because that is the realistic cost when the data comes from
// outside as lat/lng.
//
// Note: S2 does not ship a public initial-bearing function, so the heading
// benchmark is intentionally absent — this is itself information for the
// reader.

#include <benchmark/benchmark.h>

#include <cstddef>
#include <memory>
#include <vector>

#include <s2/s1angle.h>
#include <s2/s2earth.h>
#include <s2/s2latlng.h>
#include <s2/s2loop.h>
#include <s2/s2point.h>
#include <s2/s2polyline.h>

#include "random_data.hpp"

namespace {

inline S2Point to_s2_point(const geo::LatLng& p) {
    return S2LatLng::FromDegrees(p.lat, p.lng).ToPoint();
}

inline std::vector<S2Point> to_s2_points(const std::vector<geo::LatLng>& src) {
    std::vector<S2Point> out;
    out.reserve(src.size());
    for (const auto& p : src) out.push_back(to_s2_point(p));
    return out;
}

}  // namespace

// --- distance: input is lat/lng, conversion counted in the timed loop ------

static void BM_S2_DistanceBetween(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            const S2Point a = to_s2_point(pts[i]);
            const S2Point b = to_s2_point(pts[i + 1]);
            const S1Angle angle(a, b);
            benchmark::DoNotOptimize(S2Earth::ToMeters(angle));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_DistanceBetween)->Arg(1000)->Arg(100000);

// --- contains: polygon converted once outside the loop ---------------------

static void BM_S2_Contains(benchmark::State& state) {
    const auto poly_ll = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    auto loop = std::make_unique<S2Loop>(to_s2_points(poly_ll));
    loop->Normalize();  // S2Loop expects CCW; Normalize flips if needed.

    const auto queries_ll = geo::bench::queries_around(40.0, -74.0, 5.0, 1000);
    const auto queries = to_s2_points(queries_ll);

    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(loop->Contains(q));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
}
BENCHMARK(BM_S2_Contains)->Arg(10)->Arg(100)->Arg(1000);

// --- area ------------------------------------------------------------------

static void BM_S2_Area(benchmark::State& state) {
    const auto poly_ll = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    auto loop = std::make_unique<S2Loop>(to_s2_points(poly_ll));
    loop->Normalize();

    const double r2 = S2Earth::RadiusMeters() * S2Earth::RadiusMeters();
    for (auto _ : state) {
        benchmark::DoNotOptimize(loop->GetArea() * r2);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_Area)->Arg(10)->Arg(100)->Arg(1000);

// --- path_length -----------------------------------------------------------

static void BM_S2_PathLength(benchmark::State& state) {
    const auto path_ll = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    const S2Polyline line(to_s2_points(path_ll));
    for (auto _ : state) {
        benchmark::DoNotOptimize(S2Earth::ToMeters(line.GetLength()));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_PathLength)->Arg(10)->Arg(100)->Arg(1000);
