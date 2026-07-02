// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0

#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

#include <geo/poly.hpp>
#include <geo/spherical.hpp>

#include "random_data.hpp"

// --- distance_between -------------------------------------------------------

static void BM_GeoUtils_DistanceBetween(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            benchmark::DoNotOptimize(geo::distance_between(pts[i], pts[i + 1]));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_DistanceBetween)->Arg(1000)->Arg(100000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- heading ----------------------------------------------------------------

static void BM_GeoUtils_Heading(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            benchmark::DoNotOptimize(geo::heading(pts[i], pts[i + 1]));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_Heading)->Arg(1000)->Arg(100000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- contains (point-in-polygon) -------------------------------------------

static void BM_GeoUtils_Contains(benchmark::State& state) {
    const auto poly = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    const auto queries = geo::bench::bench_queries();
    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(geo::contains(q, poly));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
}
BENCHMARK(BM_GeoUtils_Contains)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- area -------------------------------------------------------------------

static void BM_GeoUtils_Area(benchmark::State& state) {
    const auto poly = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(geo::area(poly));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_Area)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- path_length ------------------------------------------------------------

static void BM_GeoUtils_PathLength(benchmark::State& state) {
    const auto path = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(geo::path_length(path));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_PathLength)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- closest_point_on_path (snap to route) -----------------------------------

static void BM_GeoUtils_ClosestPointOnPath(benchmark::State& state) {
    const auto route = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    const auto queries = geo::bench::bench_queries();
    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(geo::closest_point_on_path(q, route));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
}
BENCHMARK(BM_GeoUtils_ClosestPointOnPath)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- point_at_distance --------------------------------------------------------

static void BM_GeoUtils_PointAtDistance(benchmark::State& state) {
    const auto route = geo::bench::bench_polygon(static_cast<std::size_t>(state.range(0)));
    const double length = geo::path_length(route);
    // A fixed spread of distances along the route, reused every iteration.
    std::vector<double> distances;
    distances.reserve(100);
    for (int i = 0; i < 100; ++i) {
        distances.push_back(length * i / 100.0);
    }
    for (auto _ : state) {
        for (double d : distances) {
            benchmark::DoNotOptimize(geo::point_at_distance(route, d));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(distances.size()));
}
BENCHMARK(BM_GeoUtils_PointAtDistance)->Arg(10)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// --- simplify: planar (default) vs geodesic metric ---------------------------
//
// Random global points keep nearly every vertex at this tolerance, so the
// Douglas-Peucker recursion does full work — a worst-case-shaped input that
// is identical for both metrics.

static void BM_GeoUtils_Simplify(benchmark::State& state) {
    const auto route = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(geo::simplify(route, 1000.0));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_Simplify)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_GeoUtils_SimplifyGeodesic(benchmark::State& state) {
    const auto route = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(geo::simplify(route, 1000.0, /*geodesic=*/true));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeoUtils_SimplifyGeodesic)->Arg(100)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);
