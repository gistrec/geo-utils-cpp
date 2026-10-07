# geo-utils-cpp — WebAssembly demo

An interactive map page where every number is computed by the C++ library compiled to
WebAssembly: distance & bearing, offset, geofence (point-in-polygon), polyline
encode/decode (precision 5 and 6), Douglas–Peucker simplify, snap-to-route, and spherical area.
Client-side only: no backend, no API keys, no trackers. Map tiles come from OpenStreetMap;
Leaflet 1.9.4 (BSD-2) is vendored in `web/vendor/leaflet/`, so the only external request is for tiles.

## Layout

| File | Purpose |
|---|---|
| `demo_api.hpp` | Thin C++17 layer over `<geo/geo.hpp>`; paths are flat `[lat, lng, ...]` arrays. No Emscripten dependency. |
| `bindings.cpp` | embind wrapper — one forwarding function per `demo_api.hpp` function. |
| `test/native_ref.cpp` | Native build of the same layer; prints reference values as JSON. |
| `test/verify.mjs` | Loads the WASM module in Node and compares it to the native reference. |
| `web/index.html` | The page (vanilla JS + Leaflet). |

## Build

Needs the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
(`emcmake` on `PATH`, e.g. after `source ./emsdk_env.sh`) and CMake ≥ 3.16.

```sh
demo/build.sh          # -> demo/web/geo.js + demo/web/geo.wasm
ls -l demo/web/geo.wasm
```

Build flags: `-O3 --bind -sMODULARIZE -sEXPORT_ES6 -sALLOW_MEMORY_GROWTH`.

## Run locally

```sh
cd demo/web && python3 -m http.server 8000   # open http://localhost:8000
```

Browsers refuse to load a `.wasm` module from `file://`, so opening `index.html` directly shows
an explanatory message instead of the demo. Any static server works.

## Verify the WASM build against native

```sh
c++ -std=c++17 -Iinclude demo/test/native_ref.cpp -o native_ref && ./native_ref > expected.json
node demo/test/verify.mjs expected.json
```

## Publish on GitHub Pages

`.github/workflows/demo.yml` builds, verifies and deploys `demo/web` on pushes to `master`.
One-time setup: **Settings → Pages → Build and deployment → Source: GitHub Actions**.
