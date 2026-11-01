#include "mbf/building_brief.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace mbf {
namespace {
bool finite_positive(double value) {
    return std::isfinite(value)&&value>0.0;
}
bool known_role(RoomRole role) {
    switch(role) {
        case RoomRole::Living: case RoomRole::Kitchen: case RoomRole::Bedroom:
        case RoomRole::Bathroom: case RoomRole::Hallway: case RoomRole::Public:
        case RoomRole::Private: case RoomRole::Service: case RoomRole::Circulation:
            return true;
        case RoomRole::Auto: return false;
    }
    return false;
}
}

BuildingBrief make_default_residential_brief() {
    BuildingBrief brief;
    brief.project_name="Default Residential House";
    brief.site_width_m=12.0;
    brief.site_depth_m=9.0;
    brief.rooms={
        {"Living",RoomRole::Living,1,24.0,16.0,3.0,3.0,1.0},
        {"Kitchen",RoomRole::Kitchen,1,12.0,8.0,2.4,2.4,1.0},
        {"Bedroom",RoomRole::Bedroom,1,14.0,10.0,2.8,2.8,1.0},
        {"Bathroom",RoomRole::Bathroom,1,6.0,3.5,1.5,1.8,0.8},
        {"Hallway",RoomRole::Hallway,1,8.0,5.0,1.0,2.0,0.8}
    };
    brief.adjacency={
        {RoomRole::Living,RoomRole::Kitchen,AdjacencyStrength::Required,1.0},
        {RoomRole::Living,RoomRole::Hallway,AdjacencyStrength::Preferred,0.8},
        {RoomRole::Kitchen,RoomRole::Hallway,AdjacencyStrength::Required,1.0},
        {RoomRole::Hallway,RoomRole::Bedroom,AdjacencyStrength::Required,1.0},
        {RoomRole::Hallway,RoomRole::Bathroom,AdjacencyStrength::Required,1.0},
        {RoomRole::Living,RoomRole::Bathroom,AdjacencyStrength::Avoid,1.0}
    };
    return brief;
}

BuildingBriefValidation validate_building_brief(const BuildingBrief& brief) {
    BuildingBriefValidation result;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(brief.schema_version!=1U)fail("Unsupported building brief schema version.");
    if(brief.project_name.empty())fail("Project name must not be empty.");
    if(!finite_positive(brief.site_width_m)||!finite_positive(brief.site_depth_m))
        fail("Site dimensions must be finite and positive.");
    const double setbacks[]={brief.setback_south_m,brief.setback_north_m,
                              brief.setback_east_m,brief.setback_west_m};
    for(double setback:setbacks)
        if(!std::isfinite(setback)||setback<0.0)
            fail("Setbacks must be finite and non-negative.");
    if(brief.floor_count==0U||brief.floor_count>200U)
        fail("Floor count must be between 1 and 200.");
    if(!std::isfinite(brief.floor_to_floor_height_m)||
       brief.floor_to_floor_height_m<2.7||brief.floor_to_floor_height_m>6.0)
        fail("Floor-to-floor height must be between 2.7 m and 6.0 m.");
    result.buildable_width_m=brief.site_width_m-brief.setback_east_m-brief.setback_west_m;
    result.buildable_depth_m=brief.site_depth_m-brief.setback_south_m-brief.setback_north_m;
    if(!std::isfinite(result.buildable_width_m)||!std::isfinite(result.buildable_depth_m)||
       result.buildable_width_m<=0.0||result.buildable_depth_m<=0.0) {
        fail("Setbacks consume the entire site; no buildable footprint remains.");
    } else {
        result.buildable_area_m2=result.buildable_width_m*result.buildable_depth_m;
    }

    std::set<std::string> names;
    std::set<RoomRole> present_roles;
    for(const auto& room:brief.rooms) {
        if(room.name.empty())fail("Every room requirement needs a name.");
        else if(!names.insert(room.name).second)
            fail("Room requirement names must be unique: "+room.name+".");
        if(!known_role(room.role))fail("Room "+room.name+" must have an explicit supported semantic role.");
        else present_roles.insert(room.role);
        if(room.count==0U)fail("Room "+room.name+" count must be at least one.");
        if(!finite_positive(room.target_area_m2)||!finite_positive(room.minimum_area_m2)||
           !finite_positive(room.minimum_width_m)||!finite_positive(room.minimum_depth_m))
            fail("Room "+room.name+" dimensions and areas must be finite and positive.");
        if(std::isfinite(room.target_area_m2)&&std::isfinite(room.minimum_area_m2)&&
           room.target_area_m2<room.minimum_area_m2)
            fail("Room "+room.name+" target area cannot be below its minimum area.");
        if(!std::isfinite(room.priority)||room.priority<=0.0)
            fail("Room "+room.name+" priority must be finite and positive.");
        if(room.count>0U&&std::isfinite(room.target_area_m2)&&room.target_area_m2>0.0&&
           std::isfinite(room.minimum_area_m2)&&room.minimum_area_m2>0.0) {
            result.requested_target_room_area_m2+=
                static_cast<double>(room.count)*room.target_area_m2;
            result.requested_minimum_room_area_m2+=
                static_cast<double>(room.count)*room.minimum_area_m2;
        }
    }
    if(brief.rooms.empty())fail("Building brief must request at least one room.");
    if(result.buildable_area_m2>0.0&&
       result.requested_minimum_room_area_m2>result.buildable_area_m2+1e-7)
        fail("The sum of minimum room areas exceeds the available buildable area on one floor.");
    else if(result.buildable_area_m2>0.0&&
       result.requested_target_room_area_m2>result.buildable_area_m2+1e-7)
        result.warnings.push_back("Requested target room areas exceed the buildable area; the solver must negotiate reductions or a different floor schedule.");

    for(const auto& relation:brief.adjacency) {
        if(!known_role(relation.first)||!known_role(relation.second)||
           relation.first==relation.second)
            fail("Adjacency rules must reference two distinct explicit room roles.");
        if(!std::isfinite(relation.weight)||relation.weight<=0.0)
            fail("Adjacency rule weights must be finite and positive.");
        if(!present_roles.contains(relation.first)||!present_roles.contains(relation.second))
            fail("An adjacency rule references a room role absent from the room schedule.");
    }
    result.valid=result.errors.empty();
    return result;
}

