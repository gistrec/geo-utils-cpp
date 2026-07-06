#!/usr/bin/env python3
# Copyright 2026 Aleksandr Kovalko
# Licensed under the Apache License, Version 2.0
"""Renders the four gallery stills (#5-#8) over real basemap tiles.

  docs/assets/gallery/great-circle.png   (#5) NY -> London geodesic arc
  docs/assets/gallery/snap-to-route.png  (#6) an off-road GPS fix snapped
  docs/assets/gallery/point-in-polygon.png (#7) a midtown box + points
  docs/assets/gallery/encode-decode.png  (#8) the decoded polyline6 track

These fetch map tiles (OpenStreetMap via `staticmap`, Natural Earth via
`cartopy`), so they are **NON-deterministic** -- the CI drift-gate deliberately
skips them. Render locally and commit the PNGs by hand, exactly like the VHS
`demo.gif`. The standalone Douglas-Peucker collapse (#4, gallery/simplify.gif)
is deterministic and lives in hero.py instead, so the drift-gate can regenerate
it.

Every scene draws geometry that geo-utils-cpp computed in C++: the track
scenes read docs/assets/data/track.geojson (from tools/assets/emit.cpp); the
scalar figures quoted in captions (5570 km NY->London, the geo::contains
inside/outside split) are geo-utils outputs, inlined here with attribution --
Python only draws, it never recomputes the geometry.

Dependencies (not needed by the deterministic core, so kept out of the CI
install -- see requirements.txt):
    pip install "staticmap==0.5.*" "cartopy==0.23.*"
and network access to the tile servers. On a machine without either (e.g. a
sandboxed CI runner) each still is skipped with a clear message rather than a
silent stub.

Usage: tools/render/gallery.py [--only great-circle|snap-to-route|...]
"""

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
DATA = ROOT / "docs" / "assets" / "data"
OUT = ROOT / "docs" / "assets" / "gallery"

# OpenStreetMap standard tiles -- attribution is mandatory (ODbL).
OSM_TILES = "https://tile.openstreetmap.org/{z}/{x}/{y}.png"
OSM_ATTRIB = "(c) OpenStreetMap contributors"

GREEN = "#2f9e44"
FIX = "#e8590c"
BLUE = "#1971c2"
RED = "#e03131"


# --------------------------------------------------------------------------
# committed emitter output (geometry already computed in C++)
# --------------------------------------------------------------------------

def load_track():
    gj = json.loads((DATA / "track.geojson").read_text())
    feats = {f["properties"]["role"]: f for f in gj["features"]}
    return feats


def lnglat_to_latlng(coords):
    """GeoJSON [lng, lat] -> (lat, lng) tuples for map libraries."""
    return [(c[1], c[0]) for c in coords]


def add_attribution(image, text=OSM_ATTRIB):
    """Stamp a tile-attribution line along the bottom edge (PIL)."""
    from PIL import ImageDraw
    draw = ImageDraw.Draw(image, "RGBA")
    w, h = image.size
    box_h = 18
    draw.rectangle([0, h - box_h, w, h], fill=(255, 255, 255, 190))
    draw.text((6, h - box_h + 3), text, fill=(60, 60, 60))
    return image


# --------------------------------------------------------------------------
# #6 / #7 / #8 -- OpenStreetMap tiles via staticmap
# --------------------------------------------------------------------------

def render_snap_to_route(out_path):
    """#6: the simplified route with an off-road fix snapped onto it."""
    from staticmap import StaticMap, Line, CircleMarker

    feats = load_track()
    route = feats["simplified"]["geometry"]["coordinates"]        # [lng, lat]
    fix = feats["fix"]["geometry"]["coordinates"]
    snapped = feats["snapped"]["geometry"]["coordinates"]
    dist = feats["snapped"]["properties"]["distance_m"]

    m = StaticMap(1000, 700, url_template=OSM_TILES)
    m.add_line(Line(route, GREEN, 5))
    m.add_line(Line([fix, snapped], FIX, 2))          # the snap link
    m.add_marker(CircleMarker(fix, FIX, 12))          # off-road fix
    m.add_marker(CircleMarker(snapped, GREEN, 12))    # snapped point
    image = m.render()
    add_attribution(image)
    image.save(out_path)
    print(f"wrote {out_path}  (snap distance {dist:.1f} m, via geo::closest_point_on_path)")


def render_encode_decode(out_path):
    """#8: the polyline6 string decoded back to the 95-point track."""
    from staticmap import StaticMap, Line, CircleMarker

    feats = load_track()
    track = feats["track"]["geometry"]["coordinates"]
    n = int(feats["track"]["properties"]["count"])

    m = StaticMap(1000, 700, url_template=OSM_TILES)
    m.add_line(Line(track, BLUE, 4))
    m.add_marker(CircleMarker(track[0], GREEN, 12))
    m.add_marker(CircleMarker(track[-1], RED, 12))
    image = m.render()
    add_attribution(image)
    image.save(out_path)
    print(f"wrote {out_path}  ({n} pts <-> polyline6, via geo::encode/geo::decode)")


