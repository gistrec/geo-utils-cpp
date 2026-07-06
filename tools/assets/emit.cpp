// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// emit.cpp -- the keystone of the asset pipeline.
//
// Runs the *real* geo-utils-cpp pipeline over the same 95-point encoded
// polyline used by examples/gps_track.cpp (decode -> bounds -> simplify ->
// closest_point_on_path -> point_at_distance -> encode) and prints the result
// as GeoJSON / JSON Lines. All geometry stays here in C++; the Python
// renderers under tools/render/ only draw what this program emits, so every
// visual is provably "over real geo-utils output", not a mock-up.
//
// Output goes to stdout; tools/make-assets.sh redirects each mode to a file
// under docs/assets/data/. Coordinates follow the GeoJSON spec order
// [longitude, latitude] and are printed with the C locale (fixed precision),
// so the output is byte-stable across re-runs on the same toolchain.
//
// Build & run:
//   g++ -std=c++17 -Iinclude tools/assets/emit.cpp -o build/emit
//   build/emit                 > docs/assets/data/track.geojson
//   build/emit --dp-frames     > docs/assets/data/dp.jsonl
//   build/emit --eta-frames 60 > docs/assets/data/eta.jsonl

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <geo/geo.hpp>

namespace {

// The same 95-point GPS track examples/gps_track.cpp uses: a real recorded
// route in the classic Encoded Polyline format (1e-5-degree grid). Keeping
// one shared track means the hero animation, the DP collapse, and the gallery
// stills all show the exact same data flowing through the exact same calls.
const char kTrack[] =
    "elfjD~a}uNOnFN~Em@fJv@tEMhGDjDe@hG^nF??@lA?n@IvAC`Ay@A{@DwCA{CF_EC{CEi@PBTFDJBJ?V?n@?D@?A@?@?F?F?"
    "LAf@?n@@`@@T@~@FpA?fA?p@?r@?vAH`@OR@^ETFJCLD?JA^?J?P?fAC`B@d@?b@A\\@`@Ad@@\\?`@?f@?V?H?DD@DDBBDBD?"
    "D?B?B@B@@@B@B@B@D?D?JAF@H@FCLADBDBDCFAN?b@Af@@x@@";

// Douglas-Peucker tolerance sweep for the collapse animation (meters,
// decreasing). Coarse first (few points) down to GPS-noise level.
const double kDpTolerances[] = {300.0, 200.0, 140.0, 100.0, 70.0,
                                48.0,  32.0,  20.0,  12.0,  5.0};

// Simplification tolerance for the "settled" simplified route shown in
// track.geojson -- matches examples/gps_track.cpp (5 m, geodesic).
constexpr double kSimplifyToleranceM = 5.0;

// How far along the route the ETA marker sits / glides to (meters).
constexpr double kEtaDistanceM = 500.0;

// ---- deterministic, locale-independent formatting -------------------------

void append_num(std::string& out, double value, int precision) {
    char buf[64];
    // The program never calls setlocale, so printf uses the "C" locale: a '.'
    // decimal separator and correctly-rounded fixed output on every libc.
    std::snprintf(buf, sizeof buf, "%.*f", precision, value);
    out += buf;
}

// A GeoJSON position: [longitude, latitude] with ~1 cm precision.
void append_position(std::string& out, const geo::LatLng& p) {
    out += '[';
    append_num(out, p.lng, 7);
    out += ", ";
    append_num(out, p.lat, 7);
    out += ']';
}

void append_positions(std::string& out, const std::vector<geo::LatLng>& pts) {
    out += '[';
    for (std::size_t i = 0; i < pts.size(); ++i) {
        if (i != 0) out += ", ";
        append_position(out, pts[i]);
    }
    out += ']';
}

// ---- GeoJSON feature helpers ----------------------------------------------

void append_linestring(std::string& out, const std::string& properties,
                       const std::vector<geo::LatLng>& pts) {
    out += "    {\n      \"type\": \"Feature\",\n      \"properties\": ";
    out += properties;
    out += ",\n      \"geometry\": {\"type\": \"LineString\", \"coordinates\": ";
    append_positions(out, pts);
    out += "}\n    }";
}

void append_point(std::string& out, const std::string& properties,
                  const geo::LatLng& p) {
    out += "    {\n      \"type\": \"Feature\",\n      \"properties\": ";
    out += properties;
    out += ",\n      \"geometry\": {\"type\": \"Point\", \"coordinates\": ";
    append_position(out, p);
    out += "}\n    }";
}

void append_polygon_ring(std::string& out, const std::string& properties,
                         const std::vector<geo::LatLng>& ring) {
    out += "    {\n      \"type\": \"Feature\",\n      \"properties\": ";
    out += properties;
    out += ",\n      \"geometry\": {\"type\": \"Polygon\", \"coordinates\": [";
    append_positions(out, ring);
    out += "]}\n    }";
}

std::string prop_num(const char* key, double value, int precision) {
    std::string s = "\"";
    s += key;
    s += "\": ";
    append_num(s, value, precision);
    return s;
}

// ---- pipeline -------------------------------------------------------------

// Everything the renderers need, computed once through the real API.
struct Pipeline {
    std::vector<geo::LatLng> track;        // 95 raw points
    std::vector<geo::LatLng> simplified;   // Douglas-Peucker @ 5 m, geodesic
    geo::LatLngBounds box;                 // axis-aligned bounds of the track
    geo::LatLng fix;                       // an off-road GPS fix
    geo::PathProjection snap;              // fix snapped to the route
    geo::LatLng eta;                       // point 500 m along the route
    double track_length_m;
    double simplified_length_m;
};

Pipeline run_pipeline() {
    std::vector<geo::LatLng> track = geo::decode(kTrack);
    std::vector<geo::LatLng> simplified =
        geo::simplify(track, kSimplifyToleranceM, /*geodesic=*/true);
    // bounds() and closest_point_on_path() return optionals; the track is a
    // fixed 95-point input, so both are always present here.
    geo::LatLngBounds box = *geo::bounds(track);
    // The same synthetic ~15 m off-road fix examples/gps_track.cpp uses.
    geo::LatLng fix(track[40].lat + 0.00012, track[40].lng - 0.00008);
    geo::PathProjection snap = *geo::closest_point_on_path(fix, simplified);
    geo::LatLng eta = *geo::point_at_distance(simplified, kEtaDistanceM);
    return Pipeline{track,
                    simplified,
                    box,
                    fix,
                    snap,
                    eta,
                    geo::path_length(track),
                    geo::path_length(simplified)};
}

// Corner ring of a bounds box, closed (SW -> SE -> NE -> NW -> SW).
std::vector<geo::LatLng> box_ring(const geo::LatLngBounds& b) {
    const geo::LatLng sw = b.southwest;
    const geo::LatLng ne = b.northeast;
    return {sw,
            geo::LatLng(sw.lat, ne.lng),
            ne,
            geo::LatLng(ne.lat, sw.lng),
            sw};
}

// ---- emitters -------------------------------------------------------------

// default: one GeoJSON FeatureCollection with the whole scene.
void emit_track_geojson() {
    const Pipeline p = run_pipeline();

    std::string out;
    out += "{\n  \"type\": \"FeatureCollection\",\n";
    out += "  \"generator\": \"geo-utils-cpp tools/assets/emit.cpp\",\n";
    out += "  \"features\": [\n";

    std::vector<std::string> features;

    {  // raw track
        std::string props = "{\"role\": \"track\", ";
        props += prop_num("count", static_cast<double>(p.track.size()), 0);
        props += ", " + prop_num("length_m", p.track_length_m, 2) + "}";
        std::string f;
        append_linestring(f, props, p.track);
        features.push_back(f);
    }
    {  // axis-aligned bounds box
        std::string props = "{\"role\": \"bounds\", \"center\": ";
        append_position(props, p.box.center());
        props += "}";
        std::string f;
        append_polygon_ring(f, props, box_ring(p.box));
        features.push_back(f);
    }
    {  // simplified route
        std::string props = "{\"role\": \"simplified\", ";
        props += prop_num("tolerance_m", kSimplifyToleranceM, 1);
        props += ", " + prop_num("count", static_cast<double>(p.simplified.size()), 0);
        props += ", " + prop_num("length_m", p.simplified_length_m, 2) + "}";
        std::string f;
        append_linestring(f, props, p.simplified);
        features.push_back(f);
    }
    {  // off-road GPS fix
        std::string f;
        append_point(f, "{\"role\": \"fix\"}", p.fix);
        features.push_back(f);
    }
    {  // snapped point on the route
        std::string props = "{\"role\": \"snapped\", ";
        props += prop_num("segment", static_cast<double>(p.snap.segment), 0);
        props += ", " + prop_num("distance_m", p.snap.distance, 3) + "}";
        std::string f;
        append_point(f, props, p.snap.point);
        features.push_back(f);
    }
    {  // the perpendicular "snap link" from the fix to the route
        std::string f;
        append_linestring(f, "{\"role\": \"snap_link\"}", {p.fix, p.snap.point});
        features.push_back(f);
    }
    {  // point 500 m along the route (the ETA marker)
        std::string props = "{\"role\": \"eta\", ";
        props += prop_num("distance_m", kEtaDistanceM, 1) + "}";
        std::string f;
        append_point(f, props, p.eta);
        features.push_back(f);
    }

    for (std::size_t i = 0; i < features.size(); ++i) {
        out += features[i];
        out += (i + 1 == features.size()) ? "\n" : ",\n";
    }
    out += "  ]\n}\n";
    std::fwrite(out.data(), 1, out.size(), stdout);
}

// --dp-frames: one JSON object per line, a Douglas-Peucker frame per tolerance.
void emit_dp_frames() {
    const std::vector<geo::LatLng> track = geo::decode(kTrack);
    std::string out;
    for (double tol : kDpTolerances) {
        std::vector<geo::LatLng> simplified =
            geo::simplify(track, tol, /*geodesic=*/true);
        out += "{\"tolerance_m\": ";
        append_num(out, tol, 1);
        out += ", \"count\": ";
        append_num(out, static_cast<double>(simplified.size()), 0);
        out += ", \"points\": ";
        append_positions(out, simplified);
        out += "}\n";
    }
    std::fwrite(out.data(), 1, out.size(), stdout);
}

// --eta-frames N: N positions marching 0..500 m along the route (inclusive).
void emit_eta_frames(int frames) {
    if (frames < 2) frames = 2;
    const Pipeline p = run_pipeline();
    std::string out;
    for (int i = 0; i < frames; ++i) {
        const double d = kEtaDistanceM * static_cast<double>(i) /
                         static_cast<double>(frames - 1);
        const geo::LatLng pos = *geo::point_at_distance(p.simplified, d);
        out += "{\"index\": ";
        append_num(out, static_cast<double>(i), 0);
        out += ", \"distance_m\": ";
        append_num(out, d, 2);
        out += ", \"point\": ";
        append_position(out, pos);
        out += "}\n";
    }
    std::fwrite(out.data(), 1, out.size(), stdout);
}

void usage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s [--dp-frames | --eta-frames N]\n"
                 "  (no flag)       emit track.geojson (FeatureCollection) to stdout\n"
                 "  --dp-frames     emit dp.jsonl  (Douglas-Peucker sweep) to stdout\n"
                 "  --eta-frames N  emit eta.jsonl (N ETA positions) to stdout\n",
                 argv0);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 1) {
        emit_track_geojson();
        return 0;
    }
    if (std::strcmp(argv[1], "--dp-frames") == 0) {
        emit_dp_frames();
        return 0;
    }
    if (std::strcmp(argv[1], "--eta-frames") == 0) {
        const int frames = (argc >= 3) ? std::atoi(argv[2]) : 60;
        emit_eta_frames(frames);
        return 0;
    }
    usage(argv[0]);
    return 1;
}
