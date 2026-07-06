#!/usr/bin/env python3
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
"""Renders the animated hero + the standalone Douglas-Peucker gif.

Reads ONLY the JSON that tools/assets/emit.cpp produced from the real
geo-utils-cpp pipeline (docs/assets/data/*.geojson, *.jsonl) and draws it --
no geometry is recomputed in Python. Two outputs:

  docs/assets/hero-pipeline.gif   the storyboard: the raw track draws in ->
                                  bounds box snaps around it -> Douglas-Peucker
                                  collapses it -> an off-road GPS fix drops and
                                  snaps to the route -> an ETA marker glides
                                  500 m up the line, then holds.
  docs/assets/gallery/simplify.gif  a focused Douglas-Peucker collapse across
                                  the tolerance sweep in dp.jsonl.

Both are deterministic (no basemap, matplotlib FuncAnimation -> PillowWriter,
no external binary). matplotlib bundles FreeType 2.6.1 in its wheels and Agg
is self-contained, so a pinned matplotlib (see requirements.txt) reproduces
byte-identical gifs across machines -- which is what makes the CI drift-gate
meaningful.

Usage: tools/render/hero.py [--hero-only | --simplify-only]
"""

import argparse
import json
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
matplotlib.rcParams["path.simplify"] = False
matplotlib.rcParams["font.family"] = "DejaVu Sans"

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.animation import FuncAnimation, PillowWriter  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent.parent
DATA = ROOT / "docs" / "assets" / "data"
OUT_HERO = ROOT / "docs" / "assets" / "hero-pipeline.gif"
OUT_SIMPLIFY = ROOT / "docs" / "assets" / "gallery" / "simplify.gif"

# Dark palette, tuned to read on both a light and a dark README canvas.
BG = "#0d1117"
TRACK = "#586372"       # raw, noisy track
GREEN = "#3fb950"       # simplified route (brand green, brightened for dark bg)
GREEN_DIM = "#2ea043"
BOX = "#8b949e"         # bounds rectangle
FIX = "#f0883e"         # off-road GPS fix (amber)
ETA = "#58a6ff"         # moving point_at_distance marker (blue)
TEXT = "#e6edf3"
SUBTLE = "#8b949e"
CODE = "#7ee787"

FPS = 20
DPI = 100
FIGSIZE = (10.0, 5.6)   # -> 1000 x 560 px


# --------------------------------------------------------------------------
# data loading (drawing only -- all geometry already computed in emit.cpp)
# --------------------------------------------------------------------------

def load_scene():
    gj = json.loads((DATA / "track.geojson").read_text())
    feats = {f["properties"]["role"]: f for f in gj["features"]}

    def line(role):
        return feats[role]["geometry"]["coordinates"]  # [[lng, lat], ...]

    def point(role):
        return feats[role]["geometry"]["coordinates"]

    scene = {
        "track": line("track"),
        "bounds": feats["bounds"]["geometry"]["coordinates"][0],
        "simplified": line("simplified"),
        "fix": point("fix"),
        "snapped": point("snapped"),
        "snap_link": line("snap_link"),
        "eta": point("eta"),
        "snap_props": feats["snapped"]["properties"],
        "simplified_props": feats["simplified"]["properties"],
        "track_props": feats["track"]["properties"],
    }
    scene["eta_frames"] = [
        json.loads(ln)["point"]
        for ln in (DATA / "eta.jsonl").read_text().splitlines() if ln.strip()
    ]
    scene["dp_frames"] = [
        json.loads(ln)
        for ln in (DATA / "dp.jsonl").read_text().splitlines() if ln.strip()
    ]
    return scene


def xs(coords):
    return [c[0] for c in coords]


def ys(coords):
    return [c[1] for c in coords]


def ease(t):
    """Smoothstep -- 0..1 with zero slope at both ends."""
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def partial_polyline(pts, frac):
    """A prefix of pts grown smoothly to fraction ``frac`` of its length."""
    frac = max(0.0, min(1.0, frac))
    n = len(pts)
    if n <= 1 or frac <= 0.0:
        return pts[:1]
    if frac >= 1.0:
        return pts
    span = (n - 1) * frac
    k = int(span)
    r = span - k
    if k >= n - 1:
        return pts
    a, b = pts[k], pts[k + 1]
    mid = [a[0] + r * (b[0] - a[0]), a[1] + r * (b[1] - a[1])]
    return pts[:k + 1] + [mid]


