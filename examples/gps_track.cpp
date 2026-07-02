// An end-to-end GPS-track pipeline: decode an encoded polyline, inspect its
// bounds, simplify it, snap a live GPS fix to the route, and predict a
// position N meters down the road.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include <geo/geo.hpp>

int main() {
    // A 95-point GPS track as it would arrive from a routing API (the
    // Encoded Polyline format, 1e-5-degree grid).
    static const char kTrack[] =
        "elfjD~a}uNOnFN~Em@fJv@tEMhGDjDe@hG^nF??@lA?n@IvAC`Ay@A{@DwCA{CF_EC{CEi@PBTFDJBJ?V?n@?D@?A@?@?F?F?"
        "LAf@?n@@`@@T@~@FpA?fA?p@?r@?vAH`@OR@^ETFJCLD?JA^?J?P?fAC`B@d@?b@A\\@`@Ad@@\\?`@?f@?V?H?DD@DDBBDBD?"
        "D?B?B@B@@@B@B@B@D?D?JAF@H@FCLADBDBDCFAN?b@Af@@x@@";
    std::vector<geo::LatLng> track = geo::decode(kTrack);
    std::cout << "Decoded " << track.size() << " points\n";
    assert(track.size() == 95);

    // Where does the track live? LatLngBounds is a cheap prefilter: a point
    // outside the bounds cannot be on the track.
    auto box = geo::bounds(track);
    assert(box.has_value());
    std::cout << "Bounds: " << *box << ", center " << box->center() << "\n";
    assert(box->contains(track.front()) && box->contains(track.back()));

    double full_length = geo::path_length(track);
    std::cout << "Track length: " << full_length << " m\n";

    // Simplify for storage or display. geodesic=true measures true
    // great-circle distances (exact at any latitude and across the
    // antimeridian); 5 m tolerance keeps the shape within GPS noise.
    std::vector<geo::LatLng> simplified = geo::simplify(track, 5.0, /*geodesic=*/true);
    std::cout << "Simplified to " << simplified.size() << " points ("
              << geo::path_length(simplified) << " m)\n";
    assert(simplified.size() < track.size());
    assert(std::abs(geo::path_length(simplified) - full_length) < full_length * 0.05);

    // A GPS fix comes in: snap it to the route.
    geo::LatLng fix(track[40].lat + 0.00012, track[40].lng - 0.00008);  // ~15 m off the road
    auto snapped = geo::closest_point_on_path(fix, simplified);
    assert(snapped.has_value());
    std::cout << "GPS fix " << fix << " snapped to " << snapped->point
              << " (segment " << snapped->segment << ", " << snapped->distance << " m off)\n";
    assert(snapped->distance < 25.0);
    assert(geo::on_path(snapped->point, simplified, /*geodesic=*/true, /*tolerance=*/0.01));

    // Predict the position 500 m down the road from the start.
    auto eta_point = geo::point_at_distance(simplified, 500.0);
    assert(eta_point.has_value());
    std::cout << "500 m into the route: " << *eta_point << "\n";
    assert(box->contains(*eta_point));

    // Ship the simplified track onward as polyline6 (the OSRM/Valhalla grid).
    std::string reencoded = geo::encode(simplified, 6);
    std::cout << "Re-encoded (polyline6, " << reencoded.size() << " chars)\n";
    assert(geo::decode(reencoded, 6).size() == simplified.size());

    return 0;
}
