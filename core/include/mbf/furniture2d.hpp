#pragma once
#include "mbf/geometry2d.hpp"
#include <string>
#include <vector>

namespace mbf {
enum class FurnitureKind2D {
    Bed, Nightstand, Wardrobe, Sofa, CoffeeTable, AccentChair, TvStand,
    Counter, Sink, Cooktop, Refrigerator, DiningTable, DiningChair,
    Shower, Toilet, Vanity, Console
};
struct FurnitureInstance2D {
    std::uint64_t id{};
    RoomId room_id{};
    FurnitureKind2D kind{FurnitureKind2D::Bed};
    std::string label;
    double x{}, y{}, width_m{}, depth_m{};
    double rotation_deg{};
    double clearance_m{0.15};
    bool movable{true};
    bool collision_enabled{true};
};
struct FurnitureLayout2D {
    std::uint32_t schema_version{1};
    std::vector<FurnitureInstance2D> items;
    std::vector<std::string> diagnostics;
    bool valid{true};
};
struct FurnitureSettings2D {
    double wall_clearance_m{0.12};
    double object_clearance_m{0.18};
    double circulation_clearance_m{0.75};
};
[[nodiscard]] FurnitureLayout2D generate_furniture_layout(
    const BuildingGeometry2D& geometry,
    const FurnitureSettings2D& settings = {});
[[nodiscard]] bool validate_furniture_layout(
    const BuildingGeometry2D& geometry,
    const FurnitureLayout2D& layout,
    double tolerance = 1e-6,
    std::vector<std::string>* diagnostics = nullptr);
} // namespace mbf
