#include "mbf/floor_plan.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <utility>
namespace mbf {
namespace { using EdgeKey=std::pair<RoomId,RoomId>; EdgeKey ordered_pair(RoomId a,RoomId b){return a<b?EdgeKey{a,b}:EdgeKey{b,a};} }
void calculate_adjacency(FloorPlan& p){
 std::map<EdgeKey,std::uint32_t> shared;
 if(!p.cells.empty()){
  std::uint32_t mx=0,my=0;for(const auto&c:p.cells){mx=std::max(mx,c.x);my=std::max(my,c.y);}
  const auto cols=static_cast<std::size_t>(mx)+1U,rows=static_cast<std::size_t>(my)+1U;
  std::vector<RoomId> lookup(cols*rows,static_cast<RoomId>(-1));
  for(const auto&c:p.cells)lookup[static_cast<std::size_t>(c.y)*cols+c.x]=c.room_id;
  for(std::uint32_t y=0;y<=my;++y)for(std::uint32_t x=0;x<=mx;++x){
   const auto id=lookup[static_cast<std::size_t>(y)*cols+x];if(id==static_cast<RoomId>(-1))continue;
   if(x<mx){const auto n=lookup[static_cast<std::size_t>(y)*cols+x+1U];if(n!=id&&n!=static_cast<RoomId>(-1))++shared[ordered_pair(id,n)];}
   if(y<my){const auto n=lookup[static_cast<std::size_t>(y+1U)*cols+x];if(n!=id&&n!=static_cast<RoomId>(-1))++shared[ordered_pair(id,n)];}
  }
 }
 p.adjacency.clear();for(const auto&[edge,count]:shared)p.adjacency.push_back({edge.first,edge.second,count});
}
ValidationResult validate_floor_plan(const FloorPlan&p){
 ValidationResult r;const auto fail=[&r](const std::string&s){r.valid=false;r.errors.push_back(s);};
 if(!(p.width_m>0)||!(p.depth_m>0))fail("Floor-plan dimensions must be positive.");
 if(p.rooms.empty())fail("A floor plan must contain at least one room.");
 std::set<RoomId> ids;
 for(const auto&room:p.rooms){if(!ids.insert(room.id).second)fail("Room IDs must be unique.");if(p.algorithm==FloorPlanAlgorithm::GridGrowth&&room.cells.empty())fail("Every grid room must own cells.");if(!(room.actual_area_m2>0))fail("Every room must have positive area.");}
 for(const auto&c:p.cells)if(!ids.contains(c.room_id))fail("A cell references an unknown room ID.");
 for(const auto&e:p.adjacency)if(e.first==e.second||!ids.contains(e.first)||!ids.contains(e.second)||e.shared_edges==0U)fail("Adjacency must connect two distinct existing rooms.");
 if(p.algorithm==FloorPlanAlgorithm::GridGrowth&&p.cells.empty())fail("Grid floor plan has no cells.");
 return r;
}
} // namespace mbf
