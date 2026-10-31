#pragma once
#include "mbf/polygon.hpp"
#include <string>
#include <vector>

namespace mbf {

enum class PolygonOffsetJoin { Miter };

struct PolygonOffsetResult {
    bool valid{false};
    Polygon2D polygon;
    double requested_offset_m{};
    std::vector<std::string> errors;
};

// Positive offset expands outward; negative offset insets inward.
// The result is rejected if it self-intersects, collapses, or exceeds miter_limit.
[[nodiscard]] PolygonOffsetResult offset_polygon(
    const Polygon2D& polygon, double offset_m,
    double miter_limit = 4.0, double tolerance = 1e-8);

struct BuildableAreaResult {
    bool valid{false};
    Polygon2D lot_boundary;
    Polygon2D buildable_boundary;
    double setback_m{};
    double lot_area_m2{};
    double buildable_area_m2{};
    std::vector<std::string> errors;
};

// Applies a uniform setback to a simple lot boundary.
[[nodiscard]] BuildableAreaResult compute_buildable_area(
    const Polygon2D& lot_boundary, double setback_m,
    double tolerance = 1e-8);

} // namespace mbf
