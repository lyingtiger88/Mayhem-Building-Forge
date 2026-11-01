#include "mbf/floor_plan.hpp"
#include "mbf/generator.hpp"
#include "mbf/geometry2d.hpp"
#include "mbf/polygon.hpp"
#include "mbf/polygon_region.hpp"
#include "mbf/polygon_offset.hpp"
#include "mbf/residential_layout.hpp"
#include "mbf/multi_floor.hpp"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void req(bool b,const std::string&m){if(!b){std::cerr<<"FAILED: "<<m<<'\n';std::exit(EXIT_FAILURE);}}
std::vector<mbf::RoomSpec> sample(){return {{"Living",28,mbf::RoomRole::Living},{"Kitchen",14,mbf::RoomRole::Kitchen},{"Bedroom",18,mbf::RoomRole::Bedroom},{"Bathroom",7,mbf::RoomRole::Bathroom},{"Hallway",9,mbf::RoomRole::Hallway}};}
void grid(){mbf::FloorPlanGenerator g;mbf::GenerationSettings s{14,10,48,123456};const auto a=g.generate_grid_growth(s,sample()),b=g.generate_grid_growth(s,sample());req(a.cells==b.cells,"fixed seed is deterministic");req(a.cells.size()==48U*34U,"grid covers footprint");req(a.rooms.size()==5U,"all rooms emitted");req(mbf::validate_floor_plan(a).valid,"grid validates");for(const auto&r:a.rooms)req(!r.cells.empty()&&r.actual_area_m2>0,"grid room has cells and area");}
void treemap(){
 mbf::FloorPlanGenerator g;mbf::GenerationSettings s{18,12,48,99};
 const auto p=g.generate_treemap(s,sample());double area=0;
 req(p.rooms.size()==5U,"all rooms emitted");
 for(const auto&r:p.rooms){
  area+=r.actual_area_m2;
  req(r.bounds.x>=-1e-8&&r.bounds.y>=-1e-8,"room starts inside footprint");
  req(r.bounds.x+r.bounds.width<=s.width_m+1e-7&&r.bounds.y+r.bounds.height<=s.depth_m+1e-7,"room ends inside footprint");
  const double ratio=std::max(r.bounds.width/r.bounds.height,r.bounds.height/r.bounds.width);
  req(ratio<4.0,"room rectangle aspect ratio is bounded");
 }
 for(std::size_t i=0;i<p.rooms.size();++i)for(std::size_t j=i+1;j<p.rooms.size();++j){
  const auto&a=p.rooms[i].bounds;const auto&b=p.rooms[j].bounds;
  const double ox=std::min(a.x+a.width,b.x+b.width)-std::max(a.x,b.x);
  const double oy=std::min(a.y+a.height,b.y+b.height)-std::max(a.y,b.y);
  req(ox<=1e-7||oy<=1e-7,"room rectangles do not overlap");
 }
 req(std::abs(area-s.width_m*s.depth_m)<1e-6,"treemap covers footprint");
 req(mbf::validate_floor_plan(p).valid,"treemap validates");
 req(!p.adjacency.empty(),"treemap adjacency exists");
 const auto adjacent=[&](const std::string&a,const std::string&b){
  mbf::RoomId ia=999999U,ib=999999U;
  for(const auto&r:p.rooms){if(r.name==a)ia=r.id;if(r.name==b)ib=r.id;}
  for(const auto&edge:p.adjacency)if((edge.first==ia&&edge.second==ib)||(edge.first==ib&&edge.second==ia))return true;
  return false;
 };
 req(adjacent("Living","Kitchen"),"living is adjacent to kitchen");
 req(adjacent("Living","Hallway"),"living is adjacent to hallway");
 req(adjacent("Hallway","Bedroom"),"hallway is adjacent to bedroom");
 req(adjacent("Hallway","Bathroom"),"hallway is adjacent to bathroom");
 req(!adjacent("Living","Bathroom"),"bathroom is not directly adjacent to living");
 const auto bedroom=std::find_if(p.rooms.begin(),p.rooms.end(),[](const auto&r){return r.name=="Bedroom";});
 req(bedroom!=p.rooms.end(),"bedroom exists");
 const auto&br=bedroom->bounds;
 const double edge_gap=std::min({br.x,br.y,s.width_m-(br.x+br.width),s.depth_m-(br.y+br.height)});
 req(edge_gap<1e-6,"bedroom is placed on the building perimeter");

}

