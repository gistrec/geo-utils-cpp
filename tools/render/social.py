#!/usr/bin/env python3
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
"""Renders the 1280x640 social preview -> docs/assets/social-preview.png.

A 2x2 contact sheet over the real emit.cpp output (snap-to-route, the
Douglas-Peucker collapse, the `area` benchmark win, and bounds/viewport math),
wrapped with the wordmark and the "67M polygons/sec" hook. Meant for GitHub ->
Settings -> Social preview, not for the README body.

Deterministic and dependency-light so the CI drift-gate can regenerate it:
it draws committed data with matplotlib (FreeType 2.6.1 is bundled in the
wheels) and does NOT fetch any map tiles -- the benchmark numbers come from
bench.py and the geometry from docs/assets/data/, so there is a single source
of truth for each. The PNG's Software metadata is pinned so the bytes do not
depend on the matplotlib version string.

Usage: tools/render/social.py [-o docs/assets/social-preview.png]
"""

import argparse
import math
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
matplotlib.rcParams["path.simplify"] = False
matplotlib.rcParams["font.family"] = "DejaVu Sans"

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyBboxPatch  # noqa: E402

# Reuse the benchmark numbers (bench.py) and the data loader (hero.py) so the
# social card never drifts from the chart or the emitter output.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from bench import NUMBERS  # noqa: E402
from hero import load_scene, xs, ys  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent.parent
DEFAULT_OUT = ROOT / "docs" / "assets" / "social-preview.png"

BG = "#0d1117"
PANEL = "#111826"
TRACK = "#586372"
GREEN = "#3fb950"
BOX = "#8b949e"
FIX = "#f0883e"
ETA = "#58a6ff"
GREY = "#39414d"
TEXT = "#e6edf3"
SUBTLE = "#8b949e"
CODE = "#7ee787"


def panel_axes(ax, title):
    ax.set_facecolor(PANEL)
    ax.set_xticks([])
    ax.set_yticks([])
    for s in ax.spines.values():
        s.set_visible(False)
    ax.set_title(title, color=TEXT, fontsize=13, fontweight="bold",
                 loc="left", pad=4)


def map_limits(ax, scene, pad=0.10):
    lng = xs(scene["track"]) + xs(scene["bounds"])
    lat = ys(scene["track"]) + ys(scene["bounds"])
    dx = (max(lng) - min(lng)) * pad
    dy = (max(lat) - min(lat)) * pad
    ax.set_xlim(min(lng) - dx, max(lng) + dx)
    ax.set_ylim(min(lat) - dy, max(lat) + dy)
    ax.set_aspect(1.0 / math.cos(math.radians(0.5 * (min(lat) + max(lat)))))


def panel_snap(ax, scene):
    panel_axes(ax, "snap to route")
    map_limits(ax, scene)
    ax.plot(xs(scene["track"]), ys(scene["track"]), color=TRACK, lw=1.0,
            alpha=0.55, zorder=2)
    ax.plot(xs(scene["simplified"]), ys(scene["simplified"]), color=GREEN,
            lw=2.4, solid_capstyle="round", zorder=4)
    link = scene["snap_link"]
    ax.plot(xs(link), ys(link), color=FIX, lw=1.4, ls=(0, (2, 2)), zorder=5)
    ax.plot([scene["fix"][0]], [scene["fix"][1]], marker="o", markersize=8,
            markerfacecolor=FIX, markeredgecolor=BG, markeredgewidth=1.1, zorder=6)
    ax.plot([scene["snapped"][0]], [scene["snapped"][1]], marker="o",
            markersize=8, markerfacecolor=GREEN, markeredgecolor=BG,
            markeredgewidth=1.1, zorder=6)
    ax.plot([scene["eta"][0]], [scene["eta"][1]], marker="o", markersize=8,
            markerfacecolor=ETA, markeredgecolor=BG, markeredgewidth=1.1, zorder=6)


def panel_simplify(ax, scene):
    panel_axes(ax, "Douglas-Peucker simplify")
    map_limits(ax, scene)
    ax.plot(xs(scene["track"]), ys(scene["track"]), color=TRACK, lw=1.0,
            alpha=0.55, zorder=2)
    # A recognizable mid-coarse frame from the sweep (~8 vertices).
    frame = min(scene["dp_frames"], key=lambda f: abs(int(f["count"]) - 8))
    pts = frame["points"]
    ax.plot(xs(pts), ys(pts), color=GREEN, lw=2.4, solid_capstyle="round",
            zorder=4)
    ax.plot(xs(pts), ys(pts), linestyle="none", marker="o", markersize=5,
            markerfacecolor=GREEN, markeredgecolor=BG, markeredgewidth=0.8,
            zorder=5)
    ax.text(0.03, 0.06, f"95 -> {int(frame['count'])} pts", transform=ax.transAxes,
            color=CODE, fontsize=11, family="monospace", ha="left", va="bottom")


