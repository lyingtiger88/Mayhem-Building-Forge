#include "mbf/spatial_quality.hpp"
#include "mbf/floor_plan.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>

namespace mbf {
namespace {
double clamp01(double value) { return std::clamp(value,0.0,1.0); }
bool touches(double a,double b) { return std::abs(a-b)<=1e-6; }
double daylight_weight(RoomRole role) {
    switch(role) {
        case RoomRole::Living:return 1.0;
        case RoomRole::Bedroom:return 0.9;
        case RoomRole::Kitchen:return 0.75;
        case RoomRole::Public:return 0.8;
        case RoomRole::Private:return 0.8;
        case RoomRole::Bathroom:return 0.25;
        case RoomRole::Hallway:case RoomRole::Circulation:return 0.15;
        case RoomRole::Service:return 0.2;
        case RoomRole::Auto:return 0.5;
    }
    return 0.5;
}
double privacy_penalty(RoomRole a,RoomRole b) {
    if((a==RoomRole::Living&&b==RoomRole::Bedroom)||(b==RoomRole::Living&&a==RoomRole::Bedroom))return 18.0;
    if((a==RoomRole::Living&&b==RoomRole::Bathroom)||(b==RoomRole::Living&&a==RoomRole::Bathroom))return 24.0;
    if((a==RoomRole::Kitchen&&b==RoomRole::Bedroom)||(b==RoomRole::Kitchen&&a==RoomRole::Bedroom))return 8.0;
    if((a==RoomRole::Public&&b==RoomRole::Private)||(b==RoomRole::Public&&a==RoomRole::Private))return 16.0;
    if((a==RoomRole::Public&&b==RoomRole::Bedroom)||(b==RoomRole::Public&&a==RoomRole::Bedroom))return 18.0;
    return 0.0;
}
}

SpatialQualityReport evaluate_spatial_quality(const FloorPlan& plan,const BuildingBrief& brief) {
    SpatialQualityReport report;
    const auto validation=validate_floor_plan(plan);
    if(!validation.valid) {
        report.warnings=validation.errors;
        return report;
    }
    if(plan.rooms.empty()||!(plan.width_m>0.0)||!(plan.depth_m>0.0)) {
        report.warnings.push_back("Spatial quality requires a non-empty plan with positive dimensions.");
        return report;
    }
    std::map<RoomId,std::size_t> index_by_id;
    for(std::size_t i=0;i<plan.rooms.size();++i)index_by_id[plan.rooms[i].id]=i;
    std::vector<double> privacy(plan.rooms.size(),100.0);
    double daylight_weight_sum=0.0,daylight_total=0.0;
    for(std::size_t i=0;i<plan.rooms.size();++i) {
        const auto& room=plan.rooms[i];
        const auto& b=room.bounds;
        double frontage=0.0;
        if(touches(b.y,0.0))frontage+=b.width;
        if(touches(b.y+b.height,plan.depth_m))frontage+=b.width;
        if(touches(b.x,0.0))frontage+=b.height;
        if(touches(b.x+b.width,plan.width_m))frontage+=b.height;
        const double scale=2.0*std::sqrt(std::max(room.actual_area_m2,1e-9));
        const double potential=clamp01(frontage/std::max(scale,1e-9));
        report.rooms.push_back({room.id,frontage,potential,100.0});
        const double weight=daylight_weight(room.role);
        daylight_total+=potential*weight;
        daylight_weight_sum+=weight;
        if(frontage<=1e-7&&(room.role==RoomRole::Living||room.role==RoomRole::Bedroom||
                            room.role==RoomRole::Kitchen))
            report.warnings.push_back(room.name+" has no exterior facade exposure; daylight potential is very low.");
    }
    report.daylight_score=daylight_weight_sum>0.0
        ?100.0*daylight_total/daylight_weight_sum:0.0;

    for(const auto& edge:plan.adjacency) {
        const auto ia=index_by_id.find(edge.first),ib=index_by_id.find(edge.second);
        if(ia==index_by_id.end()||ib==index_by_id.end())continue;
        const auto penalty=privacy_penalty(plan.rooms[ia->second].role,plan.rooms[ib->second].role);
        if(penalty>0.0) {
            privacy[ia->second]-=penalty*0.5;
            privacy[ib->second]-=penalty*0.5;
        }
    }
    double privacy_total=0.0;
    for(std::size_t i=0;i<plan.rooms.size();++i) {
        privacy[i]=std::clamp(privacy[i],0.0,100.0);
        report.rooms[i].privacy_score=privacy[i];
        const auto role=plan.rooms[i].role;
        const double weight=(role==RoomRole::Bedroom||role==RoomRole::Bathroom||
                             role==RoomRole::Private)?1.5:1.0;
        privacy_total+=privacy[i]*weight;
    }
    double privacy_weights=0.0;
    for(const auto& room:plan.rooms)
        privacy_weights+=(room.role==RoomRole::Bedroom||room.role==RoomRole::Bathroom||
                          room.role==RoomRole::Private)?1.5:1.0;
    report.privacy_score=privacy_weights>0.0?privacy_total/privacy_weights:100.0;

    std::map<RoomId,std::vector<RoomId>> graph;
    for(const auto& room:plan.rooms)graph[room.id];
    for(const auto& edge:plan.adjacency) {
        if(graph.contains(edge.first)&&graph.contains(edge.second)) {
            graph[edge.first].push_back(edge.second);
            graph[edge.second].push_back(edge.first);
        }
    }
    std::set<RoomId> reached;
    std::queue<RoomId> pending;
    pending.push(plan.rooms.front().id);reached.insert(plan.rooms.front().id);
    while(!pending.empty()) {
        const RoomId current=pending.front();pending.pop();
        for(const RoomId next:graph[current])
            if(reached.insert(next).second)pending.push(next);
    }
    report.circulation_connected=reached.size()==plan.rooms.size();
    const double reachability=100.0*static_cast<double>(reached.size())/
                              static_cast<double>(plan.rooms.size());
    const double link_density=plan.rooms.size()<=1U?1.0:
        std::min(1.0,static_cast<double>(plan.adjacency.size())/
                       static_cast<double>(plan.rooms.size()-1U));
    report.circulation_score=report.circulation_connected
        ? 70.0+30.0*link_density:reachability*0.7;
    if(!report.circulation_connected)
        report.warnings.push_back("The room adjacency graph is disconnected; review access and circulation.");
    if(!plan.entrance.defined)
        report.warnings.push_back("No exterior entrance is defined in this floor plan.");

    report.overall_score=0.35*report.daylight_score+
                         0.35*report.privacy_score+
                         0.30*report.circulation_score;
    report.valid=true;
    (void)brief; // Brief context is retained in the API for future rule-profile scoring.
    return report;
}
} // namespace mbf
