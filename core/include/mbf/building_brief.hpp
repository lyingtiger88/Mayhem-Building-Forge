#pragma once
#include "mbf/types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mbf {

enum class AdjacencyStrength { Required, Preferred, Avoid };

struct RoomRequirement {
    std::string name;
    RoomRole role{RoomRole::Auto};
    std::uint32_t count{1};
    double target_area_m2{12.0};
    double minimum_area_m2{6.0};
    double minimum_width_m{2.0};
    double minimum_depth_m{2.0};
    double priority{1.0};
};

struct RoomAdjacencyRequirement {
    RoomRole first{RoomRole::Auto};
    RoomRole second{RoomRole::Auto};
    AdjacencyStrength strength{AdjacencyStrength::Preferred};
    double weight{1.0};
};

struct BuildingBrief {
    std::uint32_t schema_version{1};
    std::string project_name{"My Building"};
    double site_width_m{20.0};
    double site_depth_m{15.0};
    double setback_south_m{0.0};
    double setback_north_m{0.0};
    double setback_east_m{0.0};
    double setback_west_m{0.0};
    std::uint32_t floor_count{1};
    double floor_to_floor_height_m{3.2};
    EntranceSide entrance_side{EntranceSide::South};
    std::vector<RoomRequirement> rooms;
    std::vector<RoomAdjacencyRequirement> adjacency;
    std::uint64_t seed{1};
};

struct BuildingBriefValidation {
    bool valid{false};
    double buildable_width_m{};
    double buildable_depth_m{};
    double buildable_area_m2{};
    double requested_minimum_room_area_m2{};
    double requested_target_room_area_m2{};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct PlannerCompatibility {
    bool supported{false};
    std::vector<std::string> limitations;
};

[[nodiscard]] BuildingBrief make_default_residential_brief();
[[nodiscard]] BuildingBriefValidation validate_building_brief(const BuildingBrief& brief);
[[nodiscard]] PlannerCompatibility check_current_planner_compatibility(
    const BuildingBrief& brief);

} // namespace mbf
