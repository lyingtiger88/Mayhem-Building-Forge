#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mbf {
using RoomId = std::uint32_t;

// Auto is resolved from the room name by the layout planner. Explicit roles
// always take precedence over name-based inference.
enum class RoomRole {
    Auto,
    Living,
    Kitchen,
    Bedroom,
    Bathroom,
    Hallway,
    Public,
    Private,
    Service,
    Circulation
};

struct RoomSpec {
    std::string name;
    double target_area_m2{1.0};
    RoomRole role{RoomRole::Auto};
};

struct Rect {
    double x{}, y{}, width{}, height{};
    [[nodiscard]] double area() const noexcept { return width * height; }
};

struct GridCell {
    std::uint32_t x{}, y{};
    RoomId room_id{};
    friend bool operator==(const GridCell&, const GridCell&) = default;
};

struct RoomRegion {
    RoomId id{};
    std::string name;
    double target_area_m2{}, actual_area_m2{};
    Rect bounds;
    std::vector<GridCell> cells;
    RoomRole role{RoomRole::Auto};
};

struct Adjacency {
    RoomId first{}, second{};
    std::uint32_t shared_edges{};
    friend bool operator==(const Adjacency&, const Adjacency&) = default;
};

enum class FloorPlanAlgorithm { GridGrowth, SquarifiedTreemap };

struct FloorPlan {
    std::uint32_t schema_version{1};
    std::uint64_t seed{};
    double width_m{}, depth_m{};
    FloorPlanAlgorithm algorithm{FloorPlanAlgorithm::GridGrowth};
    std::vector<RoomRegion> rooms;
    std::vector<Adjacency> adjacency;
    std::vector<GridCell> cells;
};

struct GenerationSettings {
    double width_m{12.0}, depth_m{10.0};
    std::uint32_t grid_resolution{48};
    std::uint64_t seed{1};
};
} // namespace mbf