def panel_bounds(ax, scene):
    panel_axes(ax, "bounds / viewport")
    map_limits(ax, scene)
    ax.plot(xs(scene["track"]), ys(scene["track"]), color=GREEN, lw=1.6,
            alpha=0.9, zorder=3)
    ring = scene["bounds"]
    ax.plot(xs(ring), ys(ring), color=BOX, lw=1.4, ls=(0, (6, 4)), zorder=4)
    cx = sum(xs(ring)) / len(ring)
    cy = sum(ys(ring)) / len(ring)
    ax.plot([cx], [cy], marker="+", markersize=11, markeredgecolor=ETA,
            markeredgewidth=1.6, zorder=5)


def panel_bench(ax):
    # The standout win: `area` throughput (M polygons/s). Library names sit
    # above each bar (inside the axes) so nothing clips at the panel edge.
    _, _, rows = NUMBERS[1]
    panel_axes(ax, "area benchmark  (M polygons/s)")
    values = [r[1] for r in rows]
    positions = list(range(len(rows)))[::-1]
    ax.set_xlim(0, max(values) * 1.22)
    ax.set_ylim(-0.6, len(rows) - 0.4)
    ax.set_yticks([])
    ax.tick_params(axis="x", colors=SUBTLE, labelsize=8)
    for (name, val, is_geo), pos in zip(rows, positions):
        color = GREEN if is_geo else GREY
        ax.barh(pos, val, height=0.40, color=color, edgecolor=BG,
                linewidth=0.6, zorder=3)
        ax.text(0, pos + 0.30, name, va="bottom", ha="left", fontsize=9,
                color=GREEN if is_geo else TEXT,
                fontweight="bold" if is_geo else "normal")
        ax.text(val + max(values) * 0.02, pos, f"{val:g}", va="center",
                ha="left", fontsize=9.5, color=TEXT,
                fontweight="bold" if is_geo else "normal")


def render(out_path):
    scene = load_scene()
    fig = plt.figure(figsize=(12.8, 6.4), dpi=100)
    fig.patch.set_facecolor(BG)
    gs = fig.add_gridspec(2, 2, left=0.035, right=0.965, top=0.66, bottom=0.06,
                          hspace=0.32, wspace=0.13)

    # --- header band -------------------------------------------------------
    head = fig.add_axes([0, 0.70, 1, 0.30])
    head.set_axis_off()
    head.set_xlim(0, 1)
    head.set_ylim(0, 1)
    head.text(0.035, 0.62, "geo-utils-cpp", color=TEXT, fontsize=40,
              fontweight="bold", va="center", ha="left")
    head.text(0.037, 0.20,
              "practical lat/lng geometry for C++17  -  header-only, no dependencies",
              color=SUBTLE, fontsize=15, va="center", ha="left")
    # The "67M polygons/sec" hook as a green badge.
    badge = FancyBboxPatch((0.74, 0.34), 0.22, 0.44,
                           boxstyle="round,pad=0.012,rounding_size=0.03",
                           mutation_aspect=0.5, facecolor=GREEN, edgecolor="none",
                           transform=head.transData, zorder=3)
    head.add_patch(badge)
    head.text(0.85, 0.66, "67M", color="#04240d", fontsize=26, fontweight="bold",
              va="center", ha="center", zorder=4)
    head.text(0.85, 0.44, "polygons / sec", color="#04240d", fontsize=12,
              fontweight="bold", va="center", ha="center", zorder=4)

    # --- 2x2 panels over real emit.cpp output ------------------------------
    panel_snap(fig.add_subplot(gs[0, 0]), scene)
    panel_simplify(fig.add_subplot(gs[0, 1]), scene)
    panel_bench(fig.add_subplot(gs[1, 0]))
    panel_bounds(fig.add_subplot(gs[1, 1]), scene)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    # Fixed Software tag => bytes don't depend on the matplotlib version string.
    fig.savefig(out_path, format="png", dpi=100, facecolor=BG,
                metadata={"Software": "geo-utils-cpp asset pipeline"})
    plt.close(fig)
    print(f"wrote {out_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUT,
                        help="output PNG path")
    args = parser.parse_args()
    render(args.output)


if __name__ == "__main__":
    main()