void topology_first(){
 mbf::FloorPlanGenerator g;const mbf::GenerationSettings s{18,12,64,20261031};
 const auto p=g.generate_residential_layout(s,sample(),mbf::EntranceSide::South);
 req(p.algorithm==mbf::FloorPlanAlgorithm::ResidentialTopologyFirst,"residential topology algorithm tagged");
 req(p.entrance.defined,"south entrance is defined");
 req(p.entrance.side==mbf::EntranceSide::South,"entrance side is south");
 req(p.entrance.room_id==4U,"entrance opens into hallway");
 req(p.entrance.clear_width_m>=0.9&&p.entrance.clear_width_m<=1.0,"entrance clear width is valid");
 req(std::abs(p.entrance.start_y)<1e-7&&std::abs(p.entrance.end_y)<1e-7,"entrance lies on south facade");
 req(p.circulation_links.size()>=5U,"hallway and open-plan access links are emitted");
 req(mbf::validate_floor_plan(p).valid,"entrance and circulation metadata validate");
 const auto adjacent=[&](mbf::RoomId a,mbf::RoomId b){
  for(const auto&e:p.adjacency)if((e.first==a&&e.second==b)||(e.first==b&&e.second==a))return true;
  return false;
 };
 req(adjacent(0,1),"living directly adjoins kitchen");
 req(adjacent(0,4),"living directly adjoins hallway");
 req(adjacent(4,1),"hallway directly adjoins kitchen");
 req(adjacent(4,2),"hallway directly adjoins bedroom");
 req(adjacent(4,3),"hallway directly adjoins bathroom");
 const auto hall=std::find_if(p.rooms.begin(),p.rooms.end(),[](const auto&r){return r.role==mbf::RoomRole::Hallway;});
 req(hall!=p.rooms.end()&&std::abs(hall->bounds.y)<1e-7,"hallway touches south entrance facade");
 const auto geometry=mbf::build_geometry_2d(p);
 req(geometry.valid,"topology-first plan converts to 2D geometry");
 req(geometry.geometry.entrance.defined,"canonical geometry preserves entrance");
}