# --------------------------------------------------------------------------
# static frame setup shared by every beat
# --------------------------------------------------------------------------

def compute_limits(scene):
    all_lng = xs(scene["track"]) + xs(scene["bounds"])
    all_lat = ys(scene["track"]) + ys(scene["bounds"])
    lng0, lng1 = min(all_lng), max(all_lng)
    lat0, lat1 = min(all_lat), max(all_lat)
    pad_x = (lng1 - lng0) * 0.10
    pad_y = (lat1 - lat0) * 0.16
    # Extra headroom at the top: the GPS fix drops in from above.
    return (lng0 - pad_x, lng1 + pad_x,
            lat0 - pad_y, lat1 + pad_y * 2.4)


def setup_axes(ax, scene, limits):
    lat_mid = 0.5 * (limits[2] + limits[3])
    ax.clear()
    ax.set_facecolor(BG)
    ax.set_xlim(limits[0], limits[1])
    ax.set_ylim(limits[2], limits[3])
    # A degree of longitude is cos(lat) as long as a degree of latitude; this
    # keeps the little map from looking horizontally stretched.
    ax.set_aspect(1.0 / math.cos(math.radians(lat_mid)))
    ax.set_axis_off()


def draw_chrome(ax, limits, code_line, stage_idx):
    """Wordmark, the current API call, and a five-dot stage tracker."""
    x0, x1, y0, y1 = limits
    ax.text(x0, y1 - (y1 - y0) * 0.045, "geo-utils-cpp",
            color=TEXT, fontsize=17, fontweight="bold", va="top", ha="left")
    ax.text(x0, y1 - (y1 - y0) * 0.115, "real GPS-track pipeline, C++ -> GeoJSON",
            color=SUBTLE, fontsize=9, va="top", ha="left")
    # The live API call, monospaced, bottom-left.
    ax.text(x0, y0 + (y1 - y0) * 0.05, code_line, color=CODE, fontsize=11.5,
            va="bottom", ha="left", family="monospace")
    # Stage dots, bottom-right.
    stages = 5
    for i in range(stages):
        cx = x1 - (y1 - y0) * 0.05 - (stages - 1 - i) * (x1 - x0) * 0.022
        cy = y0 + (y1 - y0) * 0.06
        ax.plot([cx], [cy], marker="o", markersize=6,
                color=GREEN if i <= stage_idx else "#30363d", zorder=10)


# --------------------------------------------------------------------------
# hero timeline
# --------------------------------------------------------------------------

HERO_BEATS = [
    ("draw", 22),
    ("bounds", 12),
    ("simplify", 18),
    ("snap", 22),
    ("eta", 24),
    ("hold", 28),
]
STAGE_OF = {"draw": 0, "bounds": 1, "simplify": 2, "snap": 3, "eta": 4, "hold": 4}


def beat_at(frame, beats):
    acc = 0
    for name, n in beats:
        if frame < acc + n:
            return name, (frame - acc) / max(1, n - 1)
        acc += n
    name, n = beats[-1]
    return name, 1.0


def draw_track(ax, scene, alpha=1.0, frac=1.0, color=TRACK, lw=1.6, dots=False):
    pts = partial_polyline(scene["track"], frac)
    ax.plot(xs(pts), ys(pts), color=color, lw=lw, alpha=alpha,
            solid_capstyle="round", zorder=3)
    if dots:
        ax.plot(xs(pts), ys(pts), linestyle="none", marker="o", markersize=2.0,
                color=color, alpha=alpha * 0.7, zorder=3)


def draw_bounds(ax, scene, alpha, inset=0.0):
    ring = scene["bounds"]
    cx = sum(xs(ring)) / len(ring)
    cy = sum(ys(ring)) / len(ring)
    scaled = [[cx + (x - cx) * (1 + inset), cy + (y - cy) * (1 + inset)]
              for x, y in ring]
    ax.plot(xs(scaled), ys(scaled), color=BOX, lw=1.3, ls=(0, (6, 4)),
            alpha=alpha, zorder=2)


def draw_simplified(ax, scene, alpha=1.0, frac=1.0, dots=True):
    pts = partial_polyline(scene["simplified"], frac)
    ax.plot(xs(pts), ys(pts), color=GREEN, lw=2.6, alpha=alpha,
            solid_capstyle="round", solid_joinstyle="round", zorder=5)
    if dots:
        ax.plot(xs(pts), ys(pts), linestyle="none", marker="o", markersize=4.2,
                markerfacecolor=GREEN, markeredgecolor=BG, markeredgewidth=0.8,
                alpha=alpha, zorder=6)


