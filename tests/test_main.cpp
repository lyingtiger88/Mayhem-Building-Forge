#include "mbf/floor_plan.hpp"
#include "mbf/generator.hpp"
#include "mbf/geometry2d.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void req(bool b,const std::string&m){if(!b){std::cerr<<"FAILED: "<<m<<'\n';std::exit(EXIT_FAILURE);}}
std::vector<mbf::RoomSpec> sample(){return {{"Living",28},{"Kitchen",14},{"Bedroom",18},{"Bathroom",7},{"Hallway",9}};}
void grid(){mbf::FloorPlanGenerator g;mbf::GenerationSettings s{14,10,48,123456};const auto a=g.generate_grid_growth(s,sample()),b=g.generate_grid_growth(s,sample());req(a.cells==b.cells,"fixed seed is deterministic");req(a.cells.size()==48U*34U,"grid covers footprint");req(a.rooms.size()==5U,"all rooms emitted");req(mbf::validate_floor_plan(a).valid,"grid validates");for(const auto&r:a.rooms)req(!r.cells.empty()&&r.actual_area_m2>0,"grid room has cells and area");}
void treemap(){mbf::FloorPlanGenerator g;mbf::GenerationSettings s{18,12,48,99};const auto p=g.generate_treemap(s,sample());double area=0;for(const auto&r:p.rooms)area+=r.actual_area_m2;req(std::abs(area-s.width_m*s.depth_m)<1e-6,"treemap covers footprint");req(p.rooms.size()==5U,"all rooms emitted");req(mbf::validate_floor_plan(p).valid,"treemap validates");req(!p.adjacency.empty(),"treemap adjacency exists");}
void geometry2d_grid(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,64,20261031},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"grid converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all grid rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(!r.rings.empty(),"grid room has polygon rings");req(r.area_m2>0,"grid polygon has positive area");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"grid polygon area conserves footprint");}
void geometry2d_treemap(){mbf::FloorPlanGenerator g;const auto p=g.generate_treemap({18,12,48,99},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"treemap converts to valid 2D geometry");req(result.geometry.rooms.size()==p.rooms.size(),"all treemap rooms converted");double area=0;for(const auto&r:result.geometry.rooms){req(r.rings.size()==1U,"treemap room is one rectangle ring");area+=r.area_m2;}req(std::abs(area-216.0)<1e-5,"treemap polygon area conserves footprint");}
void svg_export(){mbf::FloorPlanGenerator g;const auto p=g.generate_grid_growth({18,12,32,77},sample());const auto result=mbf::build_geometry_2d(p);req(result.valid,"SVG fixture geometry valid");const auto path=std::filesystem::temp_directory_path()/"mayhem_building_forge_test.svg";std::string error;req(mbf::export_geometry_svg(result.geometry,path.string(),&error),"SVG export succeeds");req(std::filesystem::exists(path),"SVG file exists");req(std::filesystem::file_size(path)>100U,"SVG file is not empty");std::filesystem::remove(path);}
void invalid(){mbf::FloorPlanGenerator g;bool threw=false;try{(void)g.generate_grid_growth({0,10,32,1},sample());}catch(const std::invalid_argument&){threw=true;}req(threw,"invalid dimensions rejected");}
}
int main(){grid();treemap();geometry2d_grid();geometry2d_treemap();svg_export();invalid();std::cout<<"All Mayhem Building Forge tests passed.\n";return EXIT_SUCCESS;}