PlannerCompatibility check_current_planner_compatibility(const BuildingBrief& brief) {
    PlannerCompatibility result;
    const auto validation=validate_building_brief(brief);
    if(!validation.valid) {
        result.limitations.push_back("The building brief is invalid; correct its validation errors before generation.");
        return result;
    }
    const auto count=[&](RoomRole role) {
        std::uint64_t total=0;
        for(const auto& room:brief.rooms)if(room.role==role)total+=room.count;
        return total;
    };
    const RoomRole required[]={RoomRole::Living,RoomRole::Kitchen,RoomRole::Bedroom,
                               RoomRole::Bathroom,RoomRole::Hallway};
    for(RoomRole role:required)
        if(count(role)!=1U)
            result.limitations.push_back("Current residential planner requires exactly one room with role "+
                std::to_string(static_cast<int>(role))+".");
    for(const auto& room:brief.rooms)
        if(std::find(std::begin(required),std::end(required),room.role)==std::end(required))
            result.limitations.push_back("Current residential planner does not yet generate room role: "+room.name+".");
    if(brief.entrance_side!=EntranceSide::South)
        result.limitations.push_back("Current residential planner supports a south-side entrance only.");
    if(validation.buildable_width_m<8.0||validation.buildable_depth_m<7.0)
        result.limitations.push_back("Current residential topology generator requires a buildable footprint of at least 8 m by 7 m.");
    const auto is_supported_relation=[](RoomRole a,RoomRole b) {
        if(a>b)std::swap(a,b);
        return (a==RoomRole::Living&&b==RoomRole::Kitchen)||
               (a==RoomRole::Living&&b==RoomRole::Hallway)||
               (a==RoomRole::Kitchen&&b==RoomRole::Hallway)||
               (a==RoomRole::Bedroom&&b==RoomRole::Hallway)||
               (a==RoomRole::Bathroom&&b==RoomRole::Hallway);
    };
    for(const auto& relation:brief.adjacency)
        if(relation.strength==AdjacencyStrength::Required&&
           !is_supported_relation(relation.first,relation.second))
            result.limitations.push_back("Current residential planner cannot guarantee requested required adjacency between roles "+
                std::to_string(static_cast<int>(relation.first))+" and "+
                std::to_string(static_cast<int>(relation.second))+".");
    result.supported=result.limitations.empty();
    return result;
}
} // namespace mbf
