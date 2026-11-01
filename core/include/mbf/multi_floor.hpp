#pragma once
#include "mbf/residential_layout.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mbf {

struct BuildingFloor2D {
    std::uint32_t floor_index{};
    double elevation_m{};
    double floor_to_floor_height_m{3.2};
    double slab_thickness_m{0.2};
    ResidentialLayout2D layout;
};

enum class VerticalCoreKind { Stair, Elevator };

struct VerticalCore2D {
    std::uint64_t id{};
    std::string name;
    VerticalCoreKind kind{VerticalCoreKind::Stair};
    Polygon2D footprint;
    // One room ID per floor, ordered exactly like BuildingFloor2D entries.
    std::vector<RoomId> room_ids_by_floor;
};

struct MultiFloorBuilding {
    std::uint32_t schema_version{1};
    std::vector<BuildingFloor2D> floors;
    std::vector<VerticalCore2D> vertical_cores;
};

struct MultiFloorValidationSettings {
    double minimum_floor_to_floor_height_m{2.7};
    double maximum_floor_to_floor_height_m{6.0};
    double maximum_slab_thickness_m{0.6};
    double alignment_tolerance_m{1e-5};
    double geometry_tolerance{1e-7};
};

struct MultiFloorValidation {
    bool valid{false};
    double total_height_m{};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

[[nodiscard]] MultiFloorBuilding generate_stacked_building(
    const ResidentialLayout2D& typical_floor,
    std::uint32_t floor_count,
    double floor_to_floor_height_m = 3.2,
    double slab_thickness_m = 0.2);

[[nodiscard]] MultiFloorValidation validate_multi_floor_building(
    const MultiFloorBuilding& building,
    const MultiFloorValidationSettings& settings = {});

} // namespace mbf
