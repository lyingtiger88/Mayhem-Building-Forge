#include "mbf/building_brief.hpp"
#include "mbf/geometry2d.hpp"
#include "mbf/generator.hpp"
#include "mbf/topology.hpp"
#include "mbf/spatial_quality.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <set>
#include <sstream>
#include <string>

namespace {
double parse_real(const std::string& value,const std::string& option) {
    std::size_t used=0;
    const double parsed=std::stod(value,&used);
    if(used!=value.size())throw std::invalid_argument("Invalid numeric value for "+option+".");
    return parsed;
}
unsigned long parse_unsigned(const std::string& value,const std::string& option) {
    std::size_t used=0;
    const auto parsed=std::stoul(value,&used);
    if(used!=value.size())throw std::invalid_argument("Invalid integer value for "+option+".");
    return parsed;
}
std::uint64_t parse_seed(const std::string& value) {
    std::size_t used=0;
    const auto parsed=std::stoull(value,&used);
    if(used!=value.size())throw std::invalid_argument("Invalid integer value for --seed.");
    return parsed;
}
}

int main(int argc,char** argv) {
    using namespace mbf;
    try {
        double width=18.0,depth=12.0;
        double living_area=24.0,kitchen_area=12.0,bedroom_area=14.0,bathroom_area=6.0,hallway_area=8.0;
        double south=0.0,north=0.0,east=0.0,west=0.0;
        std::uint32_t bedrooms=1,bathrooms=1,floors=1;
        EntranceSide entrance_side=EntranceSide::South;
        std::uint64_t seed=42;
        std::string prefix="mayhem_topology_house";
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--help") {
                std::cout<<"Usage: mbf_generate_topology_house [--width M] [--depth M] "
                         <<"[--setback-south M] [--setback-north M] [--setback-east M] "
                         <<"[--setback-west M] [--bedrooms N] [--bathrooms N] [--living-area M2] [--kitchen-area M2] [--bedroom-area M2] [--bathroom-area M2] [--hallway-area M2] [--entrance-side south|north|east|west] [--floors N] [--seed N] [--output-prefix PATH]\n";
                return EXIT_SUCCESS;
            }
            if(i+1>=argc)throw std::invalid_argument("Missing value after "+arg);
            const std::string value=argv[++i];
            if(arg=="--width")width=parse_real(value,arg);
            else if(arg=="--depth")depth=std::stod(value);
            else if(arg=="--setback-south")south=std::stod(value);
            else if(arg=="--setback-north")north=std::stod(value);
            else if(arg=="--setback-east")east=std::stod(value);
            else if(arg=="--setback-west")west=std::stod(value);
            else if(arg=="--bedrooms") {
                const auto n=parse_unsigned(value,arg);
                if(n<1U||n>4U)throw std::invalid_argument("--bedrooms must be between 1 and 4.");
                bedrooms=static_cast<std::uint32_t>(n);
            } else if(arg=="--bathrooms") {
                const auto n=std::stoul(value);
                if(n<1U||n>4U)throw std::invalid_argument("--bathrooms must be between 1 and 4.");
                bathrooms=static_cast<std::uint32_t>(n);
            } else if(arg=="--living-area")living_area=std::stod(value);
            else if(arg=="--kitchen-area")kitchen_area=std::stod(value);
            else if(arg=="--bedroom-area")bedroom_area=std::stod(value);
            else if(arg=="--bathroom-area")bathroom_area=std::stod(value);
            else if(arg=="--hallway-area")hallway_area=std::stod(value);
            else if(arg=="--entrance-side") {
                if(value=="south")entrance_side=EntranceSide::South;
                else if(value=="north")entrance_side=EntranceSide::North;
                else if(value=="east")entrance_side=EntranceSide::East;
                else if(value=="west")entrance_side=EntranceSide::West;
                else throw std::invalid_argument("--entrance-side must be south, north, east, or west.");
            } else if(arg=="--floors") {
                const auto n=std::stoul(value);
                if(n!=1U)throw std::invalid_argument("The topology-driven geometric solver currently supports one floor. Multi-floor generation is not yet integrated.");
                floors=static_cast<std::uint32_t>(n);
            } else if(arg=="--seed")seed=parse_seed(value);
            else if(arg=="--output-prefix")prefix=value;
            else throw std::invalid_argument("Unknown option: "+arg);
        }
        auto brief=make_default_residential_brief();
        brief.site_width_m=width;brief.site_depth_m=depth;
        brief.setback_south_m=south;brief.setback_north_m=north;
        brief.setback_east_m=east;brief.setback_west_m=west;
        brief.rooms[0].target_area_m2=living_area;brief.rooms[1].target_area_m2=kitchen_area;
        brief.rooms[2].target_area_m2=bedroom_area;brief.rooms[3].target_area_m2=bathroom_area;
        brief.rooms[4].target_area_m2=hallway_area;
        brief.rooms[2].count=bedrooms;brief.rooms[3].count=bathrooms;brief.floor_count=floors;brief.entrance_side=entrance_side;brief.seed=seed;
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
        std::set<std::string> geometric_signatures;
        std::cout<<"MAYHEM TOPOLOGY-DRIVEN HOUSE GENERATION\n";
        std::cout<<"Buildable footprint: "<<settings.width_m<<" x "<<settings.depth_m<<" m\n";
        std::cout<<"Requested bedrooms: "<<bedrooms<<"\n";
        std::cout<<"Requested bathrooms: "<<bathrooms<<"\n";
        std::cout<<"Target room areas (m2): living "<<living_area<<", kitchen "<<kitchen_area
                 <<", bedroom "<<bedroom_area<<", bathroom "<<bathroom_area
                 <<", hallway "<<hallway_area<<"\n";
        for(const auto& topology:topologies) {
            try {
                const auto plan=generator.generate_topology_layout(settings,brief,topology);
                std::ostringstream signature;
                signature.precision(2);
                signature.setf(std::ios::fixed);
                for(const auto& room:plan.rooms)
                    signature<<room.id<<':'<<room.bounds.x<<','<<room.bounds.y<<','
                             <<room.bounds.width<<','<<room.bounds.height<<';';
                if(!geometric_signatures.insert(signature.str()).second) {
                    std::cout<<"Skipped "<<topology.strategy_name<<": duplicate floor-plan geometry.\n";
                    continue;
                }
                const auto quality=evaluate_spatial_quality(plan,brief);
                const auto geometry=build_geometry_2d(plan);
                if(!geometry.valid) {
                    std::cout<<"Skipped "<<topology.strategy_name<<": geometry conversion failed.\n";
                    continue;
                }
                auto walls=build_wall_topology(geometry.geometry);
                if(!walls.valid) {
                    std::cout<<"Skipped "<<topology.strategy_name<<": wall topology invalid.\\n";
                    continue;
                }
                OpeningSettings2D opening_settings;
                opening_settings.interior_door_width_m=0.9;
                opening_settings.window_width_m=1.2;
                const auto opening_ids=generate_residential_openings(walls,plan,opening_settings);
                std::vector<RoomId> room_ids;
                for(const auto& room:plan.rooms)room_ids.push_back(room.id);
                const auto access=build_circulation_graph(walls,room_ids,0.8);
                if(!access.valid) {
                    std::cout<<"Skipped "<<topology.strategy_name<<": access validation failed.\\n";
                    for(const auto& diagnostic:access.diagnostics)std::cout<<"  "<<diagnostic<<'\\n';
                    continue;
                }
                const auto doors=std::count_if(walls.openings.begin(),walls.openings.end(),
                    [](const WallOpening2D& opening){return opening.kind==OpeningKind2D::EntranceDoor||
                        opening.kind==OpeningKind2D::InteriorDoor;});
                const auto windows=std::count_if(walls.openings.begin(),walls.openings.end(),
                    [](const WallOpening2D& opening){return opening.kind==OpeningKind2D::Window;});
                const auto open_plan=std::count_if(walls.openings.begin(),walls.openings.end(),
                    [](const WallOpening2D& opening){return opening.kind==OpeningKind2D::OpenPlan;});
                const std::string path=prefix+"_"+std::to_string(++exported)+".svg";
                std::string error;
                if(!export_wall_topology_svg(geometry.geometry,walls,path,&error)) {
                    --exported;
                    std::cout<<"Skipped "<<topology.strategy_name<<": "<<error<<'\n';
                    continue;
                }
                std::cout<<"Candidate "<<exported<<": "<<topology.strategy_name
                         <<" | rooms "<<plan.rooms.size()<<" | topology score "
                         <<topology.quality_score<<" | daylight "<<quality.daylight_score
                         <<" | privacy "<<quality.privacy_score<<" | circulation "
                         <<quality.circulation_score<<" | overall "<<quality.overall_score
                         <<" | doors "<<doors<<" | windows "<<windows<<" | open-plan "<<open_plan
                         <<" | accessible "<<access.reachable_room_ids.size()<<"/"<<room_ids.size()
                         <<" | openings "<<opening_ids.size()<<" | "<<path<<'\n';
                for(const auto& warning:quality.warnings)
                    std::cout<<"Warning Candidate "<<exported<<": "<<warning<<'\n';
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
