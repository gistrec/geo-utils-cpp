// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Boost.Geometry equivalents using `cs::spherical_equatorial<degree>` —
// the same sphere-based model we use, so this is a fair head-to-head.
//
// Conversion policy (matches bench_s2.cpp):
//   * Streams of points: converted INSIDE the timed loop.
//   * Long-lived polygon: pre-converted ONCE outside the loop.
//
// Coordinate convention reminder: Boost.Geometry expects (lng, lat) order
// for spherical_equatorial points.
//
// Strategy note for `contains`: bg::within with cs::spherical_equatorial
// auto-selects strategy::within::spherical_winding, which traces great-circle
// edges and classifies crossings carefully. Our own `geo::contains` defaults
// to rhumb-line edges (geodesic=false), which is significantly cheaper.
// The performance gap on `contains` reflects this algorithmic difference,
// not a Boost misconfiguration.

#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/strategies/spherical/azimuth.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

#include "random_data.hpp"

namespace bg = boost::geometry;

namespace {

using Point     = bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>;
using Polygon   = bg::model::polygon<Point>;
using LineString = bg::model::linestring<Point>;

constexpr double kEarthRadius = 6371009.0;

inline Point to_boost_point(const geo::LatLng& p) {
    return Point(p.lng, p.lat);  // (lng, lat) order!
}

inline Polygon to_boost_polygon(const std::vector<geo::LatLng>& pts) {
    Polygon poly;
    auto& outer = poly.outer();
    outer.reserve(pts.size() + 1);
    for (const auto& p : pts) outer.emplace_back(p.lng, p.lat);
    outer.emplace_back(pts.front().lng, pts.front().lat);  // close the ring
    bg::correct(poly);  // ensure orientation matches Boost's expectation
    return poly;
}

inline LineString to_boost_linestring(const std::vector<geo::LatLng>& pts) {
    LineString ls;
    ls.reserve(pts.size());
    for (const auto& p : pts) ls.emplace_back(p.lng, p.lat);
    return ls;
}

}  // namespace

// --- distance --------------------------------------------------------------

static void BM_Boost_DistanceBetween(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    bg::strategy::distance::haversine<double> haversine(kEarthRadius);
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            const Point a = to_boost_point(pts[i]);
            const Point b = to_boost_point(pts[i + 1]);
            benchmark::DoNotOptimize(bg::distance(a, b, haversine));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_DistanceBetween)->Arg(1000)->Arg(100000);

// --- heading (azimuth) -----------------------------------------------------

static void BM_Boost_Heading(benchmark::State& state) {
    const auto pts = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            const Point a = to_boost_point(pts[i]);
            const Point b = to_boost_point(pts[i + 1]);
            benchmark::DoNotOptimize(bg::azimuth(a, b));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_Heading)->Arg(1000)->Arg(100000);

// --- contains: polygon converted once; queries converted inside the loop --

static void BM_Boost_Contains(benchmark::State& state) {
    const auto poly_ll = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    const Polygon poly = to_boost_polygon(poly_ll);

    const auto queries_ll = geo::bench::queries_around(40.0, -74.0, 5.0, 1000);

    for (auto _ : state) {
        for (const auto& q : queries_ll) {
            benchmark::DoNotOptimize(bg::within(to_boost_point(q), poly));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries_ll.size()));
}
BENCHMARK(BM_Boost_Contains)->Arg(10)->Arg(100)->Arg(1000);

// --- area ------------------------------------------------------------------

static void BM_Boost_Area(benchmark::State& state) {
    const auto poly_ll = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    const Polygon poly = to_boost_polygon(poly_ll);
    // bg::area on a spherical CS returns area on the unit sphere (steradians);
    // multiply by R² to get square meters.
    const double r2 = kEarthRadius * kEarthRadius;
    for (auto _ : state) {
        benchmark::DoNotOptimize(bg::area(poly) * r2);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_Area)->Arg(10)->Arg(100)->Arg(1000);

// --- path_length: input is lat/lng; convert + sum inside the timed loop ---

static void BM_Boost_PathLength(benchmark::State& state) {
    const auto path_ll = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    bg::strategy::distance::haversine<double> haversine(kEarthRadius);
    for (auto _ : state) {
        LineString ls = to_boost_linestring(path_ll);
        benchmark::DoNotOptimize(bg::length(ls, haversine));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_PathLength)->Arg(10)->Arg(100)->Arg(1000);
