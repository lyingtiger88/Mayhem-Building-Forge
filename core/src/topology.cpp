#include "mbf/topology.hpp"
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>

namespace mbf {
namespace {
using RoleMap = std::map<RoomRole, std::vector<RoomId>>;
using RolePair = std::pair<RoomRole, RoomRole>;

RolePair ordered_roles(RoomRole a, RoomRole b) {
    if (static_cast<int>(a) > static_cast<int>(b)) std::swap(a,b);
    return {a,b};
}
std::pair<RoomId,RoomId> ordered_ids(RoomId a,RoomId b) {
    if(a>b)std::swap(a,b);
    return {a,b};
}
bool is_avoided(const BuildingBrief& brief, RoomRole a, RoomRole b) {
    for(const auto& rule:brief.adjacency)
        if(rule.strength==AdjacencyStrength::Avoid &&
           ordered_roles(rule.first,rule.second)==ordered_roles(a,b)) return true;
    return false;
}
bool has_edge(const RoomTopology& graph, RoomId a, RoomId b) {
    const auto key=ordered_ids(a,b);
    return std::any_of(graph.edges.begin(),graph.edges.end(),[&](const auto& edge){
        return ordered_ids(edge.first,edge.second)==key;
    });
}
void add_edge(RoomTopology& graph,const BuildingBrief& brief,RoomId a,RoomId b,
              bool required,double weight) {
    if(a==b||has_edge(graph,a,b))return;
    const auto node_a=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==a;});
    const auto node_b=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==b;});
    if(node_a==graph.nodes.end()||node_b==graph.nodes.end())return;
    if(is_avoided(brief,node_a->role,node_b->role))return;
    graph.edges.push_back({std::min(a,b),std::max(a,b),required,weight});
}
RoomRole hub_role(const RoleMap& roles) {
    for(RoomRole role:{RoomRole::Circulation,RoomRole::Hallway,RoomRole::Living,RoomRole::Public})
        if(roles.contains(role)&&!roles.at(role).empty())return role;
    return roles.begin()->first;
}
std::uint32_t role_rank(RoomRole role) {
    switch(role) {
        case RoomRole::Living: case RoomRole::Kitchen: case RoomRole::Public:return 0;
        case RoomRole::Hallway: case RoomRole::Circulation:return 1;
        case RoomRole::Bedroom: case RoomRole::Private:return 2;
        case RoomRole::Bathroom: case RoomRole::Service:return 3;
        case RoomRole::Auto:return 4;
    }
    return 4;
}
bool connected(const RoomTopology& graph) {
    if(graph.nodes.empty())return false;
    std::map<RoomId,std::vector<RoomId>> adjacency;
    for(const auto& node:graph.nodes)adjacency[node.id];
    for(const auto& edge:graph.edges) {
        adjacency[edge.first].push_back(edge.second);
        adjacency[edge.second].push_back(edge.first);
    }
    std::set<RoomId> visited;
    std::queue<RoomId> pending;
    pending.push(graph.nodes.front().id);visited.insert(graph.nodes.front().id);
    while(!pending.empty()) {
        const auto id=pending.front();pending.pop();
        for(RoomId next:adjacency[id])if(visited.insert(next).second)pending.push(next);
    }
    return visited.size()==graph.nodes.size();
}
std::string signature(const RoomTopology& graph) {
    std::vector<std::pair<RoomId,RoomId>> pairs;
    for(const auto& edge:graph.edges)pairs.push_back(ordered_ids(edge.first,edge.second));
    std::sort(pairs.begin(),pairs.end());
    std::ostringstream out;
    for(const auto& pair:pairs)out<<pair.first<<'-'<<pair.second<<';';
    return out.str();
}
void apply_required_rules(RoomTopology& graph,const BuildingBrief& brief,const RoleMap& roles) {
    for(const auto& rule:brief.adjacency) {
        if(rule.strength!=AdjacencyStrength::Required)continue;
        const auto& first=roles.at(rule.first);
        const auto& second=roles.at(rule.second);
        for(RoomId a:first)for(RoomId b:second)
            add_edge(graph,brief,a,b,true,rule.weight);
    }
}
void score_graph(RoomTopology& graph,const BuildingBrief& brief) {
    double preferred=0.0;
    for(const auto& rule:brief.adjacency) {
        if(rule.strength!=AdjacencyStrength::Preferred)continue;
        const auto first_role=rule.first,second_role=rule.second;
        bool satisfied=false;
        for(const auto& edge:graph.edges) {
            const auto a=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==edge.first;});
            const auto b=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==edge.second;});
            if(a!=graph.nodes.end()&&b!=graph.nodes.end()&&
               ordered_roles(a->role,b->role)==ordered_roles(first_role,second_role)) {satisfied=true;break;}
        }
        if(satisfied)preferred+=rule.weight;
    }
    const double complexity=static_cast<double>(graph.edges.size());
    graph.quality_score=preferred*10.0-complexity*0.15;
}
}

