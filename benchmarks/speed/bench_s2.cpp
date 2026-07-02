// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// S2 Geometry equivalents. S2 uses a sphere, same as us — apples-to-apples
// comparison on accuracy.
//
// Conversion policy: every native point/geometry is pre-built OUTSIDE the
// timed loop. The timed work is the library's per-call computation, isolated
// from data-shape conversion overhead — so the numbers reflect algorithmic
// cost, not "lat/lng-in / lat/lng-out" plumbing. (Note: this means S2's
// well-known per-call lat/lng→S2Point cost is *not* counted here.)
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

// --- distance: points pre-converted outside the timed loop ----------------

static void BM_S2_DistanceBetween(benchmark::State& state) {
    const auto pts_ll = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    const auto pts = to_s2_points(pts_ll);
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            const S1Angle angle(pts[i], pts[i + 1]);
            benchmark::DoNotOptimize(S2Earth::ToMeters(angle));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_DistanceBetween)->Arg(1000)->Arg(100000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- contains: polygon and query points pre-converted outside the loop ---

static void BM_S2_Contains(benchmark::State& state) {
    const auto poly_ll = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    auto loop = std::make_unique<S2Loop>(to_s2_points(poly_ll));
    loop->Normalize();  // S2Loop expects CCW; Normalize flips if needed.

    const auto queries_ll = geo::bench::bench_queries();
    const auto queries = to_s2_points(queries_ll);

    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(loop->Contains(q));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
}
BENCHMARK(BM_S2_Contains)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- area ------------------------------------------------------------------

static void BM_S2_Area(benchmark::State& state) {
    const auto poly_ll = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    auto loop = std::make_unique<S2Loop>(to_s2_points(poly_ll));
    loop->Normalize();

    const double r2 = S2Earth::RadiusMeters() * S2Earth::RadiusMeters();
    for (auto _ : state) {
        benchmark::DoNotOptimize(loop->GetArea() * r2);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_Area)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- path_length: S2Polyline pre-built outside the timed loop ------------

static void BM_S2_PathLength(benchmark::State& state) {
    const auto path_ll = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    const S2Polyline line(to_s2_points(path_ll));
    for (auto _ : state) {
        benchmark::DoNotOptimize(S2Earth::ToMeters(line.GetLength()));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_S2_PathLength)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- closest point on path: S2Polyline::Project ---------------------------
//
// Same algorithmic class as geo::closest_point_on_path — a linear scan over
// the segments, no spatial index (that would be S2ClosestEdgeQuery over an
// S2ShapeIndex, a different tool).

static void BM_S2_ClosestPointOnPath(benchmark::State& state) {
    const auto route_ll = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    const S2Polyline line(to_s2_points(route_ll));
    const auto queries = to_s2_points(geo::bench::bench_queries());
    int next_vertex = 0;
    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(line.Project(q, &next_vertex));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
}
BENCHMARK(BM_S2_ClosestPointOnPath)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- point at distance: S2Polyline::Interpolate ----------------------------

static void BM_S2_PointAtDistance(benchmark::State& state) {
    const auto route_ll = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    const S2Polyline line(to_s2_points(route_ll));
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            benchmark::DoNotOptimize(line.Interpolate(i / 100.0));
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_S2_PointAtDistance)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);
