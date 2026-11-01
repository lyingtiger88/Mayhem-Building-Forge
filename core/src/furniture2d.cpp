#include "mbf/furniture2d.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace mbf {
namespace {
struct Bounds { double x0{},y0{},x1{},y1{}; };
Bounds room_bounds(const RoomGeometry2D& room) {
    Bounds b{};
    if(room.rings.empty()||room.rings.front().vertices.empty())return b;
    const auto& p=room.rings.front().vertices.front();
    b={p.x,p.y,p.x,p.y};
    for(const auto& ring:room.rings)for(const auto& q:ring.vertices){
        b.x0=std::min(b.x0,q.x);b.y0=std::min(b.y0,q.y);
        b.x1=std::max(b.x1,q.x);b.y1=std::max(b.y1,q.y);
    }
    return b;
}
bool inside_ring(const Ring2& ring,const Point2& p) {
    bool inside=false;
    if(ring.vertices.size()<3U)return false;
    for(std::size_t i=0,j=ring.vertices.size()-1;i<ring.vertices.size();j=i++){
        const auto& a=ring.vertices[i];const auto& b=ring.vertices[j];
        const bool crosses=((a.y>p.y)!=(b.y>p.y)) &&
            (p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y+1e-15)+a.x);
        if(crosses)inside=!inside;
    }
    return inside;
}
bool inside_room(const RoomGeometry2D& room,const Point2& p) {
    bool inside=false;
    for(const auto& ring:room.rings)if(inside_ring(ring,p))inside=!inside;
    return inside;
}
bool overlaps(const FurnitureInstance2D& a,const FurnitureInstance2D& b,double gap) {
    const double ag=a.clearance_m+gap*0.5,bg=b.clearance_m+gap*0.5;
    return a.x-ag < b.x+b.width_m+bg && a.x+a.width_m+ag > b.x-bg &&
           a.y-ag < b.y+b.depth_m+bg && a.y+a.depth_m+ag > b.y-bg;
}
bool fits(const RoomGeometry2D& room,const FurnitureInstance2D& item,
          const FurnitureLayout2D& layout,const FurnitureSettings2D& settings) {
    const double c=settings.wall_clearance_m;
    const std::array<Point2,4> corners{{
        {item.x-c,item.y-c},{item.x+item.width_m+c,item.y-c},
        {item.x+item.width_m+c,item.y+item.depth_m+c},{item.x-c,item.y+item.depth_m+c}}};
    for(const auto& p:corners)if(!inside_room(room,p))return false;
    for(const auto& other:layout.items)
        if(other.room_id==item.room_id&&overlaps(item,other,settings.object_clearance_m))return false;
    return true;
}
bool place(const RoomGeometry2D& room,FurnitureLayout2D& layout,
           FurnitureKind2D kind,const char* label,double w,double d,
           const std::vector<std::pair<double,double>>& anchors,
           const FurnitureSettings2D& settings) {
    const auto b=room_bounds(room);
    for(const auto& [fx,fy]:anchors) {
        FurnitureInstance2D item;
        item.id=static_cast<std::uint64_t>(layout.items.size()+1);
        item.room_id=room.id;item.kind=kind;item.label=label;
        item.width_m=w;item.depth_m=d;
        item.x=b.x0+fx*(b.x1-b.x0-w);item.y=b.y0+fy*(b.y1-b.y0-d);
        item.clearance_m=settings.wall_clearance_m;
        if(fits(room,item,layout,settings)){layout.items.push_back(item);return true;}
    }
    return false;
}
std::vector<std::pair<double,double>> corners_and_edges() {
    return {{0.08,0.08},{0.92,0.08},{0.08,0.92},{0.92,0.92},
            {0.5,0.08},{0.5,0.92},{0.08,0.5},{0.92,0.5},{0.5,0.5}};
}
} // namespace