bool validate_room_topology(const RoomTopology& topology,const BuildingBrief& brief,
                            std::vector<std::string>* diagnostics) {
    std::vector<std::string> local;
    const auto fail=[&](const std::string& message){local.push_back(message);};
    const auto brief_validation=validate_building_brief(brief);
    if(!brief_validation.valid)fail("Cannot validate topology against an invalid Building Brief.");
    if(topology.nodes.empty())fail("Topology has no room nodes.");
    std::set<RoomId> ids;
    for(const auto& node:topology.nodes) {
        if(!ids.insert(node.id).second)fail("Topology contains duplicate room node IDs.");
        if(node.name.empty())fail("Topology room node has an empty name.");
    }
    std::set<std::pair<RoomId,RoomId>> edge_ids;
    for(const auto& edge:topology.edges) {
        const auto key=ordered_ids(edge.first,edge.second);
        if(edge.first==edge.second)fail("Topology contains a self-adjacency edge.");
        if(!ids.contains(edge.first)||!ids.contains(edge.second))
            fail("Topology edge references an unknown room node.");
        if(!edge_ids.insert(key).second)fail("Topology contains duplicate adjacency edges.");
        const auto a=std::find_if(topology.nodes.begin(),topology.nodes.end(),[&](const auto& n){return n.id==edge.first;});
        const auto b=std::find_if(topology.nodes.begin(),topology.nodes.end(),[&](const auto& n){return n.id==edge.second;});
        if(a!=topology.nodes.end()&&b!=topology.nodes.end()&&is_avoided(brief,a->role,b->role))
            fail("Topology violates an adjacency rule marked Avoid.");
    }
    std::map<RoomRole,std::vector<RoomId>> roles;
    for(const auto& node:topology.nodes)roles[node.role].push_back(node.id);
    for(const auto& rule:brief.adjacency) {
        if(rule.strength!=AdjacencyStrength::Required)continue;
        const auto first=roles.find(rule.first),second=roles.find(rule.second);
        if(first==roles.end()||second==roles.end()) {
            fail("A required adjacency role is absent from the generated topology.");
            continue;
        }
        for(RoomId id:first->second) {
            bool found=false;
            for(const auto& edge:topology.edges)
                if((edge.first==id&&std::find(second->second.begin(),second->second.end(),edge.second)!=second->second.end())||
                   (edge.second==id&&std::find(second->second.begin(),second->second.end(),edge.first)!=second->second.end()))found=true;
            if(!found)fail("A room instance does not satisfy a required adjacency to the paired role.");
        }
        for(RoomId id:second->second) {
            bool found=false;
            for(const auto& edge:topology.edges)
                if((edge.first==id&&std::find(first->second.begin(),first->second.end(),edge.second)!=first->second.end())||
                   (edge.second==id&&std::find(first->second.begin(),first->second.end(),edge.first)!=first->second.end()))found=true;
            if(!found)fail("A room instance does not satisfy a required adjacency from the paired role.");
        }
    }
    if(topology.nodes.size()>1U&&!connected(topology))
        fail("Room topology is disconnected; at least one room cannot be reached through the adjacency graph.");
    if(diagnostics)*diagnostics=local;
    return local.empty();
}

