// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// GeographicLib uses an ellipsoid (WGS84 by default) and Karney's iterative
// geodesic algorithm — significantly more accurate than haversine but also
// slower. The numbers here intentionally show that gap; this is a
// *trade-off* benchmark, not a "we are faster" benchmark.
//
// GeographicLib does not ship a native point-in-polygon predicate, so the
// `contains` benchmark is omitted (this is itself information for the reader).

#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/PolygonArea.hpp>

#include "random_data.hpp"

namespace {

inline const GeographicLib::Geodesic& wgs84() {
    return GeographicLib::Geodesic::WGS84();
}

}  // namespace

// --- distance --------------------------------------------------------------

static void BM_GeographicLib_DistanceBetween(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    const auto& geod = wgs84();
    double s12 = 0.0;
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            geod.Inverse(pts[i].lat, pts[i].lng, pts[i + 1].lat, pts[i + 1].lng, s12);
            benchmark::DoNotOptimize(s12);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeographicLib_DistanceBetween)->Arg(1000)->Arg(100000);

// --- heading ---------------------------------------------------------------

static void BM_GeographicLib_Heading(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    const auto& geod = wgs84();
    double s12 = 0.0;
    double azi1 = 0.0;
    double azi2 = 0.0;
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            geod.Inverse(pts[i].lat, pts[i].lng, pts[i + 1].lat, pts[i + 1].lng,
                         s12, azi1, azi2);
            benchmark::DoNotOptimize(azi1);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeographicLib_Heading)->Arg(1000)->Arg(100000);

// --- area (PolygonArea, polyline=false) -----------------------------------

static void BM_GeographicLib_Area(benchmark::State& state) {
    const auto poly = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    for (auto _ : state) {
        GeographicLib::PolygonArea pa(wgs84(), /*polyline=*/false);
        for (const auto& p : poly) pa.AddPoint(p.lat, p.lng);
        double perimeter = 0.0;
        double area = 0.0;
        pa.Compute(/*reverse=*/false, /*sign=*/true, perimeter, area);
        benchmark::DoNotOptimize(area);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeographicLib_Area)->Arg(10)->Arg(100)->Arg(1000);

// --- path_length (PolygonArea with polyline=true) -------------------------

static void BM_GeographicLib_PathLength(benchmark::State& state) {
    const auto path = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        GeographicLib::PolygonArea pa(wgs84(), /*polyline=*/true);
        for (const auto& p : path) pa.AddPoint(p.lat, p.lng);
        double perimeter = 0.0;
        double area = 0.0;
        pa.Compute(/*reverse=*/false, /*sign=*/true, perimeter, area);
        benchmark::DoNotOptimize(perimeter);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GeographicLib_PathLength)->Arg(10)->Arg(100)->Arg(1000);

// Note: contains() benchmark intentionally omitted — GeographicLib does not
// provide a native point-in-polygon predicate. This is a real capability gap,
// not just a benchmark omission.
