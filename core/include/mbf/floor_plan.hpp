#pragma once
#include "mbf/types.hpp"
#include <string>
#include <vector>
namespace mbf {
struct ValidationResult { bool valid{true}; std::vector<std::string> errors; };
[[nodiscard]] ValidationResult validate_floor_plan(const FloorPlan& plan);
void calculate_adjacency(FloorPlan& plan);
} // namespace mbf
