#pragma once
#include "mbf/types.hpp"
#include <string>
#include <vector>

namespace mbf {
struct Point2 {
    double x{}, y{};
    friend bool operator==(const Point2&, const Point2&) = default;
};

struct Ring2 {
    std::vector<Point2> vertices;
    [[nodiscard]] double signed_area_m2() const noexcept;
};

struct RoomGeometry2D {
    RoomId id{};
    std::string name;
    double area_m2{};
    std::vector<Ring2> rings;
};

struct BuildingGeometry2D {
    std::uint32_t schema_version{1};
    double width_m{}, depth_m{};
    std::vector<RoomGeometry2D> rooms;
};

struct Geometry2DResult {
    bool valid{true};
    BuildingGeometry2D geometry;
    std::vector<std::string> errors;
};

[[nodiscard]] Geometry2DResult build_geometry_2d(const FloorPlan& plan);
[[nodiscard]] bool export_geometry_svg(const BuildingGeometry2D& geometry,
                                       const std::string& output_path,
                                       std::string* error = nullptr);
} // namespace mbf