void polygon_offsets_and_setbacks(){
 using namespace mbf;
 const auto lot=make_rectangle(0,0,10,8);
 const auto inset=offset_polygon(lot,-1.0);
 req(inset.valid,"rectangle inward offset succeeds");
 req(std::abs(polygon_area(inset.polygon)-48.0)<1e-7,
     "one-metre rectangle inset has analytically correct area");
 for(const auto& v:inset.polygon.vertices)
  req(v.x>=1.0-1e-8&&v.x<=9.0+1e-8&&v.y>=1.0-1e-8&&v.y<=7.0+1e-8,
      "inset vertices respect the setback distance");

 const auto expanded=offset_polygon(lot,1.0);
 req(expanded.valid,"rectangle outward offset succeeds");
 req(std::abs(polygon_area(expanded.polygon)-120.0)<1e-7,
     "one-metre rectangle expansion has analytically correct area");

 Polygon2D clockwise=lot;
 std::reverse(clockwise.vertices.begin(),clockwise.vertices.end());
 const auto cw_inset=offset_polygon(clockwise,-1.0);
 req(cw_inset.valid&&std::abs(polygon_area(cw_inset.polygon)-48.0)<1e-7,
     "offset supports clockwise polygon winding");

 const auto buildable=compute_buildable_area(lot,1.0);
 req(buildable.valid,"uniform site setback creates a buildable area");
 req(std::abs(buildable.lot_area_m2-80.0)<1e-8&&
     std::abs(buildable.buildable_area_m2-48.0)<1e-7,
     "buildable-area report includes correct lot and net areas");

 const auto no_setback=compute_buildable_area(lot,0.0);
 req(no_setback.valid&&std::abs(no_setback.buildable_area_m2-80.0)<1e-8,
     "zero setback preserves the original lot boundary");
 req(!compute_buildable_area(lot,-0.5).valid,
     "negative site setback is rejected");

 const auto collapsed=offset_polygon(make_rectangle(0,0,2,2),-1.1);
 req(!collapsed.valid,"inward offset beyond polygon collapse is rejected");
 const auto invalid_miter=offset_polygon(lot,-0.5,0.5);
 req(!invalid_miter.valid,"miter limit below one is rejected");

 const Polygon2D l_shape{{{0,0},{6,0},{6,2},{2,2},{2,6},{0,6}}};
 const auto l_inset=offset_polygon(l_shape,-0.25);
 req(l_inset.valid,"moderate inward offset of a concave L-shaped polygon succeeds");
 req(polygon_area(l_inset.polygon)<polygon_area(l_shape),
     "concave inset reduces usable area");
}
void non_rectangular_regions(){
 using namespace mbf;
 const PolygonRegion2D l_shape{Polygon2D{{{0,0},{6,0},{6,2},{2,2},{2,6},{0,6}}},{}};
 const std::vector<Polygon2D> l_parts{
  make_rectangle(0,0,6,2),make_rectangle(0,2,2,4)};
 const auto l_valid=validate_polygon_partition(l_shape,l_parts);
 req(l_valid.valid,"L-shaped envelope accepts a complete room partition");
 req(std::abs(l_valid.region_area_m2-20.0)<1e-8,"L-shaped envelope area is correct");
 auto l_gap=l_parts;l_gap[1]=make_rectangle(0,2,1.8,4);
 req(!validate_polygon_partition(l_shape,l_gap).valid,"L-shaped envelope detects uncovered notch area");
 auto l_overlap=l_parts;l_overlap[1]=make_rectangle(1.5,2,2,4);
 req(!validate_polygon_partition(l_shape,l_overlap).valid,"L-shaped envelope rejects overlapping room polygons");

 const PolygonRegion2D u_shape{Polygon2D{{{0,0},{6,0},{6,6},{4,6},{4,2},{2,2},{2,6},{0,6}}},{}};
 const std::vector<Polygon2D> u_parts{
  make_rectangle(0,0,6,2),make_rectangle(0,2,2,4),make_rectangle(4,2,2,4)};
 req(validate_polygon_partition(u_shape,u_parts).valid,
     "U-shaped envelope accepts three non-overlapping room regions");

 const PolygonRegion2D courtyard{
  make_rectangle(0,0,10,10),{make_rectangle(3,3,4,4)}};
 const std::vector<Polygon2D> courtyard_parts{
  make_rectangle(0,0,10,3),make_rectangle(0,7,10,3),
  make_rectangle(0,3,3,4),make_rectangle(7,3,3,4)};
 const auto courtyard_valid=validate_polygon_partition(courtyard,courtyard_parts);
 req(courtyard_valid.valid,"courtyard envelope accepts rooms surrounding an interior void");
 req(std::abs(courtyard_valid.region_area_m2-84.0)<1e-8,
     "courtyard area is excluded from usable building region");
 auto courtyard_intrusion=courtyard_parts;
 courtyard_intrusion[2]=make_rectangle(2.5,3,3.5,4);
 req(!validate_polygon_partition(courtyard,courtyard_intrusion).valid,
     "room geometry intruding into courtyard is rejected");

 PolygonRegion2D invalid_hole{make_rectangle(0,0,5,5),{make_rectangle(4,4,2,2)}};
 req(!validate_polygon_partition(invalid_hole,{make_rectangle(0,0,5,5)}).valid,
     "hole outside the outer boundary is rejected");
 PolygonRegion2D touching_hole{make_rectangle(0,0,5,5),{make_rectangle(0,1,1,1)}};
 req(!validate_polygon_partition(touching_hole,{make_rectangle(0,0,5,5)}).valid,
     "hole touching the outer boundary is rejected");
}
void polygon_engine(){
 using namespace mbf;
 const auto rect=make_rectangle(0,0,4,3);
 req(validate_polygon(rect).valid,"rectangle polygon validates");
 req(std::abs(polygon_area(rect)-12.0)<1e-9,"rectangle area is correct");
 const auto tri=make_regular_polygon({0,0},2,3);
 const auto pent=make_regular_polygon({0,0},2,5);
 const auto hex=make_regular_polygon({0,0},2,6);
 req(validate_polygon(tri).valid&&validate_polygon(pent).valid&&validate_polygon(hex).valid,"regular triangle pentagon hexagon validate");
 req(polygon_area(hex)>polygon_area(pent)&&polygon_area(pent)>polygon_area(tri),"regular polygon areas increase for same circumradius");
 const Polygon2D concave{{{0,0},{4,0},{4,1},{1,1},{1,4},{0,4}}};
 req(validate_polygon(concave).valid,"concave L polygon validates");
 req(std::abs(polygon_area(concave)-7.0)<1e-9,"concave L area is correct");
 req(point_in_polygon(concave,{0.5,3.0}),"point inside concave polygon detected");
 req(!point_in_polygon(concave,{3.0,3.0}),"point in concave notch rejected");
 const auto left=make_rectangle(0,0,3,2),right=make_rectangle(3,0,2,2);
 req(std::abs(shared_boundary_length(left,right)-2.0)<1e-9,"shared boundary length detected");
 const Polygon2D bowtie{{{0,0},{2,2},{0,2},{2,0}}};
 req(!validate_polygon(bowtie).valid,"self-intersecting polygon rejected");
 const auto center=polygon_centroid(rect);
 req(std::abs(center.x-2.0)<1e-9&&std::abs(center.y-1.5)<1e-9,"rectangle centroid is correct");
}


