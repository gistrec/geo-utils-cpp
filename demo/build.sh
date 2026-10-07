#!/usr/bin/env bash
# Builds web/geo.js + web/geo.wasm. Requires the Emscripten SDK (emcmake on PATH).
set -euo pipefail
cd "$(dirname "$0")"

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake not found. Install and activate the Emscripten SDK first:" >&2
  echo "  https://emscripten.org/docs/getting_started/downloads.html  (source ./emsdk_env.sh)" >&2
  exit 1
fi

emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --parallel
ls -l web/geo.js web/geo.wasm
