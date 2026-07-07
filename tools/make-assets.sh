#!/usr/bin/env bash
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
#
# One entry point for the README asset pipeline: build the C++ emitter, run it
# to regenerate the committed GeoJSON/JSONL, then run each renderer. All the
# geo-math lives in tools/assets/emit.cpp; the Python scripts only draw.
#
# Stages are split by reproducibility:
#   BYTE-GATED IN CI: benchmarks.svg only (platform-independent SVG).
#   RENDERED, HAND-COMMITTED (deterministic): the emitter data,
#       hero-pipeline.gif and social-preview.png -- rendered but not byte-gated.
#   TILE RENDERS (non-deterministic, need network): gallery/simplify.gif and
#       the four gallery/*.png stills. Pass --all to regenerate when network is up.
#
# Usage:
#   bash tools/make-assets.sh          # emitter + deterministic renders
#   bash tools/make-assets.sh --all    # also demo.gif and the gallery stills
#
# The Python renderers need the pinned deps in tools/render/requirements.txt.
# Point PYTHON at an interpreter that has them (e.g. a venv):
#   pip install -r tools/render/requirements.txt
#   PYTHON=.venv/bin/python bash tools/make-assets.sh

set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(cd "$here/.." && pwd)
cd "$root"

PYTHON=${PYTHON:-python3}
CXX=${CXX:-g++}
want_all=false
[[ "${1:-}" == "--all" ]] && want_all=true

if ! "$PYTHON" -c "import matplotlib, PIL" >/dev/null 2>&1; then
    echo "error: '$PYTHON' is missing matplotlib/pillow." >&2
    echo "       pip install -r tools/render/requirements.txt" >&2
    echo "       (or re-run with PYTHON=/path/to/venv/bin/python)" >&2
    exit 1
fi

echo "==> [1/4] build the emitter (C++17, header-only)"
mkdir -p build
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tools/assets/emit.cpp -o build/emit

echo "==> [2/4] emit data  (deterministic)"
build/emit                 > docs/assets/data/track.geojson
build/emit --dp-frames     > docs/assets/data/dp.jsonl
build/emit --eta-frames 60 > docs/assets/data/eta.jsonl
echo "    docs/assets/data/{track.geojson, dp.jsonl, eta.jsonl}"

echo "==> [3/4] deterministic renders  (matplotlib)"
"$PYTHON" tools/render/bench.py    # -> docs/assets/benchmarks.svg
"$PYTHON" tools/render/hero.py     # -> hero-pipeline.gif
"$PYTHON" tools/render/social.py   # -> docs/assets/social-preview.png

echo "==> [4/4] tile renders  (non-deterministic, committed by hand)"
if $want_all; then
    "$PYTHON" tools/render/gallery.py || true
else
    echo "    skipped (pass --all to regenerate). By hand:"
    echo "      gallery/*  : $PYTHON tools/render/gallery.py   (simplify.gif + stills; needs staticmap/cartopy + network)"
fi

echo
echo "Done."
echo "  byte-gated in CI:               benchmarks.svg"
echo "  rendered here (hand-committed): data/*, hero-pipeline.gif, social-preview.png"
echo "  needs --all + network:          gallery/simplify.gif + gallery/*.png stills"