void robust_polygon_geometry(){
 using namespace mbf;
 const Polygon2D concave{{{0,0},{4,0},{4,1},{1,1},{1,4},{0,4}}};
 const auto notch=make_rectangle(1,1,3,3);
 req(std::abs(polygon_intersection_area(concave,notch))<1e-8,
     "concave bounding boxes may overlap without polygon-area overlap");
 const auto crossing=make_rectangle(0.5,0.5,2,2);
 req(std::abs(polygon_intersection_area(concave,crossing)-1.75)<1e-7,
     "concave overlap computes actual nonzero area");
 const auto containment=make_rectangle(0.1,0.1,0.4,0.4);
 req(std::abs(polygon_intersection_area(concave,containment)-0.16)<1e-7,
     "fully contained polygon intersection is correct");

 ResidentialLayout2D tile;tile.width_m=4;tile.depth_m=4;
 auto make_room=[](RoomId id,const char* name,Polygon2D p){
  const double area=polygon_area(p);
  return PlannedRoom{id,name,RoomRole::Auto,std::move(p),area};
 };
 tile.rooms.push_back(make_room(0,"L",concave));
 tile.rooms.push_back(make_room(1,"NotchLeft",make_rectangle(1,1,1.5,3)));
 tile.rooms.push_back(make_room(2,"NotchRight",make_rectangle(2.5,1,1.5,3)));
 req(validate_residential_layout(tile).valid,
     "concave room and two rectangles tile footprint without overlapping");
 auto overlap=tile;
 overlap.rooms[1].footprint=make_rectangle(0.9,1,1.6,3);
 overlap.rooms[1].area_m2=polygon_area(overlap.rooms[1].footprint);
 req(!validate_residential_layout(overlap).valid,
     "real positive-area room overlap is rejected");
 auto gap=tile;
 gap.rooms[1].footprint=make_rectangle(1.1,1,1.4,3);
 gap.rooms[1].area_m2=polygon_area(gap.rooms[1].footprint);
 req(!validate_residential_layout(gap).valid,
     "uncovered gap within footprint is rejected");

 // Deterministic rectangle regression sweep; boundaries and containment
 // must agree with exact analytical intersection for many coordinates.
 for(int i=0;i<100;++i) {
  const double x=static_cast<double>((i*17)%29)*0.1-0.6;
  const double y=static_cast<double>((i*11)%31)*0.1-0.6;
  const double w=0.3+static_cast<double>((i*7)%9)*0.14;
  const double h=0.4+static_cast<double>((i*13)%7)*0.17;
  const auto a=make_rectangle(0,0,2,2),b=make_rectangle(x,y,w,h);
  const double expected=std::max(0.0,std::min(2.0,x+w)-std::max(0.0,x))*
                        std::max(0.0,std::min(2.0,y+h)-std::max(0.0,y));
  req(std::abs(polygon_intersection_area(a,b)-expected)<1e-7,
      "polygon intersection agrees with analytical rectangle overlap");
 }
}

void multi_floor_building(){
 using namespace mbf;
 ResidentialLayoutSettings settings;settings.width_m=12;settings.depth_m=9;
 const auto typical=generate_balanced_residential_layout(settings);
 auto building=generate_stacked_building(typical,20,3.2,0.22);
 const auto valid=validate_multi_floor_building(building);
 req(valid.valid,"twenty-floor stack validates");
 req(building.floors.size()==20U,"requested floor count is generated");
 req(std::abs(building.floors[19].elevation_m-60.8)<1e-8,
     "floor elevations follow floor-to-floor height");
 req(std::abs(valid.total_height_m-64.0)<1e-8,
     "total building height is calculated");
 req(building.floors[0].layout.rooms.size()==typical.rooms.size(),
     "each floor retains the canonical 2D room model");

 VerticalCore2D stair;
 stair.id=100;stair.name="Main Stair";stair.kind=VerticalCoreKind::Stair;
 stair.footprint=make_rectangle(0.5,5.7,1.0,1.0);
 stair.room_ids_by_floor.assign(building.floors.size(),2U);
 VerticalCore2D elevator;
 elevator.id=101;elevator.name="Elevator";elevator.kind=VerticalCoreKind::Elevator;
 elevator.footprint=make_rectangle(2.0,5.7,1.0,1.0);
 elevator.room_ids_by_floor.assign(building.floors.size(),2U);
 building.vertical_cores={stair,elevator};
 req(validate_multi_floor_building(building).valid,
     "stair and elevator cores remain continuous and aligned across all floors");

 auto missing_core_room=building;
 missing_core_room.vertical_cores[0].room_ids_by_floor[7]=999U;
 req(!validate_multi_floor_building(missing_core_room).valid,
     "missing room mapping breaks vertical core continuity");

 auto misaligned=building;
 misaligned.floors[8].elevation_m+=0.15;
 req(!validate_multi_floor_building(misaligned).valid,
     "misaligned floor elevation is rejected");

 auto wrong_plate=building;
 wrong_plate.floors[4].layout.width_m+=0.5;
 req(!validate_multi_floor_building(wrong_plate).valid,
     "misaligned floor plate dimensions are rejected");

 auto thick_slab=building;
 thick_slab.floors[2].slab_thickness_m=0.8;
 req(!validate_multi_floor_building(thick_slab).valid,
     "excessive slab thickness is rejected");

 auto overlapping_cores=building;
 overlapping_cores.vertical_cores[1].footprint=make_rectangle(0.8,5.8,1.0,0.8);
 req(!validate_multi_floor_building(overlapping_cores).valid,
     "overlapping stair and elevator core footprints are rejected");

 bool zero_floors_threw=false;
 try{(void)generate_stacked_building(typical,0);}catch(const std::invalid_argument&){zero_floors_threw=true;}
 req(zero_floors_threw,"zero-floor building generation is rejected");
}
void residential_layout(){
 using namespace mbf;
 ResidentialLayoutSettings s;s.width_m=12;s.depth_m=9;s.seed=20261031;
 const auto p=generate_balanced_residential_layout(s);
 const auto v=validate_residential_layout(p);
 req(v.valid,"balanced residential layout validates");
 req(p.rooms.size()==5U,"residential plan has five functional zones");
 req(p.openings.size()==6U,"residential plan has entrance, room doors and open-plan connection");
 double area=0;
 for(const auto&r:p.rooms){req(validate_polygon(r.footprint).valid,"every room is a valid polygon");area+=r.area_m2;}
 req(std::abs(area-108.0)<1e-7,"residential room polygons cover the whole footprint");
 req(p.rooms[0].role==RoomRole::Living&&p.rooms[1].role==RoomRole::Kitchen,"public rooms occupy south zone");
 req(p.rooms[2].role==RoomRole::Hallway,"circulation spine is explicit");
 req(p.rooms[3].role==RoomRole::Bedroom&&p.rooms[4].role==RoomRole::Bathroom,"private and service rooms occupy north zone");
 req(p.openings[0].kind==OpeningKind::EntranceDoor,"south entrance is represented as an opening");
 req(p.openings[1].kind==OpeningKind::OpenPlanConnection,"living and kitchen have open-plan connection");
 const auto path=std::filesystem::temp_directory_path()/"mayhem_house_layout_test.svg";
 std::string error;
 req(export_residential_layout_svg(p,path.string(),&error),"residential SVG export succeeds");
 req(std::filesystem::exists(path)&&std::filesystem::file_size(path)>200U,"residential SVG exists and is non-empty");
 std::filesystem::remove(path);
 bool rejected=false;
 try{ResidentialLayoutSettings bad;bad.width_m=5;(void)generate_balanced_residential_layout(bad);}
 catch(const std::invalid_argument&){rejected=true;}
 req(rejected,"too-small house footprint is rejected");
}



