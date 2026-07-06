#!/usr/bin/env bash
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
#
# One entry point for the README asset pipeline: build the C++ emitter, run it
# to regenerate the committed GeoJSON/JSONL, then run each renderer. All the
# geo-math lives in tools/assets/emit.cpp; the Python scripts only draw.
#
# Stages are split by reproducibility:
#   DETERMINISTIC (drift-gated in CI): the emitter data, benchmarks.svg,
#       hero-pipeline.gif, gallery/simplify.gif, social-preview.png.
#   MANUAL (committed by hand, NOT gated): demo.gif (VHS terminal capture) and
#       the four gallery/*.png stills (fetch map tiles). Pass --all to also
#       regenerate these when the tools and network are available.
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
"$PYTHON" tools/render/hero.py     # -> hero-pipeline.gif + gallery/simplify.gif
"$PYTHON" tools/render/social.py   # -> docs/assets/social-preview.png

echo "==> [4/4] non-deterministic renders"
if $want_all; then
    if command -v vhs >/dev/null 2>&1; then
        vhs docs/assets/demo.tape
    else
        echo "    skip demo.gif: vhs not installed (brew install vhs ttyd ffmpeg)"
    fi
    "$PYTHON" tools/render/gallery.py || true
else
    echo "    skipped (pass --all to regenerate). By hand:"
    echo "      demo.gif   : vhs docs/assets/demo.tape                 (needs vhs)"
    echo "      gallery/*  : $PYTHON tools/render/gallery.py           (needs staticmap/cartopy + network)"
fi

echo
echo "Done."
echo "  deterministic (CI drift-gated): data/*, benchmarks.svg, hero-pipeline.gif, gallery/simplify.gif, social-preview.png"
echo "  manual (committed by hand):     demo.gif, gallery/*.png"
