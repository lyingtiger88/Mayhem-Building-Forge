#include "mbf/residential_layout.hpp"
#include <iomanip>
#include <iostream>

int main() {
    using namespace mbf;
    ResidentialLayoutSettings settings;
    settings.width_m=12.0;
    settings.depth_m=9.0;
    settings.seed=20261031;

    const auto plan=generate_balanced_residential_layout(settings);
    const auto validation=validate_residential_layout(plan);
    if(!validation.valid) {
        for(const auto&error:validation.errors)std::cerr<<"ERROR: "<<error<<'\n';
        return 1;
    }

    std::cout<<"Mayhem Building Forge - Balanced Residential Layout\n";
    std::cout<<std::fixed<<std::setprecision(2);
    for(const auto&room:plan.rooms)
        std::cout<<room.name<<": "<<room.area_m2<<" m2\n";
    std::cout<<"Openings: "<<plan.openings.size()<<"\n";

    std::string error;
    if(!export_residential_layout_svg(plan,"mayhem_house_plan.svg",&error)) {
        std::cerr<<"SVG export failed: "<<error<<'\n';
        return 2;
    }
    std::cout<<"SVG exported: mayhem_house_plan.svg\n";
    return 0;
}