void residential_variations(){
 using namespace mbf;
 ResidentialLayoutSettings settings;
 settings.width_m=12.0;settings.depth_m=9.0;settings.seed=20261031;
 const auto variations=generate_residential_variations(settings);

 req(variations.size()==3U,"all three residential profiles produce valid candidates");
 std::vector<ResidentialLayoutProfile> profiles;
 for(const auto&candidate:variations) {
  profiles.push_back(candidate.profile);
  req(candidate.validation.valid,"every returned variation passes architectural validation");
  req(candidate.validation.quality_score>0.0,"every valid variation has a positive quality score");
  req(validate_residential_layout(candidate.layout).valid,"every variation passes geometric validation");
  req(candidate.layout.profile==candidate.profile,"candidate and layout profile metadata agree");
  req(candidate.layout.seed!=settings.seed||candidate.profile==ResidentialLayoutProfile::BalancedConventional,
      "non-baseline profiles receive reproducible derived seeds");
 }
 std::sort(profiles.begin(),profiles.end(),[](auto a,auto b){return static_cast<int>(a)<static_cast<int>(b);});
 req(profiles[0]==ResidentialLayoutProfile::BalancedConventional&&
     profiles[1]==ResidentialLayoutProfile::OpenLiving&&
     profiles[2]==ResidentialLayoutProfile::PrivacyFirst,
     "variation set contains the three named design profiles");
 const auto&balanced=variations[0].layout;
 bool geometry_differs=false;
 for(const auto&candidate:variations) {
  if(candidate.profile==ResidentialLayoutProfile::BalancedConventional)continue;
  for(std::size_t i=0;i<balanced.rooms.size()&&i<candidate.layout.rooms.size();++i)
   if(std::abs(balanced.rooms[i].area_m2-candidate.layout.rooms[i].area_m2)>1e-6)
    geometry_differs=true;
 }
 req(geometry_differs,"profiles change actual room geometry and areas");
 const auto repeat=generate_residential_variations(settings);
 req(repeat.size()==variations.size(),"variation generation is deterministic in candidate count");
 for(std::size_t i=0;i<repeat.size();++i) {
  req(repeat[i].profile==variations[i].profile,"candidate ranking is deterministic");
  req(std::abs(repeat[i].validation.quality_score-variations[i].validation.quality_score)<1e-9,
      "candidate scores are deterministic");
 }
 req(std::string(residential_profile_name(ResidentialLayoutProfile::OpenLiving))=="Open Living",
     "profile names are human-readable");
}

