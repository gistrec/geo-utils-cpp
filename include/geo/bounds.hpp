// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0

#pragma once

#include <cstddef>
#include <optional>
#include <ostream>

#include "detail/math.hpp"
#include "latlng.hpp"

namespace geo {

/**
 * A latitude/longitude aligned rectangle, delimited by its south-west and
 * north-east corners (degrees).
 *
 * The longitude span runs EASTWARD from southwest.lng to northeast.lng, so
 * bounds may cross the antimeridian: southwest.lng > northeast.lng describes
 * exactly that (e.g. sw lng 170, ne lng -170 covers lng in [170, 180] and
 * [-180, -170]). Equal longitudes describe a single meridian, not the whole
 * circle. As an eastern limit prefer +180 and as a western one -180 — the
 * two spellings denote the same meridian but produce different spans.
 *
 * southwest.lat <= northeast.lat is expected. As everywhere in the library,
 * nothing is validated — use is_valid() for untrusted input.
 */
struct LatLngBounds {
    LatLng southwest;
    LatLng northeast;

    constexpr LatLngBounds(const LatLng& sw, const LatLng& ne) noexcept
        : southwest(sw), northeast(ne) {}

    /**
     * Returns whether both corners are valid coordinates and the latitudes
     * are ordered (southwest.lat <= northeast.lat).
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return southwest.is_valid() && northeast.is_valid() &&
               southwest.lat <= northeast.lat;
    }

    /**
     * Eastward longitude span in degrees, in [0, 360).
     */
    [[nodiscard]] double lng_span() const noexcept {
        return detail::mod(northeast.lng - southwest.lng, 360.0);
    }

    /**
     * Returns whether the point lies inside the bounds (boundaries
     * inclusive). Longitudes are compared modulo 360, so 180 and -180 are
     * interchangeable here.
     */
    [[nodiscard]] bool contains(const LatLng& point) const noexcept {
        if (point.lat < southwest.lat || point.lat > northeast.lat) {
            return false;
        }
        return detail::mod(point.lng - southwest.lng, 360.0) <= lng_span();
    }

    /**
     * Grows the bounds by the smallest amount that makes them contain the
     * point — the same incremental semantics as the Android LatLngBounds
     * builder. Longitude ties (the point is equally far east and west) are
     * resolved eastward. Note that the result depends on the order points
     * are added in; for many points hopping more than 180 degrees apart the
     * accumulated span may not be the minimal one.
     */
    void extend(const LatLng& point) noexcept {
        if (point.lat < southwest.lat) {
            southwest.lat = point.lat;
        }
        if (point.lat > northeast.lat) {
            northeast.lat = point.lat;
        }
        if (detail::mod(point.lng - southwest.lng, 360.0) <= lng_span()) {
            return;  // already inside longitude-wise
        }
        const double east = detail::mod(point.lng - northeast.lng, 360.0);
        const double west = detail::mod(southwest.lng - point.lng, 360.0);
        if (east <= west) {
            // Extending east to the meridian 180 == -180: keep the +180
            // spelling, otherwise the eastward span would jump by 360.
            northeast.lng = point.lng == -180.0 ? 180.0 : point.lng;
        } else {
            southwest.lng = point.lng == 180.0 ? -180.0 : point.lng;
        }
    }

    /**
     * Returns the center of the bounds. The longitude midpoint follows the
     * eastward span (antimeridian-aware) and is wrapped to [-180, 180).
     */
    [[nodiscard]] LatLng center() const noexcept {
        return LatLng((southwest.lat + northeast.lat) / 2.0,
                      detail::wrap(southwest.lng + lng_span() / 2.0, -180.0, 180.0));
    }

    /**
     * Returns whether the two bounds share at least one point (touching
     * edges count as intersecting).
     */
    [[nodiscard]] bool intersects(const LatLngBounds& other) const noexcept {
        if (other.northeast.lat < southwest.lat || other.southwest.lat > northeast.lat) {
            return false;
        }
        // Two eastward arcs on a circle overlap iff either arc's western
        // edge lies within the other arc.
        return detail::mod(other.southwest.lng - southwest.lng, 360.0) <= lng_span() ||
               detail::mod(southwest.lng - other.southwest.lng, 360.0) <= other.lng_span();
    }

    /**
     * Corner-wise approximate equality — LatLng::operator== semantics,
     * including longitudes compared modulo 360.
     */
    [[nodiscard]] bool operator==(const LatLngBounds& other) const noexcept {
        return southwest == other.southwest && northeast == other.northeast;
    }

    [[nodiscard]] bool operator!=(const LatLngBounds& other) const noexcept {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const LatLngBounds& b) {
        return os << "LatLngBounds(" << b.southwest << ", " << b.northeast << ")";
    }
};

/**
 * Returns the bounds of the path, built by extending point-by-point in input
 * order (LatLngBounds::extend semantics). Returns std::nullopt for an empty
 * path. Useful as a cheap prefilter in front of contains() / on_path():
 * a point outside bounds(polygon) is guaranteed to be outside the polygon.
 */
template <typename Path>
[[nodiscard]] std::optional<LatLngBounds> bounds(const Path& path) {
    const std::size_t size = path.size();
    if (size == 0U) {
        return std::nullopt;
    }
    const LatLng first(path[0].lat, path[0].lng);
    LatLngBounds out(first, first);
    for (std::size_t i = 1; i < size; ++i) {
        out.extend(LatLng(path[i].lat, path[i].lng));
    }
    return out;
}

}  // namespace geo
