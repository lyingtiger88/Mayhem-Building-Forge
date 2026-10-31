#include "mbf/generator.hpp"
#include "mbf/floor_plan.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    const mbf::GenerationSettings settings{18.0, 12.0, 64U, 20261031U};
    const std::vector<mbf::RoomSpec> rooms{
        {"Living", 28.0}, {"Kitchen", 14.0}, {"Bedroom", 18.0},
        {"Bathroom", 7.0}, {"Hallway", 9.0}
    };

    const mbf::FloorPlanGenerator generator;
    const auto plan = generator.generate_grid_growth(settings, rooms);
    const auto validation = mbf::validate_floor_plan(plan);
    if (!validation.valid) {
        for (const auto& error : validation.errors) std::cerr << error << '\n';
        return 1;
    }

    std::cout << "Mayhem Building Forge - generated floor plan\n"
              << "Footprint: " << settings.width_m << "m x " << settings.depth_m
              << "m | seed: " << plan.seed << '\n';
    for (const auto& room : plan.rooms) {
        std::cout << std::left << std::setw(12) << room.name
                  << " area=" << std::fixed << std::setprecision(2)
                  << room.actual_area_m2 << " m2"
                  << " cells=" << room.cells.size() << '\n';
    }
    std::cout << "Room adjacencies: " << plan.adjacency.size() << '\n';
    return 0;
}
