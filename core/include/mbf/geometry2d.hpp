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

enum class BoundaryKind2D { ExteriorWall, SharedWall };

struct CoordinateSystem2D {
    std::string units{"metres"};
    std::string origin{"south-west"};
    std::string positive_x{"east"};
    std::string positive_y{"north"};
};

struct BoundarySegment2D {
    std::uint64_t id{};
    Point2 start;
    Point2 end;
    double length_m{};
    BoundaryKind2D kind{BoundaryKind2D::ExteriorWall};
    RoomId room_a{};
    RoomId room_b{};
};

enum class WallJunctionKind2D { Endpoint, Straight, Corner, Tee, Cross, Complex };

struct WallNode2D {
    std::uint64_t id{};
    Point2 position;
    WallJunctionKind2D kind{WallJunctionKind2D::Endpoint};
    std::vector<std::uint64_t> connected_wall_ids;
};

enum class OpeningKind2D { EntranceDoor, InteriorDoor, Window, OpenPlan };

struct WallOpening2D {
    std::uint64_t id{};
    std::uint64_t wall_id{};
    OpeningKind2D kind{OpeningKind2D::InteriorDoor};
    double offset_m{};
    double width_m{};
    double height_m{2.1};
    double sill_height_m{};
    RoomId room_a{};
    RoomId room_b{};
};

struct WallPiece2D {
    std::uint64_t id{};
    std::uint64_t source_wall_id{};
    Point2 start;
    Point2 end;
    double length_m{};
    bool opening_gap{false};
};

struct OpeningSettings2D {
    double entrance_width_m{1.0};
    double interior_door_width_m{0.9};
    double window_width_m{1.2};
    double minimum_end_clearance_m{0.15};
    double door_height_m{2.1};
    double window_height_m{1.2};
    double window_sill_height_m{0.9};
};

struct WallTopology2D {
    std::vector<BoundarySegment2D> walls;
    std::vector<WallOpening2D> openings;
    std::vector<WallPiece2D> wall_pieces;
    std::vector<WallNode2D> junctions;
    bool valid{true};
    std::vector<std::string> diagnostics;
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
    Entrance2D entrance;
    CoordinateSystem2D coordinates;
    std::vector<RoomGeometry2D> rooms;
    std::vector<BoundarySegment2D> boundaries;
};

struct Geometry2DResult {
    bool valid{true};
    BuildingGeometry2D geometry;
    std::vector<std::string> errors;
};

[[nodiscard]] Geometry2DResult build_geometry_2d(const FloorPlan& plan);
[[nodiscard]] std::vector<BoundarySegment2D> classify_boundaries(const BuildingGeometry2D& geometry, double tolerance = 1e-7);
[[nodiscard]] WallTopology2D build_wall_topology(const BuildingGeometry2D& geometry, double tolerance = 1e-7);
[[nodiscard]] bool add_wall_opening(WallTopology2D& topology, std::uint64_t wall_id,
                                    OpeningKind2D kind, double offset_m, double width_m,
                                    const OpeningSettings2D& settings = {},
                                    std::string* error = nullptr,
                                    double tolerance = 1e-7);
void rebuild_wall_pieces(WallTopology2D& topology, double tolerance = 1e-7);
[[nodiscard]] std::vector<std::uint64_t> generate_default_openings(
    WallTopology2D& topology, const Entrance2D& entrance,
    const OpeningSettings2D& settings = {});
[[nodiscard]] bool export_geometry_svg(const BuildingGeometry2D& geometry,
                                       const std::string& output_path,
                                       std::string* error = nullptr);
} // namespace mbf
