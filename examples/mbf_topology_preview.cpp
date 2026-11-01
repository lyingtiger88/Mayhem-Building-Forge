#include "mbf/topology.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    try {
        std::uint32_t bedrooms=1;
        std::string output="mayhem_topology_candidates.svg";
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--help") {
                std::cout<<"Usage: mbf_topology_preview [--bedrooms COUNT] [--output FILE.svg]\n";
                return EXIT_SUCCESS;
            }
            if(i+1>=argc)throw std::invalid_argument("Missing value after "+arg);
            const std::string value=argv[++i];
            if(arg=="--bedrooms") {
                const auto parsed=std::stoul(value);
                if(parsed<1U||parsed>4U)throw std::invalid_argument("--bedrooms must be between 1 and 4 for this default brief.");
                bedrooms=static_cast<std::uint32_t>(parsed);
            } else if(arg=="--output")output=value;
            else throw std::invalid_argument("Unknown option: "+arg);
        }
        auto brief=mbf::make_default_residential_brief();
        brief.rooms[2].count=bedrooms;
        const auto validation=mbf::validate_building_brief(brief);
        if(!validation.valid) {
            for(const auto& error:validation.errors)std::cerr<<"Brief error: "<<error<<'\n';
            return EXIT_FAILURE;
        }
        const auto candidates=mbf::generate_room_topology_candidates(brief);
        if(candidates.empty()) {
            std::cerr<<"No valid room topology candidates satisfy the brief.\n";
            return EXIT_FAILURE;
        }
        std::string error;
        if(!mbf::export_room_topologies_svg(candidates,output,&error))
            throw std::runtime_error(error);
        std::cout<<"MAYHEM ROOM TOPOLOGY CANDIDATES\n"
                 <<"Bedrooms: "<<bedrooms<<'\n'
                 <<"Distinct valid graph candidates: "<<candidates.size()<<'\n';
        for(const auto& candidate:candidates)
            std::cout<<" - "<<candidate.strategy_name<<": "<<candidate.nodes.size()
                     <<" rooms, "<<candidate.edges.size()<<" links, score "
                     <<candidate.quality_score<<'\n';
        std::cout<<"Graph SVG: "<<output<<'\n'
                 <<"NOTE: these are room-adjacency graphs, not solved floor-plan geometry.\n";
        return EXIT_SUCCESS;
    } catch(const std::exception& error) {
        std::cerr<<"Topology preview error: "<<error.what()<<'\n';
        return EXIT_FAILURE;
    }
}
