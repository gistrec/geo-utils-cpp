#include <gtest/gtest.h>

#include <geo/version.hpp>

// The version macros are usable in preprocessor and constant expressions.
static_assert(GEO_UTILS_CPP_VERSION ==
              GEO_UTILS_CPP_VERSION_MAJOR * 10000 +
              GEO_UTILS_CPP_VERSION_MINOR * 100 +
              GEO_UTILS_CPP_VERSION_PATCH);
#if GEO_UTILS_CPP_VERSION < 10100
#error "version macro must be usable in #if"
#endif

TEST(Version, matches_cmake_project_version) {
    // GEO_UTILS_CPP_TEST_CMAKE_VERSION is injected by the test CMakeLists
    // from ${PROJECT_VERSION}. If this fails, a release bumped one version
    // and not the other — see RELEASING.md.
    EXPECT_STREQ(GEO_UTILS_CPP_VERSION_STRING, GEO_UTILS_CPP_TEST_CMAKE_VERSION);
}

TEST(Version, matches_embedded_manifests) {
    // library.properties (Arduino) and library.json (PlatformIO) carry their
    // own copy of the version; the test CMakeLists reads them into these
    // macros. idf_component.yml has none: the publish workflow passes the tag.
    EXPECT_STREQ(GEO_UTILS_CPP_VERSION_STRING, GEO_UTILS_CPP_TEST_ARDUINO_VERSION);
    EXPECT_STREQ(GEO_UTILS_CPP_VERSION_STRING, GEO_UTILS_CPP_TEST_PLATFORMIO_VERSION);
}
