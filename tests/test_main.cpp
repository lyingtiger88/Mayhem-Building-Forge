#include "mbf/floor_plan.hpp"
#include "mbf/generator.hpp"
#include "mbf/geometry2d.hpp"
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
 req(p.circulation_links.size()>=4U,"hallway and open-plan access links are emitted");
 const auto adjacent=[&](mbf::RoomId a,mbf::RoomId b){
  for(const auto&e:p.adjacency)if((e.first==a&&e.second==b)||(e.first==b&&e.second==a))return true;
  return false;
 };
 req(adjacent(0,1),"living directly adjoins kitchen");
 req(adjacent(0,4),"living directly adjoins hallway");
 req(adjacent(4,2),"hallway directly adjoins bedroom");
 req(adjacent(4,3),"hallway directly adjoins bathroom");
 const auto hall=std::find_if(p.rooms.begin(),p.rooms.end(),[](const auto&r){return r.role==mbf::RoomRole::Hallway;});
 req(hall!=p.rooms.end()&&std::abs(hall->bounds.y)<1e-7,"hallway touches south entrance facade");
 const auto geometry=mbf::build_geometry_2d(p);
 req(geometry.valid,"topology-first plan converts to 2D geometry");
 req(geometry.geometry.entrance.defined,"canonical geometry preserves entrance");
}
void geometry2d_grid(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,64,20261031},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"grid converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all grid rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(!r.rings.empty(),"grid room has polygon rings");req(r.area_m2>0,"grid polygon has positive area");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"grid polygon area conserves footprint");}
void geometry2d_treemap(){mbf::FloorPlanGenerator g;const auto p=g.generate_treemap({18,12,48,99},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"treemap converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all treemap rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(r.rings.size()==1U,"treemap room is one rectangle ring");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"treemap polygon area conserves footprint");}
void svg_export(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,32,77},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"SVG fixture geometry valid");const auto path=std::filesystem::temp_directory_path()/"mayhem_building_forge_test.svg";std::string error;req(mbf::export_geometry_svg(result.geometry,path.string(),&error),"SVG export succeeds");req(std::filesystem::exists(path),"SVG file exists");req(std::filesystem::file_size(path)>100U,"SVG file is not empty");std::filesystem::remove(path);}
void invalid(){mbf::FloorPlanGenerator g;bool threw=false;try{(void)g.generate_grid_growth({0,10,32,1},sample());}catch(const std::invalid_argument&){threw=true;}req(threw,"invalid dimensions rejected");}
}
int main(){grid();treemap();topology_first();geometry2d_grid();geometry2d_treemap();svg_export();invalid();std::cout<<"All Mayhem Building Forge tests passed.\n";return EXIT_SUCCESS;}
