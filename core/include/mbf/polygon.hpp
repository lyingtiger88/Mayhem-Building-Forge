#pragma once
#include "mbf/geometry2d.hpp"
#include <string>
#include <vector>

namespace mbf {

struct Polygon2D {
    std::vector<Point2> vertices;
};

struct PolygonValidation {
    bool valid{false};
    std::vector<std::string> errors;
};

[[nodiscard]] Polygon2D make_rectangle(double x, double y, double width, double height);
[[nodiscard]] Polygon2D make_regular_polygon(Point2 center, double radius,
                                              std::uint32_t sides,
                                              double rotation_radians = -1.5707963267948966);
[[nodiscard]] double polygon_signed_area(const Polygon2D& polygon) noexcept;
[[nodiscard]] double polygon_area(const Polygon2D& polygon) noexcept;
[[nodiscard]] Point2 polygon_centroid(const Polygon2D& polygon);
[[nodiscard]] PolygonValidation validate_polygon(const Polygon2D& polygon,
                                                   double tolerance = 1e-9);
[[nodiscard]] bool point_in_polygon(const Polygon2D& polygon, Point2 point,
                                    double tolerance = 1e-9);
[[nodiscard]] double shared_boundary_length(const Polygon2D& a,
                                            const Polygon2D& b,
                                            double tolerance = 1e-8);
[[nodiscard]] bool export_polygon_gallery_svg(const std::vector<Polygon2D>& polygons,
                                               const std::vector<std::string>& labels,
                                               const std::string& output_path,
                                               std::string* error = nullptr);

} // namespace mbf