def draw_fix(ax, pos, alpha=1.0, size=9):
    ax.plot([pos[0]], [pos[1]], marker="o", markersize=size, alpha=alpha,
            markerfacecolor=FIX, markeredgecolor=BG, markeredgewidth=1.2, zorder=8)


def hero_frame(frame, ax, scene, limits):
    name, p = beat_at(frame, HERO_BEATS)
    setup_axes(ax, scene, limits)
    fix = scene["fix"]
    snapped = scene["snapped"]
    seg = scene["snap_props"]["segment"]
    dist = scene["snap_props"]["distance_m"]
    n_raw = int(scene["track_props"]["count"])
    n_simp = int(scene["simplified_props"]["count"])
    code = "geo::decode(polyline)"

    if name == "draw":
        e = ease(p)
        draw_track(ax, scene, frac=e, dots=True)
        shown = max(1, int(round(e * n_raw)))
        code = f"geo::decode(polyline)  ->  {shown:>2} / {n_raw} pts"
    elif name == "bounds":
        draw_track(ax, scene)
        draw_bounds(ax, scene, alpha=ease(p), inset=(1 - ease(p)) * 0.06)
        code = "geo::bounds(track)"
    elif name == "simplify":
        e = ease(p)
        draw_bounds(ax, scene, alpha=0.35)
        draw_track(ax, scene, alpha=1.0 - 0.72 * e)
        draw_simplified(ax, scene, alpha=e, frac=e)
        shown = n_raw - int(round(e * (n_raw - n_simp)))
        code = f"geo::simplify(track, 5 m, geodesic)  ->  {shown} pts"
    elif name == "snap":
        draw_bounds(ax, scene, alpha=0.28)
        draw_track(ax, scene, alpha=0.22)
        draw_simplified(ax, scene)
        # 0..0.55: the fix drops in from the top; 0.55..1: it snaps.
        drop = ease(min(1.0, p / 0.55))
        start_lat = limits[3] - (limits[3] - limits[2]) * 0.06
        cur = [fix[0], start_lat + (fix[1] - start_lat) * drop]
        draw_fix(ax, cur)
        if p > 0.55:
            s = ease((p - 0.55) / 0.45)
            link = [fix, [fix[0] + (snapped[0] - fix[0]) * s,
                          fix[1] + (snapped[1] - fix[1]) * s]]
            ax.plot(xs(link), ys(link), color=FIX, lw=1.6, ls=(0, (2, 2)),
                    alpha=0.9, zorder=7)
            ax.plot([snapped[0]], [snapped[1]], marker="o",
                    markersize=6 + 5 * s, markerfacecolor=GREEN,
                    markeredgecolor=BG, markeredgewidth=1.2, zorder=9)
        code = f"geo::closest_point_on_path(fix, route)  ->  seg {seg}, {dist:.1f} m"
    elif name == "eta":
        draw_bounds(ax, scene, alpha=0.28)
        draw_track(ax, scene, alpha=0.18)
        draw_simplified(ax, scene)
        draw_fix(ax, fix, alpha=0.85, size=8)
        ax.plot(xs([fix, snapped]), ys([fix, snapped]), color=FIX, lw=1.4,
                ls=(0, (2, 2)), alpha=0.7, zorder=7)
        frames = scene["eta_frames"]
        idx = int(round(ease(p) * (len(frames) - 1)))
        trail = frames[:idx + 1]
        ax.plot(xs(trail), ys(trail), color=ETA, lw=3.4, alpha=0.9,
                solid_capstyle="round", zorder=8)
        cur = frames[idx]
        ax.plot([cur[0]], [cur[1]], marker="o", markersize=11,
                markerfacecolor=ETA, markeredgecolor=BG, markeredgewidth=1.4,
                zorder=10)
        code = "geo::point_at_distance(route, 500 m)"
    else:  # hold
        draw_bounds(ax, scene, alpha=0.28)
        draw_track(ax, scene, alpha=0.16)
        draw_simplified(ax, scene)
        draw_fix(ax, fix, alpha=0.85, size=8)
        ax.plot(xs([fix, snapped]), ys([fix, snapped]), color=FIX, lw=1.4,
                ls=(0, (2, 2)), alpha=0.7, zorder=7)
        eta = scene["eta"]
        ax.plot(xs(scene["eta_frames"]), ys(scene["eta_frames"]), color=ETA,
                lw=3.4, alpha=0.9, solid_capstyle="round", zorder=8)
        ax.plot([eta[0]], [eta[1]], marker="o", markersize=11,
                markerfacecolor=ETA, markeredgecolor=BG, markeredgewidth=1.4,
                zorder=10)
        code = f"decode {n_raw} -> simplify {n_simp} -> snap -> ETA, all in C++"

    draw_chrome(ax, limits, code, STAGE_OF[name])


