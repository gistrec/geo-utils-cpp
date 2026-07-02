// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0
//
// libFuzzer harness for geo::decode. Two properties on arbitrary input:
//  - decoding is memory-safe and free of undefined behavior (enforced by
//    ASan/UBSan, which the target is always built with);
//  - re-encoding what was decoded preserves the point count, and — when
//    every decoded point is a plausible coordinate — reproduces the points
//    exactly, since decoded points lie on the 1e-5-degree grid.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <geo/encoding.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::string_view input(reinterpret_cast<const char*>(data), size);
    const std::vector<geo::LatLng> decoded = geo::decode(input);

    const std::string reencoded = geo::encode(decoded);
    const std::vector<geo::LatLng> redecoded = geo::decode(reencoded);
    if (redecoded.size() != decoded.size()) {
        std::abort();
    }

    // Malformed input can decode to coordinate jumps beyond the format's
    // 32-bit delta range, which do not survive re-encoding; the exact
    // round-trip is only guaranteed for plausible coordinates.
    bool all_valid = true;
    for (const auto& point : decoded) {
        all_valid = all_valid && point.is_valid();
    }
    if (all_valid) {
        for (std::size_t i = 0; i < decoded.size(); ++i) {
            // Exact comparison on purpose: both sides are int32 * 1e-5.
            if (decoded[i].lat != redecoded[i].lat || decoded[i].lng != redecoded[i].lng) {
                std::abort();
            }
        }
    }
    return 0;
}
