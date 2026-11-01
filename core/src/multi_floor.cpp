#include "mbf/multi_floor.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace mbf {
namespace {
const PlannedRoom* find_room(const ResidentialLayout2D& layout,RoomId id) {
    const auto it=std::find_if(layout.rooms.begin(),layout.rooms.end(),
        [&](const PlannedRoom& room){return room.id==id;});
    return it==layout.rooms.end()?nullptr:&*it;
}

} // namespace

MultiFloorBuilding generate_stacked_building(const ResidentialLayout2D& typical,
        std::uint32_t floor_count,double floor_height,double slab_thickness) {
    if(floor_count==0U)throw std::invalid_argument("A building must contain at least one floor.");
    if(!std::isfinite(floor_height)||floor_height<=0.0||
       !std::isfinite(slab_thickness)||slab_thickness<=0.0||slab_thickness>=floor_height)
        throw std::invalid_argument("Floor height and slab thickness must be finite, positive, and slab thinner than floor height.");
    MultiFloorBuilding building;
    building.floors.reserve(floor_count);
    for(std::uint32_t i=0;i<floor_count;++i)
        building.floors.push_back({i,static_cast<double>(i)*floor_height,
                                   floor_height,slab_thickness,typical});
    return building;
}

MultiFloorValidation validate_multi_floor_building(const MultiFloorBuilding& building,
        const MultiFloorValidationSettings& settings) {
    MultiFloorValidation result;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(settings.minimum_floor_to_floor_height_m)||
       !std::isfinite(settings.maximum_floor_to_floor_height_m)||
       settings.minimum_floor_to_floor_height_m<=0.0||
       settings.maximum_floor_to_floor_height_m<settings.minimum_floor_to_floor_height_m||
       !std::isfinite(settings.maximum_slab_thickness_m)||settings.maximum_slab_thickness_m<=0.0||
       !std::isfinite(settings.alignment_tolerance_m)||settings.alignment_tolerance_m<=0.0||
       !std::isfinite(settings.geometry_tolerance)||settings.geometry_tolerance<=0.0) {
        fail("Multi-floor validation settings are invalid.");
        return result;
    }
    if(building.floors.empty()) {
        fail("Building must contain at least one floor.");
        return result;
    }
    const auto& first=building.floors.front().layout;
    if(!validate_residential_layout(first,settings.geometry_tolerance).valid)
        fail("Reference floor has invalid 2D geometry.");
    const double width=first.width_m,depth=first.depth_m;
    for(std::size_t i=0;i<building.floors.size();++i) {
        const auto& floor=building.floors[i];
        const std::string prefix="Floor "+std::to_string(floor.floor_index)+": ";
        if(floor.floor_index!=i)fail(prefix+"floor indices must be consecutive and start at zero.");
        if(!std::isfinite(floor.elevation_m)||!std::isfinite(floor.floor_to_floor_height_m)||
           !std::isfinite(floor.slab_thickness_m)) {
            fail(prefix+"elevation and construction dimensions must be finite.");continue;
        }
        if(floor.floor_to_floor_height_m<settings.minimum_floor_to_floor_height_m||
           floor.floor_to_floor_height_m>settings.maximum_floor_to_floor_height_m)
            fail(prefix+"floor-to-floor height is outside configured limits.");
        if(floor.slab_thickness_m<=0.0||
           floor.slab_thickness_m>settings.maximum_slab_thickness_m||
           floor.slab_thickness_m>=floor.floor_to_floor_height_m)
            fail(prefix+"slab thickness is invalid.");
        const double expected_elevation=static_cast<double>(i)*
            (i==0?floor.floor_to_floor_height_m:building.floors[0].floor_to_floor_height_m);
        if(std::abs(floor.elevation_m-expected_elevation)>settings.alignment_tolerance_m)
            fail(prefix+"floor elevation is inconsistent with the vertical stack.");
        if(std::abs(floor.layout.width_m-width)>settings.alignment_tolerance_m||
           std::abs(floor.layout.depth_m-depth)>settings.alignment_tolerance_m)
            fail(prefix+"floor plate dimensions do not align with the building envelope.");
        const auto layout_validation=validate_residential_layout(floor.layout,settings.geometry_tolerance);
        if(!layout_validation.valid)fail(prefix+"2D floor plan is invalid.");
    }
    result.total_height_m=building.floors.back().elevation_m+
                          building.floors.back().floor_to_floor_height_m;
    std::set<std::uint64_t> core_ids;
    for(const auto& core:building.vertical_cores) {
        if(!core_ids.insert(core.id).second)fail("Vertical core IDs must be unique.");
        const auto geometry=validate_polygon(core.footprint,settings.geometry_tolerance);
        if(!geometry.valid) {
            fail("Vertical core "+core.name+" has invalid footprint geometry.");continue;
        }
        if(core.room_ids_by_floor.size()!=building.floors.size()) {
            fail("Vertical core "+core.name+" must map to exactly one room on every floor.");continue;
        }
        for(std::size_t i=0;i<building.floors.size();++i) {
            const auto* room=find_room(building.floors[i].layout,core.room_ids_by_floor[i]);
            if(!room) {
                fail("Vertical core "+core.name+" references a missing room on floor "+
                     std::to_string(i)+".");continue;
            }
            const double overlap=polygon_intersection_area(core.footprint,room->footprint,
                                                            settings.geometry_tolerance);
            const double core_area=polygon_area(core.footprint);
            if(!std::isfinite(overlap)||std::abs(overlap-core_area)>
               std::max(1e-6,core_area*1e-7))
                fail("Vertical core "+core.name+" is not fully contained in its mapped room on floor "+
                     std::to_string(i)+".");
        }
    }
    for(std::size_t i=0;i<building.vertical_cores.size();++i)
        for(std::size_t j=i+1;j<building.vertical_cores.size();++j) {
            const double overlap=polygon_intersection_area(building.vertical_cores[i].footprint,
                building.vertical_cores[j].footprint,settings.geometry_tolerance);
            if(!std::isfinite(overlap)||overlap>settings.geometry_tolerance)
                fail("Vertical cores "+building.vertical_cores[i].name+" and "+
                     building.vertical_cores[j].name+" overlap.");
        }
    result.valid=result.errors.empty();
    return result;
}
} // namespace mbf