def render_hero(out_path):
    scene = load_scene()
    limits = compute_limits(scene)
    fig = plt.figure(figsize=FIGSIZE, dpi=DPI)
    fig.patch.set_facecolor(BG)
    ax = fig.add_axes([0, 0, 1, 1])
    total = sum(n for _, n in HERO_BEATS)

    def update(frame):
        hero_frame(frame, ax, scene, limits)
        return []

    anim = FuncAnimation(fig, update, frames=total, blit=False)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    anim.save(str(out_path), writer=PillowWriter(fps=FPS), dpi=DPI)
    plt.close(fig)
    print(f"wrote {out_path}  ({total} frames @ {FPS} fps)")


# --------------------------------------------------------------------------
# standalone Douglas-Peucker collapse (docs/assets/gallery/simplify.gif)
# --------------------------------------------------------------------------

SIMPLIFY_HOLD = 5   # frames each tolerance is held
SIMPLIFY_TAIL = 12  # extra hold on the coarsest frame
SIMPLIFY_FPS = 12
SIMPLIFY_FIGSIZE = (8.0, 4.5)  # -> 800 x 450 px


def render_simplify(out_path):
    scene = load_scene()
    # dp.jsonl runs coarse -> fine; reverse it so the gif reads as a collapse:
    # the detailed route sheds vertices as the tolerance grows.
    frames = scene["dp_frames"][::-1]  # fine (many pts) -> coarse (few pts)
    limits = compute_limits(scene)
    fig = plt.figure(figsize=SIMPLIFY_FIGSIZE, dpi=DPI)
    fig.patch.set_facecolor(BG)
    ax = fig.add_axes([0, 0, 1, 1])

    schedule = []
    for i in range(len(frames)):
        reps = SIMPLIFY_HOLD + (SIMPLIFY_TAIL if i == len(frames) - 1 else 0)
        schedule.extend([i] * reps)

    def update(k):
        idx = schedule[k]
        f = frames[idx]
        pts = f["points"]
        setup_axes(ax, scene, limits)
        # Raw track faint underneath for reference.
        ax.plot(xs(scene["track"]), ys(scene["track"]), color=TRACK, lw=1.2,
                alpha=0.5, zorder=2)
        ax.plot(xs(pts), ys(pts), color=GREEN, lw=2.6, solid_capstyle="round",
                solid_joinstyle="round", zorder=5)
        ax.plot(xs(pts), ys(pts), linestyle="none", marker="o", markersize=5,
                markerfacecolor=GREEN, markeredgecolor=BG, markeredgewidth=0.8,
                zorder=6)
        x0, x1, y0, y1 = limits
        ax.text(x0, y1 - (y1 - y0) * 0.05, "Douglas-Peucker simplify",
                color=TEXT, fontsize=15, fontweight="bold", va="top", ha="left")
        ax.text(x0, y0 + (y1 - y0) * 0.05,
                f"tolerance {f['tolerance_m']:g} m   ->   "
                f"{int(f['count'])} / {len(scene['track'])} pts",
                color=CODE, fontsize=12, va="bottom", ha="left",
                family="monospace")
        return []

    anim = FuncAnimation(fig, update, frames=len(schedule), blit=False)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    anim.save(str(out_path), writer=PillowWriter(fps=SIMPLIFY_FPS), dpi=DPI)
    plt.close(fig)
    print(f"wrote {out_path}  ({len(schedule)} frames @ {SIMPLIFY_FPS} fps)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--hero-only", action="store_true",
                       help="render only hero-pipeline.gif")
    group.add_argument("--simplify-only", action="store_true",
                       help="render only gallery/simplify.gif")
    args = parser.parse_args()

    if not args.simplify_only:
        render_hero(OUT_HERO)
    if not args.hero_only:
        render_simplify(OUT_SIMPLIFY)


if __name__ == "__main__":
    main()
