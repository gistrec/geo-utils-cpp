// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Boost.Geometry equivalents using `cs::spherical_equatorial<degree>` —
// the same sphere-based model we use, so this is a fair head-to-head.
//
// Conversion policy (matches bench_s2.cpp): every native point/geometry is
// pre-built OUTSIDE the timed loop. The timed work is the library's per-call
// computation, isolated from data-shape conversion overhead — so the numbers
// reflect algorithmic cost, not "lat/lng-in / lat/lng-out" plumbing.
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
    const auto pts_ll = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    std::vector<Point> pts;
    pts.reserve(pts_ll.size());
    for (const auto& p : pts_ll) pts.push_back(to_boost_point(p));
    bg::strategy::distance::haversine<double> haversine(kEarthRadius);
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            benchmark::DoNotOptimize(bg::distance(pts[i], pts[i + 1], haversine));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_DistanceBetween)->Arg(1000)->Arg(100000);

// --- heading (azimuth) -----------------------------------------------------

static void BM_Boost_Heading(benchmark::State& state) {
    const auto pts_ll = geo::bench::random_points(2 * static_cast<std::size_t>(state.range(0)));
    std::vector<Point> pts;
    pts.reserve(pts_ll.size());
    for (const auto& p : pts_ll) pts.push_back(to_boost_point(p));
    for (auto _ : state) {
        for (std::size_t i = 0; i + 1 < pts.size(); i += 2) {
            benchmark::DoNotOptimize(bg::azimuth(pts[i], pts[i + 1]));
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_Heading)->Arg(1000)->Arg(100000);

// --- contains: polygon and query points pre-converted outside the loop ----

static void BM_Boost_Contains(benchmark::State& state) {
    const auto poly_ll = geo::bench::regular_polygon(
        static_cast<std::size_t>(state.range(0)), 40.0, -74.0, 5.0);
    const Polygon poly = to_boost_polygon(poly_ll);

    const auto queries_ll = geo::bench::queries_around(40.0, -74.0, 5.0, 1000);
    std::vector<Point> queries;
    queries.reserve(queries_ll.size());
    for (const auto& q : queries_ll) queries.push_back(to_boost_point(q));

    for (auto _ : state) {
        for (const auto& q : queries) {
            benchmark::DoNotOptimize(bg::within(q, poly));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(queries.size()));
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

// --- path_length: LineString pre-built outside the timed loop ------------

static void BM_Boost_PathLength(benchmark::State& state) {
    const auto path_ll = geo::bench::random_points(static_cast<std::size_t>(state.range(0)));
    const LineString ls = to_boost_linestring(path_ll);
    bg::strategy::distance::haversine<double> haversine(kEarthRadius);
    for (auto _ : state) {
        benchmark::DoNotOptimize(bg::length(ls, haversine));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Boost_PathLength)->Arg(10)->Arg(100)->Arg(1000);
