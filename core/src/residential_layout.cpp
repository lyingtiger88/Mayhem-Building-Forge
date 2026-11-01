#include "mbf/residential_layout.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>

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


const char* residential_profile_name(ResidentialLayoutProfile profile) noexcept {
    switch(profile) {
        case ResidentialLayoutProfile::BalancedConventional: return "Balanced Conventional";
        case ResidentialLayoutProfile::OpenLiving: return "Open Living";
        case ResidentialLayoutProfile::PrivacyFirst: return "Privacy First";
    }
    return "Unknown";
}

std::vector<ResidentialVariation> generate_residential_variations(
        const ResidentialLayoutSettings& base,
        const ArchitecturalValidationSettings& validation_settings) {
    struct ProfileSettings {
        ResidentialLayoutProfile profile;
        double public_depth;
        double circulation_depth;
        double living_width;
        double bedroom_width;
    };
    const std::vector<ProfileSettings> profiles{
        {ResidentialLayoutProfile::BalancedConventional,
         base.public_zone_depth_ratio,base.circulation_depth_ratio,
         base.living_width_ratio,base.bedroom_width_ratio},
        {ResidentialLayoutProfile::OpenLiving,0.64,0.17,0.60,0.58},
        {ResidentialLayoutProfile::PrivacyFirst,0.55,0.22,0.48,0.68}
    };

    std::vector<ResidentialVariation> result;
    result.reserve(profiles.size());
    for(const auto& profile:profiles) {
        auto settings=base;
        settings.public_zone_depth_ratio=profile.public_depth;
        settings.circulation_depth_ratio=profile.circulation_depth;
        settings.living_width_ratio=profile.living_width;
        settings.bedroom_width_ratio=profile.bedroom_width;

        // Adapt the private band to the footprint aspect ratio. A fixed private
        // depth makes bedrooms excessively shallow on wide, shallow lots.
        const double minimum_private_depth_ratio =
            std::max(profile.bedroom_width,1.0-profile.bedroom_width) *
            base.width_m / (base.depth_m * validation_settings.maximum_room_aspect_ratio) * 1.03;
        const double current_private_depth_ratio =
            1.0-settings.public_zone_depth_ratio-settings.circulation_depth_ratio;
        if(minimum_private_depth_ratio>current_private_depth_ratio) {
            const double required_private=minimum_private_depth_ratio;
            const double available_for_public=
                1.0-settings.circulation_depth_ratio-required_private;
            if(available_for_public<0.36) continue;
            settings.public_zone_depth_ratio=std::min(
                settings.public_zone_depth_ratio,available_for_public);
        }
        // Derive distinct, repeatable seeds without making geometry depend on randomness.
        settings.seed=base.seed+static_cast<std::uint64_t>(profile.profile)*0x9E3779B97F4A7C15ULL;
        auto layout=generate_balanced_residential_layout(settings);
        layout.profile=profile.profile;
        auto validation=validate_architectural_layout(layout,validation_settings);
        if(validation.valid)
            result.push_back({profile.profile,std::move(layout),std::move(validation)});
    }
    std::stable_sort(result.begin(),result.end(),[](const ResidentialVariation& a,
                                                    const ResidentialVariation& b) {
        if(std::abs(a.validation.quality_score-b.validation.quality_score)>1e-9)
            return a.validation.quality_score>b.validation.quality_score;
        return static_cast<int>(a.profile)<static_cast<int>(b.profile);
    });
    return result;
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
        for(std::size_t j=i+1;j<p.rooms.size();++j) {
            const double shared_area=polygon_intersection_area(
                p.rooms[i].footprint,p.rooms[j].footprint,t);
            if(!std::isfinite(shared_area))
                fail("Cannot evaluate polygon overlap for rooms "+
                     p.rooms[i].name+" and "+p.rooms[j].name+".");
            else if(shared_area>std::max(t*t,p.width_m*p.depth_m*1e-9))
                fail("Rooms "+p.rooms[i].name+" and "+p.rooms[j].name+" overlap.");
        }
    // With every valid simple room polygon inside the rectangular envelope,
    // and pairwise positive-area overlaps rejected, equality of summed area
    // to envelope area establishes coverage up to the configured tolerance.
    if(std::abs(area-p.width_m*p.depth_m)>std::max(1e-5,p.width_m*p.depth_m*1e-7))
        fail("Room footprints do not fully cover the building footprint (gap or excess area).");
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



CirculationValidation2D validate_physical_circulation(
        const ResidentialLayout2D& layout,const CirculationSettings2D& settings) {
    CirculationValidation2D result;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(settings.minimum_corridor_width_m)||settings.minimum_corridor_width_m<=0.0||
       !std::isfinite(settings.minimum_door_clear_width_m)||settings.minimum_door_clear_width_m<=0.0||
       !std::isfinite(settings.door_approach_depth_m)||settings.door_approach_depth_m<=0.0||
       !std::isfinite(settings.tolerance)||settings.tolerance<=0.0) {
        fail("Physical circulation settings must be finite and positive.");
        return result;
    }
    const auto find_room=[&](RoomId id)->const PlannedRoom* {
        const auto it=std::find_if(layout.rooms.begin(),layout.rooms.end(),
            [&](const PlannedRoom& room){return room.id==id;});
        return it==layout.rooms.end()?nullptr:&*it;
    };
    bool has_entrance=false;
    for(const auto& opening:layout.openings) {
        if(opening.kind==OpeningKind::EntranceDoor)has_entrance=true;
        const double width=std::hypot(opening.end.x-opening.start.x,
                                      opening.end.y-opening.start.y);
        if(!std::isfinite(width)||width<settings.minimum_door_clear_width_m-settings.tolerance) {
            fail("Opening "+std::to_string(result.door_approaches.size())+
                 " is narrower than the required clear width.");
            continue;
        }
        const auto* first=find_room(opening.from_room);
        if(!first) {
            fail("Door approach references an unknown source room.");
            continue;
        }
        const auto make_approach=[&](const PlannedRoom& room,std::size_t index) {
            const Point2 mid{(opening.start.x+opening.end.x)*0.5,
                             (opening.start.y+opening.end.y)*0.5};
            const double tx=(opening.end.x-opening.start.x)/width;
            const double ty=(opening.end.y-opening.start.y)/width;
            const Point2 center=polygon_centroid(room.footprint);
            double nx=-ty,ny=tx;
            if((center.x-mid.x)*nx+(center.y-mid.y)*ny<0.0){nx=-nx;ny=-ny;}
            const double d=settings.door_approach_depth_m;
            Polygon2D envelope{{opening.start,opening.end,
                {opening.end.x+nx*d,opening.end.y+ny*d},
                {opening.start.x+nx*d,opening.start.y+ny*d}}};
            bool inside=validate_polygon(envelope,settings.tolerance).valid;
            for(const auto& vertex:envelope.vertices)
                inside=inside&&point_in_polygon(room.footprint,vertex,settings.tolerance);
            result.door_approaches.push_back({room.id,index,std::move(envelope),d,inside});
            if(!inside)
                fail("Door approach clearance extends outside room "+room.name+
                     "; move the opening or increase the available approach space.");
        };
        const auto index=static_cast<std::size_t>(&opening-layout.openings.data());
        make_approach(*first,index);
        if(opening.kind!=OpeningKind::EntranceDoor) {
            const auto* second=find_room(opening.to_room);
            if(!second)fail("Interior opening references an unknown destination room.");
            else make_approach(*second,index);
        }
    }
    if(!has_entrance)fail("No exterior entrance is defined for circulation.");
    for(const auto& room:layout.rooms) {
        if(room.role!=RoomRole::Hallway)continue;
        if(room.footprint.vertices.empty())continue;
        double min_x=std::numeric_limits<double>::max();
        double min_y=min_x,max_x=-min_x,max_y=-min_x;
        for(const auto& vertex:room.footprint.vertices) {
            min_x=std::min(min_x,vertex.x);min_y=std::min(min_y,vertex.y);
            max_x=std::max(max_x,vertex.x);max_y=std::max(max_y,vertex.y);
        }
        const double clear_width=std::min(max_x-min_x,max_y-min_y);
        if(clear_width<settings.minimum_corridor_width_m-settings.tolerance)
            fail("Hallway "+room.name+" is narrower than the configured clear width.");
    }
    result.valid=result.errors.empty();
    return result;
}

