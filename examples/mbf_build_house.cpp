#include "mbf/multi_floor.hpp"
#include "mbf/residential_layout.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void usage() {
    std::cout
      << "Mayhem Building Forge - Parametric House Generator (prototype)\n"
      << "Usage: mbf_build_house [options]\n"
      << "  --width METRES       Building width (default 12)\n"
      << "  --depth METRES       Building depth (default 9)\n"
      << "  --floors COUNT       Number of repeated floors (default 1)\n"
      << "  --height METRES      Floor-to-floor height (default 3.2)\n"
      << "  --profile NAME       best | balanced | open | privacy (default best)\n"
      << "  --seed INTEGER       Reproducible generation seed (default 1)\n"
      << "  --output FILE.svg    Output floor-plan SVG (default mayhem_house.svg)\n"
      << "  --help               Show this help\n"
      << "\nCurrent prototype: five-room, two-bedroom residential typology.\n";
}
double number(const std::string& value,const char* option) {
    std::size_t used=0;
    const double result=std::stod(value,&used);
    if(used!=value.size())throw std::invalid_argument(std::string("Invalid value for ")+option);
    return result;
}
std::uint64_t integer(const std::string& value,const char* option) {
    std::size_t used=0;
    const auto result=std::stoull(value,&used);
    if(used!=value.size())throw std::invalid_argument(std::string("Invalid value for ")+option);
    return result;
}
}

int main(int argc,char** argv) {
    using namespace mbf;
    try {
        ResidentialLayoutSettings settings;
        std::uint32_t floors=1;
        double floor_height=3.2;
        std::string profile="best",output="mayhem_house.svg";
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--help"){usage();return EXIT_SUCCESS;}
            if(i+1>=argc)throw std::invalid_argument("Missing value after "+arg);
            const std::string value=argv[++i];
            if(arg=="--width")settings.width_m=number(value,"--width");
            else if(arg=="--depth")settings.depth_m=number(value,"--depth");
            else if(arg=="--floors") {
                const auto n=integer(value,"--floors");
                if(n==0||n>200)throw std::invalid_argument("--floors must be between 1 and 200");
                floors=static_cast<std::uint32_t>(n);
            } else if(arg=="--height")floor_height=number(value,"--height");
            else if(arg=="--profile")profile=value;
            else if(arg=="--seed")settings.seed=integer(value,"--seed");
            else if(arg=="--output")output=value;
            else throw std::invalid_argument("Unknown option: "+arg);
        }

        const auto variations=generate_residential_variations(settings);
        if(variations.empty()) {
            std::cerr<<"No valid plan candidates were generated for these dimensions.\n";
            return EXIT_FAILURE;
        }
        const ResidentialVariation* selected=nullptr;
        for(const auto& candidate:variations) {
            const bool match=(profile=="balanced"&&candidate.profile==ResidentialLayoutProfile::BalancedConventional)||
                (profile=="open"&&candidate.profile==ResidentialLayoutProfile::OpenLiving)||
                (profile=="privacy"&&candidate.profile==ResidentialLayoutProfile::PrivacyFirst);
            if(match){selected=&candidate;break;}
        }
        if(profile=="best")selected=&variations.front();
        else if(profile!="balanced"&&profile!="open"&&profile!="privacy")
            throw std::invalid_argument("--profile must be best, balanced, open, or privacy");
        if(!selected)throw std::runtime_error("The requested profile did not pass architectural validation.");

        const auto circulation=validate_physical_circulation(selected->layout);
        if(!circulation.valid) {
            std::cerr<<"Selected plan failed physical circulation validation:\n";
            for(const auto& error:circulation.errors)std::cerr<<" - "<<error<<'\n';
            return EXIT_FAILURE;
        }
        const auto building=generate_stacked_building(selected->layout,floors,floor_height,0.2);
        const auto building_check=validate_multi_floor_building(building);
        if(!building_check.valid) {
            std::cerr<<"Building validation failed:\n";
            for(const auto& error:building_check.errors)std::cerr<<" - "<<error<<'\n';
            return EXIT_FAILURE;
        }
        std::string error;
        if(!export_residential_layout_svg(selected->layout,output,&error))
            throw std::runtime_error("SVG export failed: "+error);

        std::cout<<std::fixed<<std::setprecision(2);
        std::cout<<"\nMAYHEM BUILDING FORGE - GENERATION COMPLETE\n";
        std::cout<<"Selected profile: "<<residential_profile_name(selected->profile)<<'\n';
        std::cout<<"Footprint: "<<settings.width_m<<" m x "<<settings.depth_m<<" m\n";
        std::cout<<"Floors: "<<floors<<" | Total height: "<<building_check.total_height_m<<" m\n";
        std::cout<<"Rooms per floor: "<<selected->layout.rooms.size()<<'\n';
        for(const auto& room:selected->layout.rooms)
            std::cout<<"  "<<room.name<<": "<<room.area_m2<<" m2\n";
        std::cout<<"Candidate score: "<<selected->validation.quality_score<<'\n';
        std::cout<<"SVG floor plan: "<<output<<'\n';
        std::cout<<"NOTE: current prototype repeats one validated two-bedroom plan on every floor.\n";
        return EXIT_SUCCESS;
    } catch(const std::exception& e) {
        std::cerr<<"Generation error: "<<e.what()<<'\n';
        usage();
        return EXIT_FAILURE;
    }
}
