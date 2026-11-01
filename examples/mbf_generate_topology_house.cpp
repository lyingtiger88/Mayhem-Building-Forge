#include "mbf/building_brief.hpp"
#include "mbf/geometry2d.hpp"
#include "mbf/generator.hpp"
#include "mbf/topology.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    using namespace mbf;
    try {
        double width=18.0,depth=12.0;
        double south=0.0,north=0.0,east=0.0,west=0.0;
        std::uint32_t bedrooms=1;
        std::uint64_t seed=42;
        std::string prefix="mayhem_topology_house";
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--help") {
                std::cout<<"Usage: mbf_generate_topology_house [--width M] [--depth M] "
                         <<"[--setback-south M] [--setback-north M] [--setback-east M] "
                         <<"[--setback-west M] [--bedrooms N] [--seed N] [--output-prefix PATH]\n";
                return EXIT_SUCCESS;
            }
            if(i+1>=argc)throw std::invalid_argument("Missing value after "+arg);
            const std::string value=argv[++i];
            if(arg=="--width")width=std::stod(value);
            else if(arg=="--depth")depth=std::stod(value);
            else if(arg=="--setback-south")south=std::stod(value);
            else if(arg=="--setback-north")north=std::stod(value);
            else if(arg=="--setback-east")east=std::stod(value);
            else if(arg=="--setback-west")west=std::stod(value);
            else if(arg=="--bedrooms") {
                const auto n=std::stoul(value);
                if(n<1U||n>4U)throw std::invalid_argument("--bedrooms must be between 1 and 4.");
                bedrooms=static_cast<std::uint32_t>(n);
            } else if(arg=="--seed")seed=std::stoull(value);
            else if(arg=="--output-prefix")prefix=value;
            else throw std::invalid_argument("Unknown option: "+arg);
        }
        auto brief=make_default_residential_brief();
        brief.site_width_m=width;brief.site_depth_m=depth;
        brief.setback_south_m=south;brief.setback_north_m=north;
        brief.setback_east_m=east;brief.setback_west_m=west;
        brief.rooms[2].count=bedrooms;brief.seed=seed;
        const auto brief_check=validate_building_brief(brief);
        if(!brief_check.valid) {
            for(const auto& error:brief_check.errors)std::cerr<<"Brief error: "<<error<<'\n';
            return EXIT_FAILURE;
        }
        const auto topologies=generate_room_topology_candidates(brief);
        if(topologies.empty())throw std::runtime_error("No valid room topology candidates satisfy the brief.");
        GenerationSettings settings;
        settings.width_m=brief_check.buildable_width_m;
        settings.depth_m=brief_check.buildable_depth_m;
        settings.seed=seed;
        FloorPlanGenerator generator;
        std::size_t exported=0;
        std::cout<<"MAYHEM TOPOLOGY-DRIVEN HOUSE GENERATION\n";
        std::cout<<"Buildable footprint: "<<settings.width_m<<" x "<<settings.depth_m<<" m\n";
        std::cout<<"Requested bedrooms: "<<bedrooms<<"\n";
        for(const auto& topology:topologies) {
            try {
                const auto plan=generator.generate_topology_layout(settings,brief,topology);
                const auto geometry=build_geometry_2d(plan);
                if(!geometry.valid) {
                    std::cout<<"Skipped "<<topology.strategy_name<<": geometry conversion failed.\n";
                    continue;
                }
                const std::string path=prefix+"_"+std::to_string(++exported)+".svg";
                std::string error;
                if(!export_geometry_svg(geometry.geometry,path,&error)) {
                    --exported;
                    std::cout<<"Skipped "<<topology.strategy_name<<": "<<error<<'\n';
                    continue;
                }
                std::cout<<"Candidate "<<exported<<": "<<topology.strategy_name
                         <<" | rooms "<<plan.rooms.size()<<" | topology score "
                         <<topology.quality_score<<" | "<<path<<'\n';
            } catch(const std::runtime_error& error) {
                std::cout<<"Skipped "<<topology.strategy_name<<": "<<error.what()<<'\n';
            }
        }
        if(exported==0U) {
            std::cerr<<"No topology candidate could be realized as a valid rectangular floor plan.\n";
            return EXIT_FAILURE;
        }
        std::cout<<"Exported "<<exported<<" topology-driven floor-plan candidate(s).\n";
        std::cout<<"NOTE: this is an early rectangular solver; no building-code or daylight compliance is implied.\n";
        return EXIT_SUCCESS;
    } catch(const std::exception& error) {
        std::cerr<<"Topology house generation error: "<<error.what()<<'\n';
        return EXIT_FAILURE;
    }
}
