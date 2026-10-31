#pragma once
#include "mbf/types.hpp"
#include <vector>
namespace mbf {
class FloorPlanGenerator {
public:
 [[nodiscard]] FloorPlan generate_grid_growth(const GenerationSettings&,const std::vector<RoomSpec>&) const;
 [[nodiscard]] FloorPlan generate_treemap(const GenerationSettings&,const std::vector<RoomSpec>&) const;
};
} // namespace mbf
