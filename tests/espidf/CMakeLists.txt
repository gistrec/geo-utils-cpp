# Smoke project for the ESP-IDF component build (.github/workflows/embedded.yml):
# main/idf_component.yml pulls the repo root in as a local component, which
# exercises idf_component.yml and the ESP_PLATFORM block in the top-level
# CMakeLists.txt with the real IDF toolchain.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(geo_utils_cpp_smoke)
