#include "mbf/floor_plan.hpp"
#include "mbf/generator.hpp"
#include "mbf/geometry2d.hpp"
#include <iomanip>
#include <iostream>
#include <string>
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
    const auto geometry = mbf::build_geometry_2d(plan);
    if (!geometry.valid) {
        for (const auto& error : geometry.errors) std::cerr << error << '\n';
        return 2;
    }
    std::string export_error;
    if (!mbf::export_geometry_svg(geometry.geometry, "mayhem_floor_plan.svg", &export_error)) {
        std::cerr << export_error << '\n';
        return 3;
    }
    std::cout << "Mayhem Building Forge - 2D floor-plan generated\n"
              << "Footprint: " << settings.width_m << "m x " << settings.depth_m
              << "m | seed: " << plan.seed << '\n';
    for (const auto& room : geometry.geometry.rooms) {
        std::cout << std::left << std::setw(12) << room.name
                  << " polygon_area=" << std::fixed << std::setprecision(2)
                  << room.area_m2 << " m2"
                  << " boundary_loops=" << room.rings.size() << '\n';
    }
    std::cout << "Room adjacencies: " << plan.adjacency.size() << '\n'
              << "SVG exported: mayhem_floor_plan.svg\n";
    return 0;
}
