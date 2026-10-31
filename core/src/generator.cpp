#include "mbf/generator.hpp"
#include "mbf/floor_plan.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>
namespace mbf {
namespace {
void check(const GenerationSettings&s,const std::vector<RoomSpec>&rs){
 if(!(s.width_m>0)||!(s.depth_m>0)||!std::isfinite(s.width_m)||!std::isfinite(s.depth_m))throw std::invalid_argument("Building dimensions must be finite and positive.");
 if(s.grid_resolution<4U||s.grid_resolution>512U)throw std::invalid_argument("Grid resolution must be 4..512.");
 if(rs.empty())throw std::invalid_argument("At least one room is required.");
 for(const auto&r:rs)if(r.name.empty()||!(r.target_area_m2>0)||!std::isfinite(r.target_area_m2)||r.min_area_m2<0)throw std::invalid_argument("Room name and target area must be valid.");
}
std::vector<double> weights(const std::vector<RoomSpec>&rs){double sum=0;for(const auto&r:rs)sum+=r.target_area_m2;std::vector<double>w;for(const auto&r:rs)w.push_back(r.target_area_m2/sum);return w;}
}
FloorPlan FloorPlanGenerator::generate_grid_growth(const GenerationSettings&s,const std::vector<RoomSpec>&rs)const{
 check(s,rs);const auto cols=s.grid_resolution;const auto rows=std::max<std::uint32_t>(4U,static_cast<std::uint32_t>(std::lround(static_cast<double>(cols)*s.depth_m/s.width_m)));const auto total=static_cast<std::size_t>(cols)*rows;
 const auto idx=[cols](std::uint32_t x,std::uint32_t y){return static_cast<std::size_t>(y)*cols+x;};
 std::vector<RoomId> owner(total,0U);std::vector<bool> used(total,false);std::mt19937_64 rng(s.seed);std::uniform_int_distribution<std::uint32_t>xd(0U,cols-1U),yd(0U,rows-1U);
 FloorPlan p;p.seed=s.seed;p.width_m=s.width_m;p.depth_m=s.depth_m;p.algorithm=FloorPlanAlgorithm::GridGrowth;const auto w=weights(rs);
 for(std::size_t i=0;i<rs.size();++i)p.rooms.push_back({static_cast<RoomId>(i),rs[i].name,rs[i].target_area_m2,0,{},{}});
 std::vector<std::vector<std::size_t>> front(rs.size());std::vector<std::size_t> counts(rs.size(),1U);
 const auto add=[&](std::size_t cell,RoomId id){const auto x=static_cast<std::uint32_t>(cell%cols),y=static_cast<std::uint32_t>(cell/cols);const auto put=[&](std::uint32_t nx,std::uint32_t ny){const auto ni=idx(nx,ny);if(!used[ni]&&std::find(front[id].begin(),front[id].end(),ni)==front[id].end())front[id].push_back(ni);};if(x>0U)put(x-1U,y);if(x+1U<cols)put(x+1U,y);if(y>0U)put(x,y-1U);if(y+1U<rows)put(x,y+1U);};
 for(std::size_t i=0;i<rs.size();++i){std::size_t chosen=0;bool found=false;for(std::size_t n=0;n<total*2U;++n){chosen=idx(xd(rng),yd(rng));if(!used[chosen]){found=true;break;}}if(!found)for(std::size_t n=0;n<total;++n)if(!used[n]){chosen=n;found=true;break;}if(!found)throw std::runtime_error("Could not seed all rooms; increase grid resolution.");used[chosen]=true;owner[chosen]=static_cast<RoomId>(i);add(chosen,static_cast<RoomId>(i));}
 std::size_t assigned=rs.size();
 while(assigned<total){double best=-std::numeric_limits<double>::infinity();std::vector<std::size_t> candidates;for(std::size_t i=0;i<rs.size();++i)if(!front[i].empty()){const auto score=w[i]-static_cast<double>(counts[i])/static_cast<double>(total)+w[i]*0.15;if(score>best+1e-12){best=score;candidates={i};}else if(std::abs(score-best)<=1e-12)candidates.push_back(i);}if(candidates.empty())throw std::runtime_error("Grid growth stalled.");
  std::uniform_int_distribution<std::size_t>pickr(0U,candidates.size()-1U);const auto ri=candidates[pickr(rng)];auto&f=front[ri];std::uniform_int_distribution<std::size_t>pickc(0U,f.size()-1U);const auto cell=f[pickc(rng)];f.erase(std::remove(f.begin(),f.end(),cell),f.end());if(used[cell])continue;used[cell]=true;owner[cell]=static_cast<RoomId>(ri);++counts[ri];++assigned;add(cell,static_cast<RoomId>(ri));
 }
 const auto cell_area=s.width_m*s.depth_m/static_cast<double>(total);
 for(std::uint32_t y=0;y<rows;++y)for(std::uint32_t x=0;x<cols;++x){const auto id=owner[idx(x,y)];GridCell c{x,y,id};p.cells.push_back(c);p.rooms[id].cells.push_back(c);p.rooms[id].actual_area_m2+=cell_area;}
 for(auto&r:p.rooms){auto minx=cols,miny=rows,maxx=0U,maxy=0U;for(const auto&c:r.cells){minx=std::min(minx,c.x);miny=std::min(miny,c.y);maxx=std::max(maxx,c.x);maxy=std::max(maxy,c.y);}r.bounds={static_cast<double>(minx)*s.width_m/cols,static_cast<double>(miny)*s.depth_m/rows,static_cast<double>(maxx-minx+1U)*s.width_m/cols,static_cast<double>(maxy-miny+1U)*s.depth_m/rows};}
 calculate_adjacency(p);return p;
}
FloorPlan FloorPlanGenerator::generate_treemap(const GenerationSettings&s,const std::vector<RoomSpec>&rs)const{
 check(s,rs);const auto w=weights(rs);FloorPlan p;p.seed=s.seed;p.width_m=s.width_m;p.depth_m=s.depth_m;p.algorithm=FloorPlanAlgorithm::SquarifiedTreemap;Rect rem{0,0,s.width_m,s.depth_m};double remw=1.0;std::size_t cursor=0;
 struct Place{std::size_t id;Rect rect;};std::vector<Place> out;
 while(cursor<rs.size()){const bool horizontal=rem.width>=rem.height;const auto shortside=std::min(rem.width,rem.height);std::size_t end=cursor;double strip=0,best=std::numeric_limits<double>::infinity();do{const auto addw=w[end]/remw;strip+=addw;const auto thick=rem.area()*strip/shortside;const auto ratio=std::max(shortside,thick)/std::max(1e-9,std::min(shortside,thick));if(ratio>best&&end>cursor){strip-=addw;break;}best=ratio;++end;}while(end<rs.size());const auto area=rem.area()*strip;
  if(horizontal){const auto h=area/rem.width;double x=rem.x;for(std::size_t i=cursor;i<end;++i){const auto rw=rem.width*(w[i]/remw)/strip;out.push_back({i,{x,rem.y,rw,h}});x+=rw;}rem.y+=h;rem.height-=h;}
  else{const auto width=area/rem.height;double y=rem.y;for(std::size_t i=cursor;i<end;++i){const auto rh=rem.height*(w[i]/remw)/strip;out.push_back({i,{rem.x,y,width,rh}});y+=rh;}rem.x+=width;rem.width-=width;}
  remw*=1.0-strip;cursor=end;
 }
 std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.id<b.id;});for(std::size_t i=0;i<rs.size();++i){const auto&q=out[i];p.rooms.push_back({static_cast<RoomId>(i),rs[i].name,rs[i].target_area_m2,q.rect.area(),q.rect,{}});}
 constexpr double eps=1e-7;for(std::size_t i=0;i<p.rooms.size();++i)for(std::size_t j=i+1;j<p.rooms.size();++j){const auto&a=p.rooms[i].bounds;const auto&b=p.rooms[j].bounds;const auto vo=std::min(a.y+a.height,b.y+b.height)-std::max(a.y,b.y);const auto ho=std::min(a.x+a.width,b.x+b.width)-std::max(a.x,b.x);if((std::abs(a.x+a.width-b.x)<eps||std::abs(b.x+b.width-a.x)<eps)&&vo>eps)p.adjacency.push_back({static_cast<RoomId>(i),static_cast<RoomId>(j),1U});else if((std::abs(a.y+a.height-b.y)<eps||std::abs(b.y+b.height-a.y)<eps)&&ho>eps)p.adjacency.push_back({static_cast<RoomId>(i),static_cast<RoomId>(j),1U});}
 return p;
}
} // namespace mbf
