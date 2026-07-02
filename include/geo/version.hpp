// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0

#pragma once

// Library version, kept in sync with the CMake project() version. This is a
// static header (not configure_file-generated) so that copy-the-headers
// consumers get it too; the test suite asserts it matches the CMake version.
#define GEO_UTILS_CPP_VERSION_MAJOR 1
#define GEO_UTILS_CPP_VERSION_MINOR 1
#define GEO_UTILS_CPP_VERSION_PATCH 0

// Single number for >= comparisons: major * 10000 + minor * 100 + patch,
// e.g. 1.2.3 -> 10203.
#define GEO_UTILS_CPP_VERSION                                        \
    (GEO_UTILS_CPP_VERSION_MAJOR * 10000 + GEO_UTILS_CPP_VERSION_MINOR * 100 + \
     GEO_UTILS_CPP_VERSION_PATCH)

#define GEO_UTILS_CPP_VERSION_STR_(x) #x
#define GEO_UTILS_CPP_VERSION_STR(x) GEO_UTILS_CPP_VERSION_STR_(x)

// "major.minor.patch", e.g. "1.1.0".
#define GEO_UTILS_CPP_VERSION_STRING                  \
    GEO_UTILS_CPP_VERSION_STR(GEO_UTILS_CPP_VERSION_MAJOR) \
    "." GEO_UTILS_CPP_VERSION_STR(GEO_UTILS_CPP_VERSION_MINOR) \
    "." GEO_UTILS_CPP_VERSION_STR(GEO_UTILS_CPP_VERSION_PATCH)