LayoutValidation validate_architectural_layout(const ResidentialLayout2D& p,
        const ArchitecturalValidationSettings& s) {
    LayoutValidation result;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(s.minimum_room_area_m2)||s.minimum_room_area_m2<0.0||
       !std::isfinite(s.minimum_room_width_m)||s.minimum_room_width_m<=0.0||
       !std::isfinite(s.minimum_room_depth_m)||s.minimum_room_depth_m<=0.0||
       !std::isfinite(s.maximum_room_aspect_ratio)||s.maximum_room_aspect_ratio<1.0||
       !std::isfinite(s.minimum_door_clear_width_m)||s.minimum_door_clear_width_m<=0.0) {
        fail("Architectural validation settings contain invalid limits.");
        return result;
    }
    const auto base=validate_residential_layout(p,s.tolerance);
    for(const auto& error:base.errors)fail(error);
    if(p.rooms.empty()) {
        result.valid=false;
        result.quality_score=0.0;
        return result;
    }
    std::set<RoomId> ids;
    double score=100.0;
    for(const auto& room:p.rooms) {
        if(!ids.insert(room.id).second) {
            fail("Room IDs must be unique.");
            continue;
        }
        if(!std::isfinite(room.area_m2)||room.area_m2<s.minimum_room_area_m2-s.tolerance)
            fail("Room "+room.name+" is below the configured minimum area.");
        double min_x=std::numeric_limits<double>::max();
        double min_y=min_x,max_x=-min_x,max_y=-min_x;
        for(const auto& vertex:room.footprint.vertices) {
            min_x=std::min(min_x,vertex.x);min_y=std::min(min_y,vertex.y);
            max_x=std::max(max_x,vertex.x);max_y=std::max(max_y,vertex.y);
        }
        const double width=max_x-min_x,depth=max_y-min_y;
        if(width<s.minimum_room_width_m-s.tolerance)
            fail("Room "+room.name+" is narrower than the configured minimum width.");
        if(depth<s.minimum_room_depth_m-s.tolerance)
            fail("Room "+room.name+" is shallower than the configured minimum depth.");
        if(room.role!=RoomRole::Hallway&&width>s.tolerance&&depth>s.tolerance) {
            const double ratio=std::max(width/depth,depth/width);
            if(ratio>s.maximum_room_aspect_ratio+s.tolerance)
                fail("Room "+room.name+" exceeds the configured maximum aspect ratio.");
            else score-=std::max(0.0,ratio-2.0)*1.5;
        }
        if(room.area_m2<s.minimum_room_area_m2*1.15)
            result.warnings.push_back("Room "+room.name+" is close to the minimum area limit.");
    }

    std::vector<RoomId> reachable;
    for(const auto& opening:p.openings) {
        if(!ids.contains(opening.from_room)||!ids.contains(opening.to_room))
            continue;
        if(opening.kind==OpeningKind::EntranceDoor) {
            if(opening.clear_width_m+s.tolerance<s.minimum_door_clear_width_m)
                fail("Entrance door is narrower than the configured minimum clear width.");
            if(std::find(reachable.begin(),reachable.end(),opening.from_room)==reachable.end())
                reachable.push_back(opening.from_room);
        } else if(opening.kind==OpeningKind::InteriorDoor||
                  opening.kind==OpeningKind::OpenPlanConnection) {
            if(opening.clear_width_m+s.tolerance<s.minimum_door_clear_width_m)
                fail("Room access opening is narrower than the configured minimum clear width.");
        }
    }
    for(std::size_t i=0;i<reachable.size();++i) {
        const RoomId current=reachable[i];
        for(const auto& opening:p.openings) {
            if(opening.kind==OpeningKind::EntranceDoor)continue;
            RoomId next=static_cast<RoomId>(-1);
            if(opening.from_room==current)next=opening.to_room;
            else if(opening.to_room==current)next=opening.from_room;
            if(ids.contains(next)&&std::find(reachable.begin(),reachable.end(),next)==reachable.end())
                reachable.push_back(next);
        }
    }
    for(const auto& room:p.rooms)
        if(std::find(reachable.begin(),reachable.end(),room.id)==reachable.end())
            fail("Room "+room.name+" is not reachable from an exterior entrance.");

    result.valid=result.errors.empty();
    result.quality_score=result.valid?std::clamp(score,0.0,100.0):0.0;
    if(result.valid&&result.quality_score<70.0)
        result.warnings.push_back("Layout passed hard constraints but has a relatively low geometric quality score.");
    return result;
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
