// Cross-checks the WebAssembly build against the native reference.
//   1. c++ -std=c++17 -I../include test/native_ref.cpp -o native_ref && ./native_ref > expected.json
//   2. node test/verify.mjs expected.json
// Same inputs as native_ref.cpp; exits non-zero on any mismatch.
import { readFileSync } from 'node:fs';
import { fileURLToPath, pathToFileURL } from 'node:url';
import path from 'node:path';

const here = path.dirname(fileURLToPath(import.meta.url));
const expected = JSON.parse(readFileSync(process.argv[2], 'utf8'));
const { default: createGeo } = await import(pathToFileURL(path.join(here, '../web/geo.js')).href);
const g = await createGeo();

const route = [37.7749, -122.4194, 37.8044, -122.2712, 37.8716, -122.2727, 38.5816, -121.4944];
const polygon = [37.80, -122.45, 37.80, -122.40, 37.76, -122.40, 37.76, -122.45];
const anti = [-17.0, 179.5, -17.5, -179.5, -18.0, 179.8];

const actual = {
  version: g.version(),
  distance_sf_ny: g.distanceBetween(37.7749, -122.4194, 40.7128, -74.0060),
  heading_sf_ny: g.heading(37.7749, -122.4194, 40.7128, -74.0060),
  offset: g.offset(37.7749, -122.4194, 10000, 45),
  interpolate: g.interpolate(37.7749, -122.4194, 40.7128, -74.0060, 0.25),
  path_length: g.pathLength(route),
  point_at_distance: g.pointAtDistance(route, 20000),
  area_polygon: g.area(polygon),
  contains_inside: +g.contains(37.78, -122.42, polygon, false),
  contains_outside: +g.contains(37.90, -122.42, polygon, false),
  distance_to_edge: g.distanceToEdge(37.78, -122.42, polygon),
  encode5: g.encode(route, 5),
  encode6: g.encode(route, 6),
  decode5: g.decode('_p~iF~ps|U_ulLnnqC_mqNvxq`@', 5),
  decode6_roundtrip: g.decode(g.encode(route, 6), 6),
  simplify_100m: g.simplify(route, 100, false),
  closest: g.closestPointOnPath(37.83, -122.30, route),
  bounds_anti: g.bounds(anti),
  path_length_anti: g.pathLength(anti),
  area_anti: g.area(anti),
};

let bad = 0;
for (const [key, want] of Object.entries(expected)) {
  const got = actual[key];
  let ok, diff;
  if (typeof want === 'string') {
    ok = want === got;
    diff = ok ? 0 : `"${want}" != "${got}"`;
  } else {
    const a = Array.isArray(want) ? want : [want];
    const b = Array.isArray(got) ? got : [got];
    diff = a.length === b.length ? Math.max(...a.map((x, i) => Math.abs(x - b[i]))) : Infinity;
    // Native and WASM run the same C++ (libm sin/cos/asin may differ in the last ulps).
    // Tolerance is relative to the magnitude, so km-scale areas are not held to nanometers.
    ok = diff <= 1e-9 * Math.max(1, ...a.map(Math.abs));
  }
  if (!ok) bad++;
  console.log(`${ok ? 'OK  ' : 'FAIL'} ${key.padEnd(20)} max|Δ| = ${diff}`);
}
process.exit(bad ? 1 : 0);
