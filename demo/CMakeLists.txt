cmake_minimum_required(VERSION 3.16)
project(geo_utils_cpp_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GEO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")

if(EMSCRIPTEN)
  # WebAssembly module: web/geo.js + web/geo.wasm
  add_executable(geo bindings.cpp)
  target_include_directories(geo PRIVATE "${GEO_INCLUDE_DIR}")
  target_compile_options(geo PRIVATE -O3 -Wall -Wextra)
  target_link_options(geo PRIVATE
    --bind -O3
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createGeo
    -sALLOW_MEMORY_GROWTH=1
    -sENVIRONMENT=web,node
    -sASSERTIONS=0)
  set_target_properties(geo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
    SUFFIX ".js")
else()
  # Native reference used to cross-check the WebAssembly build (test/verify.mjs).
  add_executable(native_ref test/native_ref.cpp)
  target_include_directories(native_ref PRIVATE "${GEO_INCLUDE_DIR}")
  target_compile_options(native_ref PRIVATE -Wall -Wextra -pedantic)
endif()
