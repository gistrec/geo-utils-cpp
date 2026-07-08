# Native Meson build for geo-utils-cpp.
#
# The library is header-only, so this does three things:
#   * expose an internal dependency (declare_dependency) so the project can be
#     consumed as a Meson subproject / WrapDB wrap,
#   * install the headers for a system-wide `meson install`,
#   * optionally build the self-verifying example as a smoke test.
#
# Keep `version` in sync with CMakeLists.txt's project(VERSION ...) and
# include/geo/version.hpp -- the test suite asserts they agree.
project(
  'geo-utils-cpp',
  'cpp',
  version: '1.2.2',
  license: 'Apache-2.0',
  default_options: ['cpp_std=c++17'],
  meson_version: '>=0.56.0',
)

geo_utils_cpp_dep = declare_dependency(
  include_directories: include_directories('include'),
)

# Lets `dependency('geo-utils-cpp')` resolve without a find_package /
# pkg-config round-trip when this repo is used as a subproject.
meson.override_dependency('geo-utils-cpp', geo_utils_cpp_dep)

install_subdir(
  'include/geo',
  install_dir: get_option('includedir'),
)

# Build the example only when standalone (or explicitly asked), so downstream
# consumers don't compile our example into their build tree.
tests_opt = get_option('tests')
if not tests_opt.disabled() and (not meson.is_subproject() or tests_opt.enabled())
  # examples/gps_track.cpp asserts its own results and returns non-zero on
  # failure, so it doubles as an end-to-end smoke test.
  smoke = executable(
    'gps_track',
    'examples/gps_track.cpp',
    dependencies: geo_utils_cpp_dep,
  )
  test('gps_track', smoke)
endif