void physical_circulation_clearances(){
 using namespace mbf;
 ResidentialLayoutSettings layout_settings;
 layout_settings.width_m=12.0;layout_settings.depth_m=9.0;
 const auto plan=generate_balanced_residential_layout(layout_settings);
 const auto valid=validate_physical_circulation(plan);
 req(valid.valid,"baseline residential plan passes physical circulation checks");
 req(valid.door_approaches.size()==11U,
     "entrance and internal doors produce approach envelopes on each served side");
 for(const auto& approach:valid.door_approaches) {
  req(approach.valid,"door approach envelope fits within its room");
  req(std::abs(polygon_area(approach.envelope)-
       plan.openings[approach.opening_index].clear_width_m*approach.depth_m)<1e-6,
       "door approach envelope has expected clear area");
 }
 CirculationSettings2D narrow_corridor;
 narrow_corridor.minimum_corridor_width_m=2.0;
 req(!validate_physical_circulation(plan,narrow_corridor).valid,
     "hallway narrower than required clear width is rejected");
 CirculationSettings2D excessive_approach;
 excessive_approach.door_approach_depth_m=20.0;
 req(!validate_physical_circulation(plan,excessive_approach).valid,
     "door approach extending outside the room is rejected");
 auto no_entrance=plan;
 no_entrance.openings.erase(std::remove_if(no_entrance.openings.begin(),no_entrance.openings.end(),
  [](const PlanOpening& opening){return opening.kind==OpeningKind::EntranceDoor;}),
  no_entrance.openings.end());
 req(!validate_physical_circulation(no_entrance).valid,
     "circulation without an exterior entrance is rejected");
 CirculationSettings2D invalid;
 invalid.door_approach_depth_m=0.0;
 req(!validate_physical_circulation(plan,invalid).valid,
     "invalid physical clearance settings are rejected");
}
void architectural_validation(){
 using namespace mbf;
 ResidentialLayoutSettings settings;settings.width_m=12;settings.depth_m=9;settings.seed=77;
 auto plan=generate_balanced_residential_layout(settings);
 const auto valid=validate_architectural_layout(plan);
 req(valid.valid,"balanced residential layout passes architectural constraints");
 req(valid.quality_score>0.0&&valid.quality_score<=100.0,"valid layout receives bounded quality score");

 ArchitecturalValidationSettings strict;
 strict.minimum_room_area_m2=20.0;
 const auto undersized=validate_architectural_layout(plan,strict);
 req(!undersized.valid&&undersized.quality_score==0.0,"undersized rooms fail configured minimum area");

 auto disconnected=plan;
 disconnected.openings.erase(std::remove_if(disconnected.openings.begin(),disconnected.openings.end(),
  [](const PlanOpening&o){return o.from_room==2U&&o.to_room==4U;}),disconnected.openings.end());
 const auto access=validate_architectural_layout(disconnected);
 req(!access.valid,"room disconnected from entrance fails circulation validation");
 req(std::any_of(access.errors.begin(),access.errors.end(),
  [](const std::string&e){return e.find("not reachable")!=std::string::npos;}),
  "inaccessible room diagnostic identifies reachability failure");

 auto narrow=plan;
 narrow.openings.front().clear_width_m=0.5;
 const auto width=validate_architectural_layout(narrow);
 req(!width.valid,"narrow entrance fails clear-width constraint");

 ArchitecturalValidationSettings invalid_settings;
 invalid_settings.maximum_room_aspect_ratio=0.5;
 req(!validate_architectural_layout(plan,invalid_settings).valid,
     "invalid architectural constraints are rejected");
}
void boundary_classification(){
 using namespace mbf;
 FloorPlanGenerator generator;
 const auto plan=generator.generate_treemap({18,12,48,99},sample());
 const auto built=build_geometry_2d(plan);
 req(built.valid,"boundary fixture geometry is valid");
 req(built.geometry.coordinates.units=="metres","canonical coordinates use metres");
 req(built.geometry.coordinates.origin=="south-west","canonical origin is south-west");
 req(built.geometry.coordinates.positive_x=="east"&&built.geometry.coordinates.positive_y=="north","canonical axes point east and north");
 req(!built.geometry.boundaries.empty(),"room boundaries are extracted");
 double exterior_length=0.0,shared_length=0.0;
 std::size_t exterior_count=0,shared_count=0;
 for(const auto&edge:built.geometry.boundaries) {
  req(edge.id>0U,"boundary has stable nonzero id");
  req(edge.length_m>0.0,"boundary has positive length");
  const double measured=std::hypot(edge.end.x-edge.start.x,edge.end.y-edge.start.y);
  req(std::abs(measured-edge.length_m)<1e-8,"boundary length matches endpoints");
  if(edge.kind==BoundaryKind2D::ExteriorWall) {
   exterior_length+=edge.length_m;++exterior_count;
  } else {
   shared_length+=edge.length_m;++shared_count;
   req(edge.room_a!=edge.room_b,"shared wall joins distinct rooms");
  }
 }
 req(exterior_count>=4U,"exterior boundary segments are classified");
 req(std::abs(exterior_length-60.0)<1e-5,"exterior wall length equals building perimeter");
 req(shared_count>0U&&shared_length>0.0,"shared room walls are classified");
}

