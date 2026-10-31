#pragma once
#include "mbf/polygon.hpp"
#include <string>
#include <vector>

namespace mbf {

struct PolygonRegion2D {
    Polygon2D outer_boundary;
    std::vector<Polygon2D> holes;
};

struct PolygonPartitionValidation {
    bool valid{false};
    double region_area_m2{};
    double covered_area_m2{};
    std::vector<std::string> errors;
};

// Validates a complete, non-overlapping partition of an arbitrary simple
// outer polygon with zero or more interior holes. Room polygons must be
// simple, lie inside the outer boundary, and avoid all holes.
[[nodiscard]] PolygonPartitionValidation validate_polygon_partition(
    const PolygonRegion2D& region, const std::vector<Polygon2D>& parts,
    double tolerance = 1e-8);

} // namespace mbf