FurnitureLayout2D generate_furniture_layout(const BuildingGeometry2D& geometry,
                                             const FurnitureSettings2D& settings) {
    FurnitureLayout2D result;
    for(const auto& room:geometry.rooms) {
        const auto b=room_bounds(room);const double w=b.x1-b.x0,h=b.y1-b.y0;
        if(w<1.4||h<1.4)continue;
        const auto& name=room.name;
        const auto try_place=[&](FurnitureKind2D k,const char* label,double fw,double fd,
                                 const std::vector<std::pair<double,double>>& a){
            if(!place(room,result,k,label,fw,fd,a,settings)){
                std::ostringstream msg;msg<<room.name<<": could not fit "<<label<<" ("<<fw<<" x "<<fd<<" m).";
                result.diagnostics.push_back(msg.str());
            }
        };
        if(name.find("Bedroom")!=std::string::npos) {
            if(w>=2.35&&h>=2.45){
                try_place(FurnitureKind2D::Bed,"Bed",1.5,2.0,{{0.5,0.08},{0.08,0.5},{0.92,0.5}});
                try_place(FurnitureKind2D::Nightstand,"Nightstand",0.38,0.38,{{0.08,0.08},{0.82,0.08},{0.08,0.82},{0.82,0.82}});
            }
            try_place(FurnitureKind2D::Wardrobe,"Wardrobe",1.2,0.58,{{0.08,0.9},{0.5,0.9},{0.9,0.9},{0.08,0.08}});
        } else if(name.find("Living")!=std::string::npos) {
            try_place(FurnitureKind2D::Sofa,"Sofa",2.1,0.88,{{0.08,0.88},{0.92,0.88},{0.08,0.08},{0.92,0.08}});
            try_place(FurnitureKind2D::CoffeeTable,"Coffee table",1.0,0.58,{{0.5,0.48},{0.5,0.72},{0.5,0.2}});
            try_place(FurnitureKind2D::TvStand,"TV stand",1.5,0.28,{{0.5,0.08},{0.5,0.9},{0.08,0.5},{0.92,0.5}});
            try_place(FurnitureKind2D::AccentChair,"Lounge chair",0.72,0.72,{{0.08,0.48},{0.92,0.48}});
        } else if(name.find("Kitchen")!=std::string::npos) {
            try_place(FurnitureKind2D::Counter,"Counter",std::min(2.4,w-0.4),0.62,{{0.08,0.08},{0.08,0.9},{0.9,0.08}});
            try_place(FurnitureKind2D::Sink,"Sink",0.55,0.42,{{0.08,0.08},{0.25,0.08},{0.08,0.9}});
            try_place(FurnitureKind2D::Cooktop,"Cooktop",0.6,0.52,{{0.52,0.08},{0.08,0.5},{0.52,0.9}});
            try_place(FurnitureKind2D::Refrigerator,"Refrigerator",0.68,0.72,{{0.9,0.08},{0.9,0.9},{0.08,0.5}});
            if(w>=2.8&&h>=2.4) {
                try_place(FurnitureKind2D::DiningTable,"Dining table",1.2,0.8,{{0.5,0.55},{0.5,0.35}});
                try_place(FurnitureKind2D::DiningChair,"Dining chair",0.42,0.42,{{0.35,0.55},{0.65,0.55}});
            }
        } else if(name.find("Bathroom")!=std::string::npos) {
            try_place(FurnitureKind2D::Shower,"Shower",0.9,0.9,{{0.08,0.9},{0.9,0.9},{0.08,0.08}});
            try_place(FurnitureKind2D::Toilet,"Toilet",0.42,0.68,{{0.9,0.08},{0.5,0.08},{0.9,0.5}});
            try_place(FurnitureKind2D::Vanity,"Vanity",0.72,0.48,{{0.08,0.08},{0.5,0.9},{0.5,0.08}});
        } else if(name.find("Hallway")!=std::string::npos) {
            try_place(FurnitureKind2D::Console,"Console",0.34,1.0,{{0.08,0.5},{0.9,0.5},{0.5,0.08}});
        }
    }
    result.valid=validate_furniture_layout(geometry,result,1e-6,nullptr);
    return result;
}

bool validate_furniture_layout(const BuildingGeometry2D& geometry,
                               const FurnitureLayout2D& layout,double tolerance,
                               std::vector<std::string>* diagnostics) {
    bool valid=true;
    const auto report=[&](const std::string& message){valid=false;if(diagnostics)diagnostics->push_back(message);};
    for(std::size_t i=0;i<layout.items.size();++i) {
        const auto& item=layout.items[i];
        const auto room=std::find_if(geometry.rooms.begin(),geometry.rooms.end(),
            [&](const RoomGeometry2D& r){return r.id==item.room_id;});
        if(room==geometry.rooms.end()){report("Furniture references an unknown room.");continue;}
        if(item.width_m<=0.0||item.depth_m<=0.0){report("Furniture has non-positive dimensions.");continue;}
        const std::array<Point2,4> corners{{
            {item.x-tolerance,item.y-tolerance},
            {item.x+item.width_m+tolerance,item.y-tolerance},
            {item.x+item.width_m+tolerance,item.y+item.depth_m+tolerance},
            {item.x-tolerance,item.y+item.depth_m+tolerance}}};
        for(const auto& p:corners)if(!inside_room(*room,p)){report("Furniture footprint leaves its room boundary.");break;}
        for(std::size_t j=0;j<i;++j)if(layout.items[j].room_id==item.room_id&&
            overlaps(item,layout.items[j],0.0)){report("Furniture footprints overlap or violate clearance.");break;}
    }
    return valid;
}
} // namespace mbf
