#!/usr/bin/env python3
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
"""Renders the README benchmark bar chart -> docs/assets/benchmarks.svg.

Three stacked horizontal-bar panels (``distance_between``, ``area``,
``point_at_distance``). geo-utils-cpp is drawn in the brand green; every
competitor is grey. The throughput numbers are hard-coded from
``docs/benchmarks.md`` -- keep the two in sync (see NUMBERS below).

Deterministic by construction so the CI drift-gate is meaningful:
  * ``svg.hashsalt`` is fixed          -> stable gradient/clip element ids;
  * ``svg.fonttype = "path"``          -> glyphs are embedded as outlines, so
                                          the output does not depend on which
                                          fonts the renderer has installed;
  * ``metadata={"Date": None}``        -> no creation timestamp in the file.
matplotlib bundles FreeType 2.6.1 in its wheels, so a pinned matplotlib
(see requirements.txt) reproduces byte-identical output across machines.

Usage: tools/render/bench.py [-o docs/assets/benchmarks.svg]
"""

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
# Determinism knobs -- must be set before pyplot builds any figure.
matplotlib.rcParams["svg.hashsalt"] = "geo-utils-cpp"
matplotlib.rcParams["svg.fonttype"] = "path"
matplotlib.rcParams["path.simplify"] = False
matplotlib.rcParams["font.family"] = "DejaVu Sans"  # matplotlib's bundled font

import matplotlib.pyplot as plt  # noqa: E402  (after rcParams)

# Repo root (…/geo-utils-cpp), two levels up from tools/render/.
ROOT = Path(__file__).resolve().parent.parent.parent
DEFAULT_OUT = ROOT / "docs" / "assets" / "benchmarks.svg"

GEO_GREEN = "#2f9e44"  # brand green -- geo-utils-cpp bars
GREY = "#adb5bd"       # every competitor / baseline
TEXT = "#212529"
GRID = "#dee2e6"

# Throughput in million operations per second (higher is better).
# KEEP IN SYNC WITH docs/benchmarks.md (median columns quoted below).
# Each panel: (title, unit, [(library, value, is_geo), ...]) in draw order
# (first entry ends up at the top of the panel).
NUMBERS = [
    (
        "distance_between  (N=1 000)",
        "million pairs / s",
        [
            ("geo-utils-cpp", 40.5, True),
            ("naive haversine", 38.3, False),
            ("S2 Geometry", 82.9, False),
            ("Boost.Geometry", 39.8, False),
            ("GeographicLib", 1.2, False),
        ],
    ),
    (
        "area  (polygon N=100)",
        "million polygons / s",
        [
            ("geo-utils-cpp", 67.2, True),
            ("S2 Geometry", 14.0, False),
            ("Boost.Geometry", 36.2, False),
            ("GeographicLib", 2.0, False),
        ],
    ),
    (
        "point_at_distance  (route N=100)",
        "million queries / s",
        [
            ("geo-utils-cpp", 0.79, True),
            ("S2 Geometry", 0.50, False),
        ],
    ),
]


def render(out_path: Path) -> None:
    n_panels = len(NUMBERS)
    fig, axes = plt.subplots(
        n_panels,
        1,
        figsize=(8.0, 6.4),
        gridspec_kw={"height_ratios": [len(rows) for _, _, rows in NUMBERS]},
    )

    for ax, (title, unit, rows) in zip(axes, NUMBERS):
        labels = [name for name, _, _ in rows]
        values = [value for _, value, _ in rows]
        colors = [GEO_GREEN if is_geo else GREY for _, _, is_geo in rows]
        # barh draws bottom-up; reverse so the first row lands on top.
        positions = list(range(len(rows)))[::-1]

        bars = ax.barh(positions, values, color=colors, height=0.66,
                       edgecolor="white", linewidth=0.8, zorder=3)
        ax.set_yticks(positions)
        ax.set_yticklabels(labels, fontsize=9, color=TEXT)
        for name, bar, value, is_geo in (
            (r[0], b, r[1], r[2]) for r, b in zip(rows, bars)
        ):
            ax.text(bar.get_width() + max(values) * 0.012,
                    bar.get_y() + bar.get_height() / 2,
                    f"{value:g}", va="center", ha="left",
                    fontsize=9, color=TEXT,
                    fontweight="bold" if is_geo else "normal")

        ax.set_title(title, fontsize=10.5, color=TEXT, loc="left",
                     fontweight="bold", pad=6)
        ax.set_xlabel(unit, fontsize=8, color="#868e96")
        ax.set_xlim(0, max(values) * 1.16)
        ax.tick_params(axis="x", labelsize=8, colors="#868e96")
        ax.margins(y=0.16)
        ax.xaxis.grid(True, color=GRID, linewidth=0.8, zorder=0)
        ax.set_axisbelow(True)
        for spine in ("top", "right", "left"):
            ax.spines[spine].set_visible(False)
        ax.spines["bottom"].set_color(GRID)

    fig.suptitle("geo-utils-cpp throughput vs S2 / Boost.Geometry / GeographicLib",
                 fontsize=12, fontweight="bold", color=TEXT, x=0.012, ha="left")
    fig.text(0.012, 0.005,
             "Apple M1 / clang 17 / -O2 -DNDEBUG  -  higher is better  -  "
             "numbers from docs/benchmarks.md",
             fontsize=7.5, color="#868e96", ha="left")

    fig.tight_layout(rect=(0.0, 0.02, 1.0, 0.95), h_pad=1.4)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, format="svg", metadata={"Date": None})
    plt.close(fig)
    print(f"wrote {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUT,
                        help="output SVG path")
    args = parser.parse_args()
    render(args.output)


if __name__ == "__main__":
    main()
