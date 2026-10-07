// Thin, JS-friendly layer over geo-utils-cpp. Paths are flat arrays
// [lat0, lng0, lat1, lng1, ...]. Pure C++17 with no Emscripten dependency, so the
// exact same code is compiled natively (test/native_ref.cpp) to cross-check the
// WebAssembly build.
#pragma once

#include <geo/geo.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace demo {

using Flat = std::vector<double>;

inline std::vector<geo::LatLng> to_path(const Flat& flat) {
    std::vector<geo::LatLng> path;
    path.reserve(flat.size() / 2);
    for (std::size_t i = 0; i + 1 < flat.size(); i += 2) {
        path.emplace_back(flat[i], flat[i + 1]);
    }
    return path;
}

inline Flat to_flat(const std::vector<geo::LatLng>& path) {
    Flat flat;
    flat.reserve(path.size() * 2);
    for (const auto& p : path) {
        flat.push_back(p.lat);
        flat.push_back(p.lng);
    }
    return flat;
}

inline std::string version() { return GEO_UTILS_CPP_VERSION_STRING; }

inline double distance_between(double lat1, double lng1, double lat2, double lng2) {
    return geo::distance_between({lat1, lng1}, {lat2, lng2});
}

inline double heading(double lat1, double lng1, double lat2, double lng2) {
    return geo::heading({lat1, lng1}, {lat2, lng2});
}

inline Flat offset(double lat, double lng, double distance, double heading_deg) {
    const geo::LatLng p = geo::offset({lat, lng}, distance, heading_deg);
    return {p.lat, p.lng};
}

inline Flat interpolate(double lat1, double lng1, double lat2, double lng2, double fraction) {
    const geo::LatLng p = geo::interpolate({lat1, lng1}, {lat2, lng2}, fraction);
    return {p.lat, p.lng};
}

inline double path_length(const Flat& flat) { return geo::path_length(to_path(flat)); }

// Empty result for an empty path.
inline Flat point_at_distance(const Flat& flat, double distance) {
    const auto p = geo::point_at_distance(to_path(flat), distance);
    return p ? Flat{p->lat, p->lng} : Flat{};
}

inline double area(const Flat& flat) { return geo::area(to_path(flat)); }

inline bool contains(double lat, double lng, const Flat& polygon, bool geodesic) {
    return geo::contains({lat, lng}, to_path(polygon), geodesic);
}

inline bool on_edge(double lat, double lng, const Flat& polygon, bool geodesic, double tolerance) {
    return geo::on_edge({lat, lng}, to_path(polygon), geodesic, tolerance);
}

// Distance in meters from the point to the closest polygon edge (the polygon is
// closed implicitly). -1 for an empty polygon.
inline double distance_to_edge(double lat, double lng, const Flat& polygon) {
    auto path = to_path(polygon);
    if (path.empty()) {
        return -1.0;
    }
    if (path.size() > 1) {
        path.push_back(path.front());
    }
    const auto projection = geo::closest_point_on_path({lat, lng}, path);
    return projection ? projection->distance : -1.0;
}

inline std::string encode(const Flat& flat, unsigned precision) {
    return geo::encode(to_path(flat), precision);
}

inline Flat decode(const std::string& encoded, unsigned precision) {
    return to_flat(geo::decode(encoded, precision));
}

inline Flat simplify(const Flat& flat, double tolerance, bool geodesic) {
    return to_flat(geo::simplify(to_path(flat), tolerance, geodesic));
}

// [lat, lng, segment, distance_to_path_m, distance_along_path_m]; empty for an empty path.
inline Flat closest_point_on_path(double lat, double lng, const Flat& flat) {
    const auto path = to_path(flat);
    const auto projection = geo::closest_point_on_path({lat, lng}, path);
    if (!projection) {
        return {};
    }
    const std::vector<geo::LatLng> head(path.begin(), path.begin() + static_cast<std::ptrdiff_t>(projection->segment) + 1);
    const double along = geo::path_length(head) + geo::distance_between(path[projection->segment], projection->point);
    return {projection->point.lat, projection->point.lng, static_cast<double>(projection->segment),
            projection->distance, along};
}

// [south, west, north, east]; empty for an empty path. Antimeridian-aware: west > east
// means the box crosses the 180th meridian.
inline Flat bounds(const Flat& flat) {
    const auto b = geo::bounds(to_path(flat));
    if (!b) {
        return {};
    }
    return {b->southwest.lat, b->southwest.lng, b->northeast.lat, b->northeast.lng};
}

}  // namespace demo