std::vector<RoomTopology> generate_room_topology_candidates(const BuildingBrief& brief) {
    const auto validation=validate_building_brief(brief);
    if(!validation.valid)return {};
    RoomTopology base;base.seed=brief.seed;
    RoleMap roles;
    RoomId next_id=0;
    for(const auto& requirement:brief.rooms) {
        for(std::uint32_t instance=0;instance<requirement.count;++instance) {
            std::string name=requirement.name;
            if(requirement.count>1U)name+=" "+std::to_string(instance+1U);
            base.nodes.push_back({next_id,name,requirement.role,instance});
            roles[requirement.role].push_back(next_id++);
        }
    }
    apply_required_rules(base,brief,roles);

    std::vector<RoomTopology> candidates;
    std::set<std::string> signatures;
    const auto finish=[&](RoomTopology graph) {
        std::vector<std::string> diagnostics;
        graph.valid=validate_room_topology(graph,brief,&diagnostics);
        graph.diagnostics=std::move(diagnostics);
        if(!graph.valid)return;
        score_graph(graph,brief);
        const auto key=signature(graph);
        if(signatures.insert(key).second)candidates.push_back(std::move(graph));
    };

    // Strategy 1: a circulation/hall hub connects each accessible room.
    {
        auto graph=base;graph.strategy=TopologyStrategy::CirculationHub;
        graph.strategy_name="Circulation Hub";
        const auto role=hub_role(roles);
        const auto& hubs=roles.at(role);
        for(RoomId hub:hubs)for(const auto& node:graph.nodes)
            if(node.id!=hub)add_edge(graph,brief,hub,node.id,false,1.0);
        for(const auto& rule:brief.adjacency)
            if(rule.strength==AdjacencyStrength::Preferred)
                for(RoomId a:roles.at(rule.first))for(RoomId b:roles.at(rule.second))
                    add_edge(graph,brief,a,b,false,rule.weight);
        finish(std::move(graph));
    }

    // Strategy 2: public rooms form a cluster; private/service rooms connect
    // to the circulation spine. A public-to-spine link keeps the graph traversable.
    {
        auto graph=base;graph.strategy=TopologyStrategy::PublicPrivateSpine;
        graph.strategy_name="Public / Private Spine";
        const auto hub=roles.find(RoomRole::Hallway)!=roles.end()?RoomRole::Hallway:
                       roles.find(RoomRole::Circulation)!=roles.end()?RoomRole::Circulation:hub_role(roles);
        const auto& hubs=roles.at(hub);
        std::vector<RoomId> public_nodes;
        for(RoomRole role:{RoomRole::Living,RoomRole::Kitchen,RoomRole::Public})
            if(roles.contains(role))public_nodes.insert(public_nodes.end(),roles.at(role).begin(),roles.at(role).end());
        for(std::size_t i=1;i<public_nodes.size();++i)
            add_edge(graph,brief,public_nodes[i-1],public_nodes[i],false,1.0);
        for(RoomId h:hubs)for(const auto& node:graph.nodes)
            if(node.id!=h&&role_rank(node.role)>=1U)
                add_edge(graph,brief,h,node.id,false,1.0);
        if(!public_nodes.empty())
            add_edge(graph,brief,hubs.front(),public_nodes.back(),false,1.0);
        finish(std::move(graph));
    }

    // Strategy 3: a compact chain ordered by public, circulation, private, service.
    {
        auto graph=base;graph.strategy=TopologyStrategy::CompactLinear;
        graph.strategy_name="Compact Linear";
        std::vector<TopologyNode> ordered=graph.nodes;
        std::stable_sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){
            if(role_rank(a.role)!=role_rank(b.role))return role_rank(a.role)<role_rank(b.role);
            return a.id<b.id;
        });
        for(std::size_t i=1;i<ordered.size();++i)
            add_edge(graph,brief,ordered[i-1].id,ordered[i].id,false,0.8);
        finish(std::move(graph));
    }
    std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){
        if(a.quality_score!=b.quality_score)return a.quality_score>b.quality_score;
        return static_cast<int>(a.strategy)<static_cast<int>(b.strategy);
    });
    return candidates;
}
} // namespace mbf
