// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// Portions of this file are based on Google Maps Android Utils:
// https://github.com/googlemaps/android-maps-utils
//
// Original work:
// Copyright 2013 Google Inc.
// Licensed under the Apache License, Version 2.0
//
// This file has been modified from the original work,
// including a port from Java to C++.

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "latlng.hpp"

namespace geo {

namespace detail {

inline void encode_value(std::int64_t v, std::string& out) {
    // Zig-zag encode in unsigned arithmetic: left-shifting a negative signed
    // value is undefined behavior in C++17.
    std::uint64_t value = static_cast<std::uint64_t>(v) << 1;
    if (v < 0) {
        value = ~value;
    }
    while (value >= 0x20) {
        out += static_cast<char>((0x20 | (value & 0x1f)) + 63);
        value >>= 5;
    }
    out += static_cast<char>(value + 63);
}

}  // namespace detail

/**
 * Encodes a sequence of LatLngs into a string using the Encoded Polyline
 * Algorithm Format. Coordinates are quantized to 1e-5 degrees (about one
 * meter), so an encode/decode round-trip is lossy beyond that precision.
 */
template <typename Path>
[[nodiscard]] std::string encode(const Path& path) {
    std::int64_t last_lat = 0;
    std::int64_t last_lng = 0;
    std::string result;

    for (const auto& point : path) {
        std::int64_t lat = std::llround(point.lat * 1e5);
        std::int64_t lng = std::llround(point.lng * 1e5);

        detail::encode_value(lat - last_lat, result);
        detail::encode_value(lng - last_lng, result);

        last_lat = lat;
        last_lng = lng;
    }
    return result;
}

/**
 * Decodes an Encoded Polyline Algorithm Format string into a sequence of
 * LatLngs on the 1e-5-degree grid the format uses.
 *
 * The input is assumed to be a well-formed encoded polyline. Decoding any
 * string is memory-safe, but malformed input yields unspecified coordinates;
 * a string truncated mid-point yields the points decoded so far and drops
 * the incomplete trailing point.
 */
[[nodiscard]] inline std::vector<LatLng> decode(std::string_view encoded) {
    std::vector<LatLng> path;
    std::size_t index = 0;
    std::int32_t lat = 0;
    std::int32_t lng = 0;

    // Reads one zig-zag/varint-encoded delta and adds it to coord; returns
    // false when the string ends mid-chunk. Accumulates in unsigned
    // arithmetic (well-defined wrap-around), matching the Java original bit
    // for bit on well-formed input.
    const auto decode_delta = [&encoded, &index](std::int32_t& coord) {
        std::uint32_t result = 1;
        unsigned shift = 0;
        std::int32_t b;
        do {
            if (index >= encoded.size()) {
                return false;
            }
            b = static_cast<unsigned char>(encoded[index++]) - 64;
            if (shift < 32) {  // only malformed input reaches shift 35+
                result += static_cast<std::uint32_t>(b) << shift;
            }
            shift += 5;
        } while (b >= 0x1f);
        const auto r = static_cast<std::int32_t>(result);
        // r >> 1 on a negative value is an arithmetic shift on every
        // supported compiler; C++20 makes that guarantee standard.
        const std::int32_t delta = (r & 1) != 0 ? ~(r >> 1) : (r >> 1);
        // Accumulate with unsigned wrap-around: malformed input can push the
        // sum past INT32_MAX, which would be signed-overflow UB. Wrapping
        // matches the Java original's int semantics.
        coord = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(coord) + static_cast<std::uint32_t>(delta));
        return true;
    };

    while (index < encoded.size()) {
        if (!decode_delta(lat) || !decode_delta(lng)) {
            break;
        }
        path.push_back(LatLng(lat * 1e-5, lng * 1e-5));
    }
    return path;
}

}  // namespace geo
