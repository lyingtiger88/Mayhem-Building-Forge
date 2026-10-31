#pragma once
#include "mbf/polygon.hpp"
#include "mbf/types.hpp"
#include <string>
#include <vector>

namespace mbf {

enum class OpeningKind { EntranceDoor, InteriorDoor, OpenPlanConnection };

struct PlanOpening {
    RoomId from_room{};
    RoomId to_room{};
    OpeningKind kind{OpeningKind::InteriorDoor};
    Point2 start;
    Point2 end;
    double clear_width_m{};
};

struct PlannedRoom {
    RoomId id{};
    std::string name;
    RoomRole role{RoomRole::Auto};
    Polygon2D footprint;
    double area_m2{};
};

struct ResidentialLayout2D {
    double width_m{}, depth_m{};
    std::uint64_t seed{};
    std::vector<PlannedRoom> rooms;
    std::vector<PlanOpening> openings;
};

struct ResidentialLayoutSettings {
    double width_m{12.0};
    double depth_m{9.0};
    double public_zone_depth_ratio{0.61};
    double circulation_depth_ratio{0.17};
    double living_width_ratio{0.54};
    double bedroom_width_ratio{0.625};
    double entrance_width_m{1.0};
    double internal_door_width_m{0.9};
    double open_plan_width_m{2.4};
    std::uint64_t seed{1};
};

struct LayoutValidation {
    bool valid{false};
    std::vector<std::string> errors;
};

// First deterministic residential typology: public rooms to the south,
// a full-width circulation spine, and private/service rooms to the north.
[[nodiscard]] ResidentialLayout2D generate_balanced_residential_layout(
    const ResidentialLayoutSettings& settings = {});

[[nodiscard]] LayoutValidation validate_residential_layout(
    const ResidentialLayout2D& layout, double tolerance = 1e-7);

[[nodiscard]] bool export_residential_layout_svg(
    const ResidentialLayout2D& layout, const std::string& output_path,
    std::string* error = nullptr);

} // namespace mbf