# The midtown Manhattan box and query points from the README usage example.
# The inside/outside split is geo::contains output (geodesic=false ray-cast),
# computed in C++ and inlined here -- Python only colors the dots:
#     contains(pt, midtown)  ->  see tools/assets/emit.cpp's pipeline / README
MIDTOWN = [  # [lng, lat], CCW
    (-74.01, 40.74), (-73.96, 40.74), (-73.96, 40.78), (-74.01, 40.78),
]
PIP_POINTS = [  # (name, lng, lat, inside) -- `inside` from geo::contains
    ("Times Square",    -73.9855, 40.7580, True),
    ("Columbus Circle", -73.9818, 40.7680, True),
    ("Empire State",    -73.9857, 40.7484, True),
    ("Bryant Park",     -73.9832, 40.7536, True),
    ("Central Park N",  -73.9665, 40.7812, False),
    ("Hudson River",    -74.0200, 40.7600, False),
    ("Brooklyn",        -73.9900, 40.7000, False),
    ("Queens",          -73.9200, 40.7550, False),
]


def render_point_in_polygon(out_path):
    """#7: a midtown box with points coloured by geo::contains."""
    from staticmap import StaticMap, Line, CircleMarker

    m = StaticMap(1000, 800, url_template=OSM_TILES)
    ring = list(MIDTOWN) + [MIDTOWN[0]]
    m.add_line(Line(ring, GREEN, 4))
    for _name, lng, lat, inside in PIP_POINTS:
        m.add_marker(CircleMarker((lng, lat), GREEN if inside else RED, 12))
    image = m.render()
    add_attribution(image)
    image.save(out_path)
    inside = sum(1 for p in PIP_POINTS if p[3])
    print(f"wrote {out_path}  ({inside}/{len(PIP_POINTS)} inside, via geo::contains)")


# --------------------------------------------------------------------------
# #5 -- the NY -> London great circle via cartopy
# --------------------------------------------------------------------------

# Endpoints (inputs) and the geo-utils great-circle figures (outputs), inlined
# with attribution: geo::distance_between(NY, London) = 5570 km,
# geo::heading(NY, London) = 51.2 deg. cartopy's Geodetic transform draws the
# arc itself; geo-utils supplies the numbers in the caption.
NEW_YORK = (-74.0060, 40.7128)   # (lng, lat)
LONDON = (-0.1278, 51.5074)
GC_DISTANCE_KM = 5570
GC_HEADING_DEG = 51.2


def render_great_circle(out_path):
    """#5: the NY -> London geodesic drawn on a Robinson projection."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import cartopy.crs as ccrs
    import cartopy.feature as cfeature

    fig = plt.figure(figsize=(10, 6), dpi=100)
    ax = plt.axes(projection=ccrs.Robinson(central_longitude=-35))
    ax.set_global()
    ax.add_feature(cfeature.LAND, facecolor="#e9ecef")
    ax.add_feature(cfeature.OCEAN, facecolor="#d0ebff")
    ax.add_feature(cfeature.COASTLINE, linewidth=0.4, edgecolor="#868e96")
    # Plotting a straight line in the Geodetic CRS makes cartopy trace the
    # great circle across the projection.
    ax.plot([NEW_YORK[0], LONDON[0]], [NEW_YORK[1], LONDON[1]],
            color=GREEN, linewidth=2.5, transform=ccrs.Geodetic())
    for lng, lat in (NEW_YORK, LONDON):
        ax.plot([lng], [lat], marker="o", markersize=7, color=FIX,
                transform=ccrs.Geodetic())
    ax.set_title(
        f"geo::distance_between(NY, London) = {GC_DISTANCE_KM} km   "
        f"heading {GC_HEADING_DEG} deg", fontsize=12)
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {out_path}  (great circle, {GC_DISTANCE_KM} km)")


# --------------------------------------------------------------------------

STILLS = {
    "great-circle": ("great-circle.png", render_great_circle),
    "snap-to-route": ("snap-to-route.png", render_snap_to_route),
    "point-in-polygon": ("point-in-polygon.png", render_point_in_polygon),
    "encode-decode": ("encode-decode.png", render_encode_decode),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", choices=sorted(STILLS),
                        help="render a single still")
    args = parser.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    names = [args.only] if args.only else list(STILLS)
    failures = 0
    for name in names:
        filename, fn = STILLS[name]
        try:
            fn(OUT / filename)
        except ImportError as exc:
            failures += 1
            print(f"skip {name}: missing dependency ({exc.name}). "
                  f"pip install staticmap cartopy")
        except Exception as exc:  # network / tile-server errors
            failures += 1
            print(f"skip {name}: {type(exc).__name__}: {exc} "
                  f"(needs network access to the tile servers)")
    if failures:
        print(f"\n{failures}/{len(names)} still(s) skipped -- these are "
              f"non-deterministic tile renders, run locally and commit by hand.")


if __name__ == "__main__":
    main()