void edge_normalization(){
 using namespace mbf;
 BuildingGeometry2D g;g.width_m=4.0;g.depth_m=2.0;
 // The right room deliberately stores its shared left edge as two segments.
 g.rooms.push_back({0,"Left",4.0,{{{{0,0},{2,0},{2,2},{0,2}}}}});
 g.rooms.push_back({1,"Right",4.0,{{{{2,0},{4,0},{4,2},{2,2},{2,1}}}}});
 const auto edges=classify_boundaries(g);
 double exterior=0.0,shared=0.0;std::size_t shared_count=0;
 for(const auto&e:edges){
  req(e.length_m>1e-8,"normalized edge has positive length");
  req(std::abs(std::hypot(e.end.x-e.start.x,e.end.y-e.start.y)-e.length_m)<1e-8,
      "normalized edge length matches endpoints");
  if(e.kind==BoundaryKind2D::ExteriorWall)exterior+=e.length_m;
  else {shared+=e.length_m;++shared_count;
        req(e.room_a==0U&&e.room_b==1U,"shared wall ownership is canonical");}
 }
 req(std::abs(exterior-12.0)<1e-7,"outer perimeter is emitted once");
 req(std::abs(shared-2.0)<1e-7,"partial collinear shared edges are fully matched");
 req(shared_count==1U,"split shared edge is merged into one wall segment");
 req(edges.size()==7U,"normalized two-room rectangle has four outer sides plus shared wall subdivisions only where required");
}

void wall_topology_graph(){
 using namespace mbf;
 BuildingGeometry2D g;g.width_m=4.0;g.depth_m=2.0;
 g.rooms.push_back({0,"Left",4.0,{{{{0,0},{2,0},{2,2},{0,2}}}}});
 g.rooms.push_back({1,"Right",4.0,{{{{2,0},{4,0},{4,2},{2,2}}}}});
 const auto graph=build_wall_topology(g);
 req(graph.valid,"wall topology graph validates");
 req(graph.walls.size()==7U,"two-room partition creates seven normalized wall segments");
 req(graph.junctions.size()==6U,"wall graph has six unique junction nodes");
 std::size_t tees=0,corners=0,endpoints=0;
 for(const auto&node:graph.junctions){
  req(node.id>0U&&!node.connected_wall_ids.empty(),"junction has ID and connected walls");
  if(node.kind==WallJunctionKind2D::Tee)++tees;
  if(node.kind==WallJunctionKind2D::Corner)++corners;
  if(node.kind==WallJunctionKind2D::Endpoint)++endpoints;
 }
 req(tees==2U,"shared wall endpoints are classified as T junctions");
 req(corners==4U,"outer building corners are classified as corners");
 req(endpoints==0U,"closed two-room perimeter has no dangling wall endpoints");
 const auto repeated=build_wall_topology(g);
 req(repeated.walls.size()==graph.walls.size()&&repeated.junctions.size()==graph.junctions.size(),
     "wall topology generation is deterministic");
 for(std::size_t i=0;i<graph.walls.size();++i)
  req(graph.walls[i].id==repeated.walls[i].id,"wall IDs are deterministic for identical geometry");
}

void wall_opening_integration(){
 using namespace mbf;
 BuildingGeometry2D g;g.width_m=4.0;g.depth_m=2.0;
 g.rooms.push_back({0,"Left",4.0,{{{{0,0},{2,0},{2,2},{0,2}}}}});
 g.rooms.push_back({1,"Right",4.0,{{{{2,0},{4,0},{4,2},{2,2}}}}});
 auto graph=build_wall_topology(g);
 req(graph.valid,"opening fixture wall topology is valid");
 const auto shared=std::find_if(graph.walls.begin(),graph.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::SharedWall;});
 const auto exterior=std::find_if(graph.walls.begin(),graph.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::ExteriorWall&&std::abs(w.start.y-w.end.y)<1e-8;});
 req(shared!=graph.walls.end()&&exterior!=graph.walls.end(),"shared and exterior walls exist");
 std::string error;
 req(add_wall_opening(graph,shared->id,OpeningKind2D::InteriorDoor,0.55,0.9,{},&error),
     "interior door accepted on shared wall");
 req(graph.openings.size()==1U,"one opening stored");
 req(!add_wall_opening(graph,shared->id,OpeningKind2D::Window,0.2,0.8,{},&error),
     "window rejected on shared wall");
 req(!add_wall_opening(graph,shared->id,OpeningKind2D::InteriorDoor,0.7,0.8,{},&error),
     "overlapping opening rejected");
 req(!add_wall_opening(graph,shared->id,OpeningKind2D::InteriorDoor,0.0,0.8,{},&error),
     "opening too close to wall endpoint rejected");
 req(add_wall_opening(graph,exterior->id,OpeningKind2D::Window,0.4,1.2,{},&error),
     "window accepted on exterior wall");
 std::size_t solid=0,gaps=0;
 for(const auto&piece:graph.wall_pieces) {
  req(piece.length_m>1e-8,"wall piece has positive length");
  if(piece.opening_gap)++gaps;else ++solid;
 }
 req(gaps==2U,"each opening creates a represented opening gap");
 req(solid>=4U,"wall solids remain on either side of openings");
 const auto window=std::find_if(graph.openings.begin(),graph.openings.end(),
  [](const WallOpening2D&o){return o.kind==OpeningKind2D::Window;});
 req(window!=graph.openings.end()&&window->height_m>0.0&&window->sill_height_m>0.0,
     "window stores height and sill elevation");
}

