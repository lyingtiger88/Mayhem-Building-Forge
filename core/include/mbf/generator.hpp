#pragma once
#include "mbf/types.hpp"
#include "mbf/building_brief.hpp"
#include "mbf/topology.hpp"
#include <vector>
namespace mbf {
class FloorPlanGenerator {
public:
 [[nodiscard]] FloorPlan generate_grid_growth(const GenerationSettings&,const std::vector<RoomSpec>&) const;
 [[nodiscard]] FloorPlan generate_treemap(const GenerationSettings&,const std::vector<RoomSpec>&) const;
 [[nodiscard]] FloorPlan generate_residential_layout(const GenerationSettings&, const std::vector<RoomSpec>&, EntranceSide entrance_side = EntranceSide::South) const;
 [[nodiscard]] FloorPlan generate_topology_layout(const GenerationSettings&, const BuildingBrief&, const RoomTopology&) const;
};
} // namespace mbf
