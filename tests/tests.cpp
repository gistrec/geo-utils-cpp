/** Including all tests */
#include "latlng/is_valid.hpp"
#include "latlng/operator_equal.hpp"

#include "math/mod.hpp"

#include "spherical/angle_between.hpp"
#include "spherical/area.hpp"
#include "spherical/distance_between.hpp"
#include "spherical/heading.hpp"
#include "spherical/interpolate.hpp"
#include "spherical/offset.hpp"
#include "spherical/offset_origin.hpp"
#include "spherical/path_length.hpp"
#include "spherical/signed_area.hpp"

#include "poly/contains.hpp"
#include "poly/distance_to_segment.hpp"
#include "poly/is_closed_polygon.hpp"
#include "poly/on_edge.hpp"
#include "poly/on_path.hpp"
#include "poly/simplify.hpp"

#include "encoding/decode.hpp"
#include "encoding/encode.hpp"


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
