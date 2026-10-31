#include "mbf/residential_layout.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>

namespace mbf {
namespace {
constexpr double eps = 1e-8;

PlannedRoom room(RoomId id, const char* name, RoomRole role,
                 double x, double y, double w, double h) {
    auto polygon = make_rectangle(x,y,w,h);
    return {id,name,role,polygon,polygon_area(polygon)};
}
void add_opening(ResidentialLayout2D& p, RoomId a, RoomId b, OpeningKind kind,
                 Point2 start, Point2 end) {
    p.openings.push_back({a,b,kind,start,end,std::hypot(end.x-start.x,end.y-start.y)});
}
bool inside_bounds(const Polygon2D& p,double w,double h,double t) {
    for(const auto&v:p.vertices)
        if(v.x < -t || v.y < -t || v.x > w+t || v.y > h+t) return false;
    return true;
}
bool rects_overlap_area(const Polygon2D& a,const Polygon2D& b,double t) {
    double aminx=std::numeric_limits<double>::max(),aminy=aminx,amaxx=-aminx,amaxy=-aminx;
    double bminx=aminx,bminy=aminx,bmaxx=-aminx,bmaxy=-aminx;
    for(const auto&p:a.vertices){aminx=std::min(aminx,p.x);aminy=std::min(aminy,p.y);amaxx=std::max(amaxx,p.x);amaxy=std::max(amaxy,p.y);}
    for(const auto&p:b.vertices){bminx=std::min(bminx,p.x);bminy=std::min(bminy,p.y);bmaxx=std::max(bmaxx,p.x);bmaxy=std::max(bmaxy,p.y);}
    return std::min(amaxx,bmaxx)-std::max(aminx,bminx)>t &&
           std::min(amaxy,bmaxy)-std::max(aminy,bminy)>t;
}
bool segment_on_boundary(const PlanOpening& o,const Polygon2D& a,const Polygon2D* b,double t) {
    const auto on_edge=[&](const Polygon2D& p) {
        for(std::size_t i=0;i<p.vertices.size();++i) {
            const auto&u=p.vertices[i];const auto&v=p.vertices[(i+1)%p.vertices.size()];
            const double dx=v.x-u.x,dy=v.y-u.y,len=std::hypot(dx,dy);
            if(len<=t)continue;
            const auto cross=[&](Point2 q){return dx*(q.y-u.y)-dy*(q.x-u.x);};
            if(std::abs(cross(o.start))<=t*len && std::abs(cross(o.end))<=t*len) {
                const double p0=((o.start.x-u.x)*dx+(o.start.y-u.y)*dy)/len;
                const double p1=((o.end.x-u.x)*dx+(o.end.y-u.y)*dy)/len;
                if(std::min(p0,p1)>=-t && std::max(p0,p1)<=len+t)return true;
            }
        }
        return false;
    };
    return on_edge(a) && (!b || on_edge(*b));
}
}
ResidentialLayout2D generate_balanced_residential_layout(const ResidentialLayoutSettings& s) {
    if(!std::isfinite(s.width_m)||!std::isfinite(s.depth_m)||s.width_m<8.0||s.depth_m<7.0)
        throw std::invalid_argument("Residential footprint must be finite and at least 8 m wide by 7 m deep.");
    if(s.public_zone_depth_ratio<=0.35||s.circulation_depth_ratio<=0.10||
       s.public_zone_depth_ratio+s.circulation_depth_ratio>=0.90||
       s.living_width_ratio<=0.35||s.living_width_ratio>=0.70||
       s.bedroom_width_ratio<=0.45||s.bedroom_width_ratio>=0.80)
        throw std::invalid_argument("Residential zoning ratios are outside supported ranges.");
    if(s.entrance_width_m<0.85||s.internal_door_width_m<0.75||s.open_plan_width_m<1.5)
        throw std::invalid_argument("Door and open-plan widths are below supported minimums.");

    ResidentialLayout2D p;
    p.width_m=s.width_m;p.depth_m=s.depth_m;p.seed=s.seed;
    const double public_y=s.depth_m*s.public_zone_depth_ratio;
    const double private_y=public_y+s.depth_m*s.circulation_depth_ratio;
    const double living_x=s.width_m*s.living_width_ratio;
    const double bedroom_x=s.width_m*s.bedroom_width_ratio;

    // South/public band: living and kitchen share a broad open-plan boundary.
    p.rooms.push_back(room(0,"Living",RoomRole::Living,0,0,living_x,public_y));
    p.rooms.push_back(room(1,"Kitchen",RoomRole::Kitchen,living_x,0,s.width_m-living_x,public_y));
    // Middle band is a real, continuous circulation spine, not a leftover gap.
    p.rooms.push_back(room(2,"Hallway",RoomRole::Hallway,0,public_y,s.width_m,private_y-public_y));
    // North/private band: bedroom and bathroom have independent hallway access.
    p.rooms.push_back(room(3,"Bedroom",RoomRole::Bedroom,0,private_y,bedroom_x,s.depth_m-private_y));
    p.rooms.push_back(room(4,"Bathroom",RoomRole::Bathroom,bedroom_x,private_y,s.width_m-bedroom_x,s.depth_m-private_y));

    const double entrance_x=living_x*0.50;
    add_opening(p,0,0,OpeningKind::EntranceDoor,
        {entrance_x-s.entrance_width_m/2.0,0},{entrance_x+s.entrance_width_m/2.0,0});
    const double open_x=living_x;
    const double open_center=public_y*0.52;
    add_opening(p,0,1,OpeningKind::OpenPlanConnection,
        {open_x,open_center-s.open_plan_width_m/2.0},
        {open_x,open_center+s.open_plan_width_m/2.0});
    const double living_door_x=living_x*0.72;
    add_opening(p,0,2,OpeningKind::InteriorDoor,
        {living_door_x-s.internal_door_width_m/2.0,public_y},
        {living_door_x+s.internal_door_width_m/2.0,public_y});
    const double kitchen_door_x=living_x+(s.width_m-living_x)*0.5;
    add_opening(p,1,2,OpeningKind::InteriorDoor,
        {kitchen_door_x-s.internal_door_width_m/2.0,public_y},
        {kitchen_door_x+s.internal_door_width_m/2.0,public_y});
    const double bedroom_door_x=bedroom_x*0.72;
    add_opening(p,2,3,OpeningKind::InteriorDoor,
        {bedroom_door_x-s.internal_door_width_m/2.0,private_y},
        {bedroom_door_x+s.internal_door_width_m/2.0,private_y});
    const double bath_door_x=bedroom_x+(s.width_m-bedroom_x)*0.50;
    add_opening(p,2,4,OpeningKind::InteriorDoor,
        {bath_door_x-s.internal_door_width_m/2.0,private_y},
        {bath_door_x+s.internal_door_width_m/2.0,private_y});
    return p;
}

LayoutValidation validate_residential_layout(const ResidentialLayout2D& p,double t) {
    LayoutValidation r;
    const auto fail=[&](const std::string& m){r.errors.push_back(m);};
    if(!std::isfinite(p.width_m)||!std::isfinite(p.depth_m)||p.width_m<=0||p.depth_m<=0)
        fail("Building footprint dimensions must be finite and positive.");
    if(p.rooms.size()<3)fail("Residential layout must contain at least three rooms.");
    double area=0;
    for(const auto&room:p.rooms) {
        const auto v=validate_polygon(room.footprint,t);
        if(!v.valid)fail("Room "+room.name+" has invalid polygon geometry.");
        if(!inside_bounds(room.footprint,p.width_m,p.depth_m,t))
            fail("Room "+room.name+" extends outside the building footprint.");
        area+=polygon_area(room.footprint);
    }
    for(std::size_t i=0;i<p.rooms.size();++i)
        for(std::size_t j=i+1;j<p.rooms.size();++j)
            if(rects_overlap_area(p.rooms[i].footprint,p.rooms[j].footprint,t))
                fail("Rooms "+p.rooms[i].name+" and "+p.rooms[j].name+" overlap.");
    if(std::abs(area-p.width_m*p.depth_m)>std::max(1e-5,p.width_m*p.depth_m*1e-7))
        fail("Room footprints do not fully cover the building footprint.");
    for(const auto&o:p.openings) {
        if(o.from_room>=p.rooms.size()||o.to_room>=p.rooms.size())
            {fail("Opening references an unknown room.");continue;}
        const Polygon2D* second=o.kind==OpeningKind::EntranceDoor?nullptr:&p.rooms[o.to_room].footprint;
        if(!segment_on_boundary(o,p.rooms[o.from_room].footprint,second,t))
            fail("Opening is not located on its host room boundary.");
        const double length=std::hypot(o.end.x-o.start.x,o.end.y-o.start.y);
        if(length<=t||std::abs(length-o.clear_width_m)>t)
            fail("Opening has invalid clear width.");
    }
    r.valid=r.errors.empty();return r;
}

bool export_residential_layout_svg(const ResidentialLayout2D& p,const std::string& path,std::string* error) {
    const auto validation=validate_residential_layout(p);
    if(!validation.valid){if(error)*error=validation.errors.front();return false;}
    std::ofstream f(path);
    if(!f){if(error)*error="Could not open residential SVG output.";return false;}
    constexpr double scale=85.0,margin=48.0;
    const double width=p.width_m*scale+margin*2,height=p.depth_m*scale+margin*2+24;
    const char* colors[]={"#dbeafe","#dcfce7","#ede9fe","#fef3c7","#fce7f3"};
    f<<std::fixed<<std::setprecision(3);
    f<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<width<<"\" height=\""<<height
     <<"\" viewBox=\"0 0 "<<width<<" "<<height<<"\">\n<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    f<<"<g transform=\"translate("<<margin<<","<<margin+p.depth_m*scale<<") scale("<<scale<<",-"<<scale<<")\">\n";
    f<<"<rect x=\"0\" y=\"0\" width=\""<<p.width_m<<"\" height=\""<<p.depth_m
     <<"\" fill=\"none\" stroke=\"#0f172a\" stroke-width=\"0.055\"/>\n";
    for(std::size_t i=0;i<p.rooms.size();++i) {
        f<<"<polygon points=\"";
        for(const auto&v:p.rooms[i].footprint.vertices)f<<v.x<<","<<v.y<<" ";
        f<<"\" fill=\""<<colors[i%5]<<"\" stroke=\"#334155\" stroke-width=\"0.035\"/>\n";
    }
    for(const auto&o:p.openings) {
        const char* color=o.kind==OpeningKind::EntranceDoor?"#16a34a":
                           o.kind==OpeningKind::OpenPlanConnection?"#2563eb":"#d97706";
        f<<"<line x1=\""<<o.start.x<<"\" y1=\""<<o.start.y<<"\" x2=\""<<o.end.x<<"\" y2=\""<<o.end.y
         <<"\" stroke=\"white\" stroke-width=\"0.11\"/>\n";
        f<<"<line x1=\""<<o.start.x<<"\" y1=\""<<o.start.y<<"\" x2=\""<<o.end.x<<"\" y2=\""<<o.end.y
         <<"\" stroke=\""<<color<<"\" stroke-width=\"0.045\"/>\n";
    }
    f<<"</g>\n";
    for(const auto&room:p.rooms) {
        const auto c=polygon_centroid(room.footprint);
        f<<"<text x=\""<<margin+c.x*scale<<"\" y=\""<<margin+(p.depth_m-c.y)*scale
         <<"\" text-anchor=\"middle\" dominant-baseline=\"middle\" font-family=\"Arial\" font-size=\"14\" fill=\"#0f172a\">"
         <<room.name<<"</text>\n";
        f<<"<text x=\""<<margin+c.x*scale<<"\" y=\""<<margin+(p.depth_m-c.y)*scale+17
         <<"\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"10\" fill=\"#475569\">"
         <<std::setprecision(1)<<room.area_m2<<" m2</text>\n"<<std::setprecision(3);
    }
    f<<"<text x=\""<<margin<<"\" y=\""<<height-10<<"\" font-family=\"Arial\" font-size=\"11\" fill=\"#475569\">"
     <<p.width_m<<" m x "<<p.depth_m<<" m | SOUTH / ENTRY</text>\n</svg>\n";
    if(!f.good()){if(error)*error="Failed writing residential SVG.";return false;}return true;
}
} // namespace mbf