void circulation_access_graph(){
 using namespace mbf;
 BuildingGeometry2D g;g.width_m=4.0;g.depth_m=2.0;
 g.rooms.push_back({0,"Left",4.0,{{{{0,0},{2,0},{2,2},{0,2}}}}});
 g.rooms.push_back({1,"Right",4.0,{{{{2,0},{4,0},{4,2},{2,2}}}}});
 auto graph=build_wall_topology(g);
 const auto exterior=std::find_if(graph.walls.begin(),graph.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::ExteriorWall;});
 const auto shared=std::find_if(graph.walls.begin(),graph.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::SharedWall;});
 req(exterior!=graph.walls.end()&&shared!=graph.walls.end(),"circulation fixture has exterior and shared walls");
 std::string error;
 req(add_wall_opening(graph,exterior->id,OpeningKind2D::EntranceDoor,0.5,1.0,{},&error),
     "exterior entrance door accepted");
 req(add_wall_opening(graph,shared->id,OpeningKind2D::InteriorDoor,0.5,0.9,{},&error),
     "interior access door accepted");
 const auto access=build_circulation_graph(graph,{0,1},0.8);
 req(access.valid,"all rooms are reachable from entrance");
 req(access.reachable_room_ids.size()==2U&&access.inaccessible_room_ids.empty(),
     "entrance and interior door connect both rooms");
 req(access.links.size()==2U,"access graph contains entrance and room-to-room links");
 req(std::count_if(access.links.begin(),access.links.end(),[](const RoomAccessLink2D&l){return l.is_entrance;})==1,
     "access graph identifies one exterior entrance");

 auto disconnected=build_wall_topology(g);
 const auto ext2=std::find_if(disconnected.walls.begin(),disconnected.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::ExteriorWall;});
 req(add_wall_opening(disconnected,ext2->id,OpeningKind2D::EntranceDoor,0.5,1.0,{},&error),
     "entrance-only fixture accepted");
 const auto disconnected_result=build_circulation_graph(disconnected,{0,1},0.8);
 req(!disconnected_result.valid&&disconnected_result.inaccessible_room_ids.size()==1U,
     "room without a door connection is reported inaccessible");

 auto narrow=build_wall_topology(g);
 const auto ext3=std::find_if(narrow.walls.begin(),narrow.walls.end(),
  [](const BoundarySegment2D&w){return w.kind==BoundaryKind2D::ExteriorWall;});
 req(add_wall_opening(narrow,ext3->id,OpeningKind2D::EntranceDoor,0.5,0.6,{},&error),
     "narrow entrance can be represented before access validation");
 const auto narrow_result=build_circulation_graph(narrow,{0,1},0.8);
 req(!narrow_result.valid,"entrance below minimum clear width fails access validation");
 req(!add_wall_opening(narrow,ext3->id,OpeningKind2D::InteriorDoor,0.5,0.9,{},&error),
     "interior door cannot be placed on exterior wall");
}
void geometry2d_grid(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,64,20261031},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"grid converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all grid rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(!r.rings.empty(),"grid room has polygon rings");req(r.area_m2>0,"grid polygon has positive area");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"grid polygon area conserves footprint");}
void geometry2d_treemap(){mbf::FloorPlanGenerator g;const auto p=g.generate_treemap({18,12,48,99},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"treemap converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all treemap rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(r.rings.size()==1U,"treemap room is one rectangle ring");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"treemap polygon area conserves footprint");}
void svg_export(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,32,77},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"SVG fixture geometry valid");const auto path=std::filesystem::temp_directory_path()/"mayhem_building_forge_test.svg";std::string error;req(mbf::export_geometry_svg(result.geometry,path.string(),&error),"SVG export succeeds");req(std::filesystem::exists(path),"SVG file exists");req(std::filesystem::file_size(path)>100U,"SVG file is not empty");std::filesystem::remove(path);}
void invalid(){mbf::FloorPlanGenerator g;bool threw=false;try{(void)g.generate_grid_growth({0,10,32,1},sample());}catch(const std::invalid_argument&){threw=true;}req(threw,"invalid dimensions rejected");}
}
int main(){grid();treemap();topology_first();polygon_engine();polygon_offsets_and_setbacks();non_rectangular_regions();robust_polygon_geometry();residential_layout();multi_floor_building();residential_variations();physical_circulation_clearances();architectural_validation();boundary_classification();edge_normalization();wall_topology_graph();wall_opening_integration();circulation_access_graph();geometry2d_grid();geometry2d_treemap();svg_export();invalid();std::cout<<"All Mayhem Building Forge tests passed.\n";return EXIT_SUCCESS;}
