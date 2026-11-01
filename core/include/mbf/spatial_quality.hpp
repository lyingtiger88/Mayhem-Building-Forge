#pragma once
#include "mbf/building_brief.hpp"
#include "mbf/types.hpp"
#include <string>
#include <vector>

namespace mbf {

struct RoomSpatialMetrics {
    RoomId room_id{};
    double exterior_frontage_m{};
    double daylight_potential{}; // 0..1 heuristic; not a daylight simulation.
    double privacy_score{};       // 0..100 adjacency-based heuristic.
};

struct SpatialQualityReport {
    bool valid{false};
    double daylight_score{};   // 0..100
    double privacy_score{};    // 0..100
    double circulation_score{};// 0..100
    double overall_score{};    // 0..100
    bool circulation_connected{false};
    std::vector<RoomSpatialMetrics> rooms;
    std::vector<std::string> warnings;
};

[[nodiscard]] SpatialQualityReport evaluate_spatial_quality(
    const FloorPlan& plan, const BuildingBrief& brief);

} // namespace mbf
