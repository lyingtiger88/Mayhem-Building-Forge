#include "mbf/geometry2d.hpp"
#include "mbf/furniture2d.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace mbf {
namespace {
using LatticePoint = std::pair<std::uint32_t, std::uint32_t>;
struct Edge { LatticePoint from, to; bool used{false}; };

int direction(const Edge& e) {
    if (e.to.first > e.from.first) return 0;
    if (e.to.second > e.from.second) return 1;
    if (e.to.first < e.from.first) return 2;
    return 3;
}

int turn_priority(int incoming, int outgoing) {
    const int turn = (outgoing - incoming + 4) % 4;
    // At a vertex shared only diagonally, the left-most continuation keeps
    // edge-connected cell components as separate, deterministic rings.
    switch (turn) {
        case 1: return 0;
        case 0: return 1;
        case 3: return 2;
        default: return 3;
    }
}

double distance(const Point2& a, const Point2& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

std::string room_color(RoomId id) {
    static const char* colors[] = {
        "#dceafe", "#dcfce7", "#fef3c7", "#fce7f3", "#ede9fe",
        "#cffafe", "#ffedd5", "#e2e8f0", "#f3e8ff", "#ecfccb"
    };
    return colors[id % (sizeof(colors) / sizeof(colors[0]))];
}

std::vector<Ring2> extract_grid_rings(const FloorPlan& plan, RoomId room_id,
                                      std::uint32_t cols, std::uint32_t rows,
                                      const std::vector<RoomId>& owners) {
    const auto index = [cols](std::uint32_t x, std::uint32_t y) {
        return static_cast<std::size_t>(y) * cols + x;
    };
    const auto belongs = [&](std::uint32_t x, std::uint32_t y) {
        return owners[index(x, y)] == room_id;
    };

    std::vector<Edge> edges;
    for (const auto& cell : plan.cells) {
        if (cell.room_id != room_id) continue;
        const auto x = cell.x, y = cell.y;
        if (y == 0U || !belongs(x, y - 1U)) edges.push_back({{x,y},{x+1U,y},false});
        if (x + 1U == cols || !belongs(x + 1U, y)) edges.push_back({{x+1U,y},{x+1U,y+1U},false});
        if (y + 1U == rows || !belongs(x, y + 1U)) edges.push_back({{x+1U,y+1U},{x,y+1U},false});
        if (x == 0U || !belongs(x - 1U, y)) edges.push_back({{x,y+1U},{x,y},false});
    }

    std::map<LatticePoint, std::vector<std::size_t>> outgoing;
    for (std::size_t i = 0; i < edges.size(); ++i) outgoing[edges[i].from].push_back(i);

    const double cell_w = plan.width_m / static_cast<double>(cols);
    const double cell_h = plan.depth_m / static_cast<double>(rows);
    std::vector<Ring2> rings;

    for (std::size_t start = 0; start < edges.size(); ++start) {
        if (edges[start].used) continue;
        Ring2 ring;
        std::size_t current = start;
        const auto first_point = edges[start].from;
        bool closed = false;

        for (std::size_t guard = 0; guard <= edges.size(); ++guard) {
            auto& edge = edges[current];
            if (edge.used) break;
            edge.used = true;
            ring.vertices.push_back({static_cast<double>(edge.from.first) * cell_w,
                                     static_cast<double>(edge.from.second) * cell_h});
            const auto endpoint = edge.to;
            if (endpoint == first_point) { closed = true; break; }

            const auto it = outgoing.find(endpoint);
            if (it == outgoing.end()) break;
            std::size_t next = edges.size();
            int best_priority = 99;
            for (const auto candidate : it->second) {
                if (edges[candidate].used) continue;
                const int priority = turn_priority(direction(edge), direction(edges[candidate]));
                if (priority < best_priority) {
                    best_priority = priority;
                    next = candidate;
                }
            }
            if (next == edges.size()) break;
            current = next;
        }

        if (closed && ring.vertices.size() >= 3U) {
            if (ring.signed_area_m2() > 1e-10) rings.push_back(std::move(ring));
            else if (ring.signed_area_m2() < -1e-10) rings.push_back(std::move(ring));
        }
    }
    return rings;
}

void append_svg_escape(std::ostringstream& out, const std::string& value) {
    for (const char c : value) {
        switch (c) {
            case '&': out << "&amp;"; break;
            case '<': out << "&lt;"; break;
            case '>': out << "&gt;"; break;
            case '"': out << "&quot;"; break;
            case '\'': out << "&apos;"; break;
            default: out << c; break;
        }
    }
}
} // namespace

std::vector<BoundarySegment2D> classify_boundaries(const BuildingGeometry2D& geometry,double tolerance) {
    struct EdgeRef { Point2 a,b; RoomId room; };
    std::vector<EdgeRef> edges;
    for(const auto& room:geometry.rooms)
        for(const auto& ring:room.rings)
            for(std::size_t i=0;i<ring.vertices.size();++i)
                edges.push_back({ring.vertices[i],ring.vertices[(i+1U)%ring.vertices.size()],room.id});

    const auto length=[](Point2 a,Point2 b){return std::hypot(b.x-a.x,b.y-a.y);};
    const auto cross=[](Point2 a,Point2 b,Point2 c){
        return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    };
    const auto near=[&](double a,double b){return std::abs(a-b)<=tolerance;};
    const auto point_at=[](Point2 a,Point2 b,double t){
        return Point2{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
    };
    const auto on_outer=[&](Point2 a,Point2 b){
        return (near(a.x,0.0)&&near(b.x,0.0)) ||
               (near(a.x,geometry.width_m)&&near(b.x,geometry.width_m)) ||
               (near(a.y,0.0)&&near(b.y,0.0)) ||
               (near(a.y,geometry.depth_m)&&near(b.y,geometry.depth_m));
    };

    struct Piece { Point2 a,b; RoomId ra,rb; BoundaryKind2D kind; };
    std::vector<Piece> pieces;
    for(const auto& edge:edges) {
        const double len=length(edge.a,edge.b);
        if(len<=tolerance) continue;
        const double dx=(edge.b.x-edge.a.x)/len,dy=(edge.b.y-edge.a.y)/len;
        std::vector<double> cuts{0.0,1.0};
        for(const auto& other:edges) {
            if(other.room==edge.room) continue;
            if(std::abs(cross(edge.a,edge.b,other.a))>tolerance*len ||
               std::abs(cross(edge.a,edge.b,other.b))>tolerance*len) continue;
            const double t0=((other.a.x-edge.a.x)*dx+(other.a.y-edge.a.y)*dy)/len;
            const double t1=((other.b.x-edge.a.x)*dx+(other.b.y-edge.a.y)*dy)/len;
            const double lo=std::max(0.0,std::min(t0,t1));
            const double hi=std::min(len,std::max(t0,t1));
            if(hi-lo>tolerance) { cuts.push_back(lo/len); cuts.push_back(hi/len); }
        }
        std::sort(cuts.begin(),cuts.end());
        cuts.erase(std::unique(cuts.begin(),cuts.end(),[&](double a,double b){
            return std::abs(a-b)<=tolerance/len;
        }),cuts.end());
        for(std::size_t i=1;i<cuts.size();++i) {
            const double t0=cuts[i-1],t1=cuts[i];
            if((t1-t0)*len<=tolerance) continue;
            const Point2 a=point_at(edge.a,edge.b,t0),b=point_at(edge.a,edge.b,t1);
            const Point2 mid=point_at(a,b,0.5);
            RoomId partner=static_cast<RoomId>(-1);
            for(const auto& other:edges) {
                if(other.room==edge.room) continue;
                if(std::abs(cross(other.a,other.b,mid))>tolerance*length(other.a,other.b)) continue;
                const double olen=length(other.a,other.b);
                if(olen<=tolerance) continue;
                const double proj=((mid.x-other.a.x)*(other.b.x-other.a.x)+
                                   (mid.y-other.a.y)*(other.b.y-other.a.y))/(olen*olen);
                if(proj>=-tolerance/olen && proj<=1.0+tolerance/olen) {
                    partner=other.room; break;
                }
            }
            const bool exterior=on_outer(a,b);
            if(partner==static_cast<RoomId>(-1) && !exterior) continue;
            if(partner!=static_cast<RoomId>(-1) && partner<edge.room) continue;
            pieces.push_back({a,b,edge.room,partner,
                partner==static_cast<RoomId>(-1)?BoundaryKind2D::ExteriorWall:BoundaryKind2D::SharedWall});
        }
    }

    // Deduplicate pieces emitted from both room sides, then merge collinear
    // adjacent pieces with identical semantic ownership and boundary kind.
    const auto same_undirected=[&](const Piece&a,const Piece&b){
        return (near(a.a.x,b.a.x)&&near(a.a.y,b.a.y)&&near(a.b.x,b.b.x)&&near(a.b.y,b.b.y)) ||
               (near(a.a.x,b.b.x)&&near(a.a.y,b.b.y)&&near(a.b.x,b.a.x)&&near(a.b.y,b.a.y));
    };
    std::vector<Piece> unique;
    for(const auto& piece:pieces) {
        if(std::none_of(unique.begin(),unique.end(),[&](const Piece& p){return same_undirected(p,piece);}))
            unique.push_back(piece);
    }
    bool changed=true;
    while(changed) {
        changed=false;
        for(std::size_t i=0;i<unique.size()&&!changed;++i) {
            for(std::size_t j=i+1;j<unique.size();++j) {
                auto&a=unique[i];const auto&b=unique[j];
                if(a.kind!=b.kind||a.ra!=b.ra||a.rb!=b.rb) continue;
                if(std::abs(cross(a.a,a.b,b.a))>tolerance*length(a.a,a.b)||
                   std::abs(cross(a.a,a.b,b.b))>tolerance*length(a.a,a.b)) continue;
                Point2 shared{};bool touches=false;
                if(near(a.a.x,b.a.x)&&near(a.a.y,b.a.y)){shared=a.a;touches=true;}
                else if(near(a.a.x,b.b.x)&&near(a.a.y,b.b.y)){shared=a.a;touches=true;}
                else if(near(a.b.x,b.a.x)&&near(a.b.y,b.a.y)){shared=a.b;touches=true;}
                else if(near(a.b.x,b.b.x)&&near(a.b.y,b.b.y)){shared=a.b;touches=true;}
                if(!touches) continue;
                std::vector<Point2> ends;
                for(Point2 q:{a.a,a.b,b.a,b.b})
                    if(length(q,shared)>tolerance) ends.push_back(q);
                if(ends.size()!=2U) continue;
                a.a=ends[0];a.b=ends[1];
                unique.erase(unique.begin()+static_cast<std::ptrdiff_t>(j));
                changed=true;break;
            }
        }
    }

    std::vector<BoundarySegment2D> result;
    result.reserve(unique.size());
    std::uint64_t id=1;
    for(const auto& p:unique) {
        BoundarySegment2D boundary;
        boundary.id=id++;boundary.start=p.a;boundary.end=p.b;
        boundary.length_m=length(p.a,p.b);boundary.kind=p.kind;
        boundary.room_a=p.ra;boundary.room_b=p.rb;
        result.push_back(boundary);
    }
    return result;
}


WallTopology2D build_wall_topology(const BuildingGeometry2D& geometry,double tolerance) {
    WallTopology2D graph;
    graph.walls=classify_boundaries(geometry,tolerance);
    struct EndpointRef { Point2 point; std::uint64_t wall_id; };
    std::vector<EndpointRef> endpoints;
    for(const auto& wall:graph.walls) {
        if(wall.id==0U || wall.length_m<=tolerance ||
           std::abs(distance(wall.start,wall.end)-wall.length_m)>std::max(tolerance,wall.length_m*1e-8)) {
            graph.valid=false;
            graph.diagnostics.push_back("Wall has an invalid ID, length, or endpoint geometry.");
            continue;
        }
        endpoints.push_back({wall.start,wall.id});
        endpoints.push_back({wall.end,wall.id});
    }
    const auto close=[&](Point2 a,Point2 b){
        return distance(a,b)<=tolerance;
    };
    for(const auto& endpoint:endpoints) {
        auto it=std::find_if(graph.junctions.begin(),graph.junctions.end(),
            [&](const WallNode2D& node){return close(node.position,endpoint.point);});
        if(it==graph.junctions.end()) {
            WallNode2D node;
            node.id=static_cast<std::uint64_t>(graph.junctions.size()+1U);
            node.position=endpoint.point;
            node.connected_wall_ids.push_back(endpoint.wall_id);
            graph.junctions.push_back(std::move(node));
        } else if(std::find(it->connected_wall_ids.begin(),it->connected_wall_ids.end(),
                            endpoint.wall_id)==it->connected_wall_ids.end()) {
            it->connected_wall_ids.push_back(endpoint.wall_id);
        }
    }
    for(auto& node:graph.junctions) {
        std::sort(node.connected_wall_ids.begin(),node.connected_wall_ids.end());
        const auto degree=node.connected_wall_ids.size();
        if(degree==1U) node.kind=WallJunctionKind2D::Endpoint;
        else if(degree==2U) {
            const auto find_wall=[&](std::uint64_t id)->const BoundarySegment2D*{
                const auto it=std::find_if(graph.walls.begin(),graph.walls.end(),
                    [&](const BoundarySegment2D& wall){return wall.id==id;});
                return it==graph.walls.end()?nullptr:&*it;
            };
            const auto* a=find_wall(node.connected_wall_ids[0]);
            const auto* b=find_wall(node.connected_wall_ids[1]);
            if(!a||!b) {
                graph.valid=false;
                graph.diagnostics.push_back("Junction references a missing wall.");
                continue;
            }
            const Point2 va=close(a->start,node.position)
                ?Point2{a->end.x-node.position.x,a->end.y-node.position.y}
                :Point2{a->start.x-node.position.x,a->start.y-node.position.y};
            const Point2 vb=close(b->start,node.position)
                ?Point2{b->end.x-node.position.x,b->end.y-node.position.y}
                :Point2{b->start.x-node.position.x,b->start.y-node.position.y};
            const double denom=std::max(tolerance,distance({0,0},va)*distance({0,0},vb));
            const double cosine=(va.x*vb.x+va.y*vb.y)/denom;
            node.kind=std::abs(cosine+1.0)<=1e-6
                ?WallJunctionKind2D::Straight:WallJunctionKind2D::Corner;
        } else if(degree==3U) node.kind=WallJunctionKind2D::Tee;
        else if(degree==4U) node.kind=WallJunctionKind2D::Cross;
        else node.kind=WallJunctionKind2D::Complex;
    }
    if(graph.walls.empty()) {
        graph.valid=false;
        graph.diagnostics.push_back("Wall topology contains no wall segments.");
    }
    return graph;
}


bool add_wall_opening(WallTopology2D& topology,std::uint64_t wall_id,
                      OpeningKind2D kind,double offset_m,double width_m,
                      const OpeningSettings2D& settings,std::string* error,double tolerance) {
    const auto fail=[&](const std::string& message){if(error)*error=message;return false;};
    const auto wall_it=std::find_if(topology.walls.begin(),topology.walls.end(),
        [&](const BoundarySegment2D& wall){return wall.id==wall_id;});
    if(wall_it==topology.walls.end()) return fail("Opening references an unknown wall ID.");
    if(!std::isfinite(offset_m)||!std::isfinite(width_m)||width_m<=tolerance)
        return fail("Opening offset and width must be finite; width must be positive.");
    if((kind==OpeningKind2D::Window||kind==OpeningKind2D::EntranceDoor) &&
       wall_it->kind!=BoundaryKind2D::ExteriorWall)
        return fail("Windows and entrance doors can only be placed on exterior walls.");
    if((kind==OpeningKind2D::InteriorDoor||kind==OpeningKind2D::OpenPlan) &&
       wall_it->kind!=BoundaryKind2D::SharedWall)
        return fail("Interior doors and open-plan connections require a shared wall.");
    const double end=offset_m+width_m;
    if(offset_m<settings.minimum_end_clearance_m-tolerance ||
       end>wall_it->length_m-settings.minimum_end_clearance_m+tolerance)
        return fail("Opening violates the required wall-end clearance.");
    for(const auto& existing:topology.openings) {
        if(existing.wall_id!=wall_id) continue;
        const double a=existing.offset_m,b=a+existing.width_m;
        if(std::min(end,b)-std::max(offset_m,a)>tolerance)
            return fail("Opening overlaps an existing opening on this wall.");
    }
    WallOpening2D opening;
    opening.id=topology.openings.empty()?1U:topology.openings.back().id+1U;
    opening.wall_id=wall_id;opening.kind=kind;
    opening.offset_m=offset_m;opening.width_m=width_m;
    opening.height_m=kind==OpeningKind2D::Window?settings.window_height_m:settings.door_height_m;
    opening.sill_height_m=kind==OpeningKind2D::Window?settings.window_sill_height_m:0.0;
    opening.room_a=wall_it->room_a;opening.room_b=wall_it->room_b;
    topology.openings.push_back(opening);
    rebuild_wall_pieces(topology,tolerance);
    return true;
}


CirculationGraph2D build_circulation_graph(const WallTopology2D& topology,
        const std::vector<RoomId>& room_ids,double minimum_clear_width_m,double tolerance) {
    CirculationGraph2D graph;
    const auto report=[&](const std::string& message){graph.valid=false;graph.diagnostics.push_back(message);};
    if(!std::isfinite(minimum_clear_width_m)||minimum_clear_width_m<=0.0) {
        report("Minimum circulation clear width must be finite and positive.");
        return graph;
    }
    std::vector<RoomId> rooms=room_ids;
    std::sort(rooms.begin(),rooms.end());
    if(std::adjacent_find(rooms.begin(),rooms.end())!=rooms.end())
        report("Room ID list contains duplicates.");
    if(rooms.empty()) {
        report("Circulation graph requires at least one room.");
        return graph;
    }
    const auto has_room=[&](RoomId id){return std::binary_search(rooms.begin(),rooms.end(),id);};
    std::vector<RoomId> seeds;
    for(const auto& opening:topology.openings) {
        const auto wall=std::find_if(topology.walls.begin(),topology.walls.end(),
            [&](const BoundarySegment2D& candidate){return candidate.id==opening.wall_id;});
        if(wall==topology.walls.end()) {
            report("Opening references a wall missing from the topology.");
            continue;
        }
        if(!std::isfinite(opening.width_m)||opening.width_m<=tolerance||
           !std::isfinite(opening.offset_m)||opening.offset_m< -tolerance||
           opening.offset_m+opening.width_m>wall->length_m+tolerance) {
            report("Opening has invalid dimensions or lies outside its source wall.");
            continue;
        }
        if(opening.kind==OpeningKind2D::Window) continue;
        if(opening.width_m+tolerance<minimum_clear_width_m) {
            report("Access opening "+std::to_string(opening.id)+" is narrower than the minimum clear width.");
            continue;
        }
        if(opening.kind==OpeningKind2D::EntranceDoor) {
            if(wall->kind!=BoundaryKind2D::ExteriorWall) {
                report("Entrance door must be placed on an exterior wall.");
                continue;
            }
            if(!has_room(opening.room_a)) {
                report("Entrance door references a room absent from the room model.");
                continue;
            }
            graph.links.push_back({opening.room_a,opening.room_a,opening.id,
                                   opening.width_m,opening.kind,true});
            seeds.push_back(opening.room_a);
            continue;
        }
        if(wall->kind!=BoundaryKind2D::SharedWall||!has_room(opening.room_a)||
           !has_room(opening.room_b)||opening.room_a==opening.room_b) {
            report("Interior access opening must connect two distinct known rooms on a shared wall.");
            continue;
        }
        graph.links.push_back({opening.room_a,opening.room_b,opening.id,
                               opening.width_m,opening.kind,false});
    }
    std::sort(seeds.begin(),seeds.end());
    seeds.erase(std::unique(seeds.begin(),seeds.end()),seeds.end());
    std::vector<RoomId> queue=seeds;
    for(std::size_t i=0;i<queue.size();++i) {
        const RoomId current=queue[i];
        for(const auto& link:graph.links) {
            if(link.is_entrance) continue;
            RoomId next=static_cast<RoomId>(-1);
            if(link.from_room==current) next=link.to_room;
            else if(link.to_room==current) next=link.from_room;
            if(next!=static_cast<RoomId>(-1)&&
               std::find(queue.begin(),queue.end(),next)==queue.end())
                queue.push_back(next);
        }
    }
    graph.reachable_room_ids=queue;
    std::sort(graph.reachable_room_ids.begin(),graph.reachable_room_ids.end());
    for(const RoomId id:rooms)
        if(!std::binary_search(graph.reachable_room_ids.begin(),graph.reachable_room_ids.end(),id)) {
            graph.inaccessible_room_ids.push_back(id);
            report("Room "+std::to_string(id)+" is not reachable from an exterior entrance.");
        }
    return graph;
}

void rebuild_wall_pieces(WallTopology2D& topology,double tolerance) {
    topology.wall_pieces.clear();
    std::uint64_t next_id=1;
    for(const auto& wall:topology.walls) {
        std::vector<std::pair<double,double>> intervals;
        for(const auto& opening:topology.openings)
            if(opening.wall_id==wall.id)
                intervals.push_back({opening.offset_m,opening.offset_m+opening.width_m});
        std::sort(intervals.begin(),intervals.end());
        double cursor=0.0;
        const double dx=(wall.end.x-wall.start.x)/wall.length_m;
        const double dy=(wall.end.y-wall.start.y)/wall.length_m;
        const auto emit=[&](double from,double to,bool gap) {
            if(to-from<=tolerance)return;
            WallPiece2D piece;
            piece.id=next_id++;piece.source_wall_id=wall.id;
            piece.start={wall.start.x+dx*from,wall.start.y+dy*from};
            piece.end={wall.start.x+dx*to,wall.start.y+dy*to};
            piece.length_m=to-from;piece.opening_gap=gap;
            topology.wall_pieces.push_back(piece);
        };
        for(const auto& interval:intervals) {
            const double a=std::clamp(interval.first,0.0,wall.length_m);
            const double b=std::clamp(interval.second,0.0,wall.length_m);
            if(a>cursor+tolerance)emit(cursor,a,false);
            if(b>a+tolerance)emit(a,b,true);
            cursor=std::max(cursor,b);
        }
        if(wall.length_m>cursor+tolerance)emit(cursor,wall.length_m,false);
    }
}

std::vector<std::uint64_t> generate_default_openings(
    WallTopology2D& topology,const Entrance2D& entrance,const OpeningSettings2D& settings) {
    std::vector<std::uint64_t> created;
    if(entrance.defined) {
        const Point2 midpoint{(entrance.start_x+entrance.end_x)*0.5,
                              (entrance.start_y+entrance.end_y)*0.5};
        for(const auto& wall:topology.walls) {
            if(wall.kind!=BoundaryKind2D::ExteriorWall)continue;
            const double dx=wall.end.x-wall.start.x,dy=wall.end.y-wall.start.y;
            const double len2=dx*dx+dy*dy;
            if(len2<=1e-14)continue;
            const double t=((midpoint.x-wall.start.x)*dx+(midpoint.y-wall.start.y)*dy)/len2;
            if(t< -1e-7||t>1.0+1e-7)continue;
            const Point2 projected{wall.start.x+t*dx,wall.start.y+t*dy};
            if(std::hypot(projected.x-midpoint.x,projected.y-midpoint.y)>1e-5)continue;
            const double width=entrance.clear_width_m>0?entrance.clear_width_m:settings.entrance_width_m;
            std::string ignored;
            if(add_wall_opening(topology,wall.id,OpeningKind2D::EntranceDoor,
                                wall.length_m*t-width*0.5,width,settings,&ignored)) {
                created.push_back(topology.openings.back().id);
                break;
            }
        }
    }
    const auto walls=topology.walls;
    for(const auto& wall:walls) {
        const double width=wall.kind==BoundaryKind2D::SharedWall
            ?settings.interior_door_width_m:settings.window_width_m;
        if(wall.length_m<width+2.0*settings.minimum_end_clearance_m)continue;
        const double offset=(wall.length_m-width)*0.5;
        const auto kind=wall.kind==BoundaryKind2D::SharedWall
            ?OpeningKind2D::InteriorDoor:OpeningKind2D::Window;
        std::string ignored;
        if(add_wall_opening(topology,wall.id,kind,offset,width,settings,&ignored))
            created.push_back(topology.openings.back().id);
    }
    return created;
}

std::vector<std::uint64_t> generate_residential_openings(
        WallTopology2D& topology,const FloorPlan& plan,const OpeningSettings2D& settings) {
    (void)generate_default_openings(topology,plan.entrance,settings);
    const auto role_for=[&](RoomId id) {
        const auto it=std::find_if(plan.rooms.begin(),plan.rooms.end(),
            [&](const RoomRegion& room){return room.id==id;});
        return it==plan.rooms.end()?RoomRole::Auto:it->role;
    };
    std::vector<WallOpening2D> retained;
    retained.reserve(topology.openings.size());
    std::vector<std::uint64_t> ids;
    for(auto opening:topology.openings) {
        if(opening.kind==OpeningKind2D::Window) {
            const auto role=role_for(opening.room_a);
            if(role==RoomRole::Hallway||role==RoomRole::Circulation||role==RoomRole::Service)
                continue;
        }
        if(opening.kind==OpeningKind2D::Window) {
            const auto role=role_for(opening.room_a);
            double target_width=1.2;
            if(role==RoomRole::Living) {
                target_width=1.8;opening.sill_height_m=0.6;opening.height_m=1.5;
            } else if(role==RoomRole::Bedroom) {
                target_width=1.5;opening.sill_height_m=0.9;opening.height_m=1.2;
            } else if(role==RoomRole::Kitchen) {
                target_width=1.2;opening.sill_height_m=1.0;opening.height_m=1.0;
            } else if(role==RoomRole::Bathroom) {
                target_width=0.6;opening.sill_height_m=1.5;opening.height_m=0.6;
            }
            const auto wall=std::find_if(topology.walls.begin(),topology.walls.end(),
                [&](const BoundarySegment2D& item){return item.id==opening.wall_id;});
            if(wall!=topology.walls.end()) {
                const double available=wall->length_m-2.0*settings.minimum_end_clearance_m;
                if(available>=0.5) {
                    opening.width_m=std::min(target_width,available);
                    opening.offset_m=(wall->length_m-opening.width_m)*0.5;
                }
            }
        }
        if(opening.kind==OpeningKind2D::InteriorDoor) {
            const auto a=role_for(opening.room_a),b=role_for(opening.room_b);
            const bool open_plan_pair=
                (a==RoomRole::Living&&(b==RoomRole::Kitchen||b==RoomRole::Hallway))||
                (b==RoomRole::Living&&(a==RoomRole::Kitchen||a==RoomRole::Hallway))||
                (a==RoomRole::Public&&b==RoomRole::Circulation)||
                (b==RoomRole::Public&&a==RoomRole::Circulation);
            if(open_plan_pair) {
                opening.kind=OpeningKind2D::OpenPlan;
                const auto wall=std::find_if(topology.walls.begin(),topology.walls.end(),
                    [&](const BoundarySegment2D& item){return item.id==opening.wall_id;});
                if(wall!=topology.walls.end()) {
                    const double available=wall->length_m-2.0*settings.minimum_end_clearance_m;
                    if(available>=1.2) {
                        opening.width_m=std::min(1.5,available);
                        opening.offset_m=(wall->length_m-opening.width_m)*0.5;
                    }
                }
            }
        }
        ids.push_back(opening.id);
        retained.push_back(opening);
    }
    topology.openings=std::move(retained);
    rebuild_wall_pieces(topology);
    return ids;
}

double Ring2::signed_area_m2() const noexcept {
    if (vertices.size() < 3U) return 0.0;
    double twice_area = 0.0;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto& a = vertices[i];
        const auto& b = vertices[(i + 1U) % vertices.size()];
        twice_area += a.x * b.y - b.x * a.y;
    }
    return twice_area * 0.5;
}

Geometry2DResult build_geometry_2d(const FloorPlan& plan) {
    Geometry2DResult result;
    result.geometry.width_m = plan.width_m;
    result.geometry.depth_m = plan.depth_m;
    result.geometry.entrance = plan.entrance;

    const auto fail = [&](const std::string& message) {
        result.valid = false;
        result.errors.push_back(message);
    };
    if (!std::isfinite(plan.width_m) || !std::isfinite(plan.depth_m) ||
        plan.width_m <= 0.0 || plan.depth_m <= 0.0) {
        fail("Building dimensions must be finite and positive.");
        return result;
    }
    if (plan.rooms.empty()) {
        fail("Cannot create 2D geometry without rooms.");
        return result;
    }

    if (plan.algorithm == FloorPlanAlgorithm::GridGrowth) {
        if (plan.cells.empty()) {
            fail("Grid-based plan contains no cells.");
            return result;
        }
        std::uint32_t cols = 0U, rows = 0U;
        for (const auto& cell : plan.cells) {
            cols = std::max(cols, cell.x + 1U);
            rows = std::max(rows, cell.y + 1U);
        }
        std::vector<RoomId> owners(static_cast<std::size_t>(cols) * rows,
                                   static_cast<RoomId>(-1));
        for (const auto& cell : plan.cells) {
            const auto slot = static_cast<std::size_t>(cell.y) * cols + cell.x;
            if (owners[slot] != static_cast<RoomId>(-1)) {
                fail("Grid contains duplicate cell coordinates.");
                return result;
            }
            owners[slot] = cell.room_id;
        }
        if (std::find(owners.begin(), owners.end(), static_cast<RoomId>(-1)) != owners.end()) {
            fail("Grid contains uncovered cells.");
            return result;
        }
        for (const auto& room : plan.rooms) {
            RoomGeometry2D converted{room.id, room.name, 0.0,
                extract_grid_rings(plan, room.id, cols, rows, owners)};
            for (const auto& ring : converted.rings)
                converted.area_m2 += ring.signed_area_m2();
            if (converted.rings.empty() || converted.area_m2 <= 0.0) {
                fail("Room " + room.name + " did not produce a valid polygon boundary.");
                return result;
            }
            result.geometry.rooms.push_back(std::move(converted));
        }
    } else {
        for (const auto& room : plan.rooms) {
            const auto& r = room.bounds;
            if (!std::isfinite(r.x) || !std::isfinite(r.y) ||
                !std::isfinite(r.width) || !std::isfinite(r.height) ||
                r.width <= 0.0 || r.height <= 0.0) {
                fail("Treemap room " + room.name + " has invalid bounds.");
                return result;
            }
            Ring2 ring{{{r.x,r.y},{r.x+r.width,r.y},
                        {r.x+r.width,r.y+r.height},{r.x,r.y+r.height}}};
            result.geometry.rooms.push_back({room.id, room.name, ring.signed_area_m2(), {std::move(ring)}});
        }
    }

    double total_area = 0.0;
    for (const auto& room : result.geometry.rooms) total_area += room.area_m2;
    const double expected = plan.width_m * plan.depth_m;
    if (std::abs(total_area - expected) > std::max(1e-6, expected * 1e-6))
        fail("Generated room polygons do not conserve the building footprint area.");
    result.geometry.boundaries=classify_boundaries(result.geometry);
    return result;
}

bool export_geometry_svg(const BuildingGeometry2D& geometry,
                         const std::string& output_path, std::string* error) {
    if (geometry.width_m <= 0.0 || geometry.depth_m <= 0.0 ||
        geometry.rooms.empty() || output_path.empty()) {
        if (error) *error = "SVG export requires positive dimensions, rooms, and an output path.";
        return false;
    }
    constexpr double margin = 24.0;
    constexpr double scale = 48.0;
    const double width = geometry.width_m * scale + margin * 2.0;
    const double height = geometry.depth_m * scale + margin * 2.0;
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
        << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " " << height << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n"
        << "<g transform=\"translate(" << margin << "," << margin + geometry.depth_m * scale
        << ") scale(" << scale << "," << -scale << ")\">\n";
    for (const auto& room : geometry.rooms) {
        svg << "<g fill=\"" << room_color(room.id)
            << "\" stroke=\"#263238\" stroke-width=\"0.035\" fill-rule=\"evenodd\">\n";
        for (const auto& ring : room.rings) {
            if (ring.vertices.size() < 3U) continue;
            svg << "<path d=\"M " << ring.vertices.front().x << " " << ring.vertices.front().y;
            for (std::size_t i = 1; i < ring.vertices.size(); ++i)
                svg << " L " << ring.vertices[i].x << " " << ring.vertices[i].y;
            svg << " Z\"/>\n";
        }
        svg << "</g>\n";
    }
    if (geometry.entrance.defined) {
        svg << "<line x1=\"" << geometry.entrance.start_x << "\" y1=\"" << geometry.entrance.start_y
            << "\" x2=\"" << geometry.entrance.end_x << "\" y2=\"" << geometry.entrance.end_y
            << "\" stroke=\"#16a34a\" stroke-width=\"0.16\" stroke-linecap=\"round\"/>\n";
    }
    svg << "</g>\n";
    if (geometry.entrance.defined) {
        const double ex = (geometry.entrance.start_x + geometry.entrance.end_x) * 0.5;
        const double ey = (geometry.entrance.start_y + geometry.entrance.end_y) * 0.5;
        const double label_y = geometry.entrance.side == EntranceSide::South
            ? margin + geometry.depth_m * scale + 16.0
            : margin + (geometry.depth_m - ey) * scale - 8.0;
        svg << "<text x=\"" << margin + ex * scale << "\" y=\"" << label_y
            << "\" font-family=\"Arial,sans-serif\" font-size=\"11\" text-anchor=\"middle\" "
            << "fill=\"#15803d\">ENTRY</text>\n";
    }
    for (const auto& room : geometry.rooms) {
        double cx = 0.0, cy = 0.0;
        for (const auto& ring : room.rings) for (const auto& point : ring.vertices) {
            cx += point.x; cy += point.y;
        }
        std::size_t count = 0;
        for (const auto& ring : room.rings) count += ring.vertices.size();
        if (count == 0U) continue;
        cx /= static_cast<double>(count); cy /= static_cast<double>(count);
        svg << "<text x=\"" << margin + cx * scale << "\" y=\""
            << margin + (geometry.depth_m - cy) * scale
            << "\" font-family=\"Arial,sans-serif\" font-size=\"13\" text-anchor=\"middle\" "
            << "fill=\"#17202a\">";
        append_svg_escape(svg, room.name);
        svg << "</text>\n";
    }
    svg << "<text x=\"" << margin << "\" y=\"" << height - 6.0
        << "\" font-family=\"Arial,sans-serif\" font-size=\"11\" fill=\"#475569\">"
        << geometry.width_m << " m x " << geometry.depth_m << " m — Mayhem Building Forge</text>\n"
        << "</svg>\n";

    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        if (error) *error = "Could not open SVG output path: " + output_path;
        return false;
    }
    file << svg.str();
    if (!file.good()) {
        if (error) *error = "Failed while writing SVG output: " + output_path;
        return false;
    }
    return true;
}

bool export_wall_topology_svg(const BuildingGeometry2D& geometry,
                              const WallTopology2D& topology,
                              const std::string& output_path,std::string* error) {
    const auto fail=[&](const std::string& message){if(error)*error=message;return false;};
    if(geometry.width_m<=0.0||geometry.depth_m<=0.0||geometry.rooms.empty()||
       topology.walls.empty()||output_path.empty())
        return fail("Wall SVG export requires a valid building geometry, wall topology, and output path.");
    if(!topology.valid)
        return fail("Cannot export an invalid wall topology.");
    constexpr double margin=62.0,scale=52.0;
    const double width=geometry.width_m*scale+margin*2.0;
    const double height=geometry.depth_m*scale+margin*2.0+48.0;
    std::ostringstream svg;
    svg<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<width
       <<"\" height=\""<<height<<"\" viewBox=\"0 0 "<<width<<" "<<height<<"\">\n"
       <<"<rect width=\"100%\" height=\"100%\" fill=\"#f6f8fb\"/>\n"
       <<"<text x=\""<<margin<<"\" y=\"30\" fill=\"#263a56\" font-family=\"Arial,sans-serif\" font-size=\"17\" font-weight=\"700\">MAYHEM  /  FLOOR PLAN</text>\n"
       <<"<g transform=\"translate("<<margin<<","<<margin+geometry.depth_m*scale
       <<") scale("<<scale<<","<<-scale<<")\">\n";
    for(const auto& room:geometry.rooms) {
        const auto& name=room.name;
        const char* fill="#eff2f7";
        if(name.find("Living")!=std::string::npos)fill="#dff0ec";
        else if(name.find("Kitchen")!=std::string::npos)fill="#f8ecd6";
        else if(name.find("Bedroom")!=std::string::npos)fill="#e3e9fb";
        else if(name.find("Bathroom")!=std::string::npos)fill="#e0f0fa";
        else if(name.find("Hallway")!=std::string::npos)fill="#f0f2f5";
        svg<<"<g fill=\""<<fill<<"\" stroke=\"none\" fill-rule=\"evenodd\">\n";
        for(const auto& ring:room.rings) {
            if(ring.vertices.size()<3U)continue;
            svg<<"<path d=\"M "<<ring.vertices.front().x<<" "<<ring.vertices.front().y;
            for(std::size_t i=1;i<ring.vertices.size();++i)
                svg<<" L "<<ring.vertices[i].x<<" "<<ring.vertices[i].y;
            svg<<" Z\"/>\n";
        }
        svg<<"</g>\n";
    }
    // Render the typed, validated furniture layout produced by the placement engine.
    const auto furniture=generate_furniture_layout(geometry);
    if(!furniture.valid)return fail("Furniture layout failed geometric validation.");
    svg<<"<g id=\"FURNITURE\" stroke-linejoin=\"round\" stroke-linecap=\"round\">\n";
    for(const auto& item:furniture.items) {
        const double cx=item.x+item.width_m*0.5,cy=item.y+item.depth_m*0.5;
        svg<<"<g data-furniture-id=\""<<item.id<<"\" data-room-id=\""<<item.room_id
           <<"\" data-kind=\""<<static_cast<int>(item.kind)<<"\" data-label=\"";
        append_svg_escape(svg,item.label);
        svg<<"\" data-movable=\""<<(item.movable?"true":"false")
           <<"\" transform=\"translate("<<cx<<" "<<cy<<") rotate("<<item.rotation_deg
           <<") translate("<<-item.width_m*0.5<<" "<<-item.depth_m*0.5<<")\">\n";
        const char* fill="#e6ddd0";const char* stroke="#8d806d";
        switch(item.kind) {
            case FurnitureKind2D::Bed:fill="#f8f3e9";stroke="#8d806d";break;
            case FurnitureKind2D::Nightstand:fill="#c6ad8e";break;
            case FurnitureKind2D::Wardrobe:fill="#c9b79d";break;
            case FurnitureKind2D::Sofa:fill="#d8c6b2";stroke="#8f7967";break;
            case FurnitureKind2D::CoffeeTable:fill="#c6a77f";stroke="#8f7967";break;
            case FurnitureKind2D::AccentChair:fill="#c6b29a";break;
            case FurnitureKind2D::TvStand:fill="#8d9aa5";stroke="#657483";break;
            case FurnitureKind2D::Counter:fill="#e8e3d9";stroke="#8b8d8d";break;
            case FurnitureKind2D::Sink:fill="#b9d4df";stroke="#718d9b";break;
            case FurnitureKind2D::Cooktop:fill="#444b55";stroke="#202833";break;
            case FurnitureKind2D::Refrigerator:fill="#d5dce2";stroke="#8996a2";break;
            case FurnitureKind2D::DiningTable:fill="#c6a77f";stroke="#8f7967";break;
            case FurnitureKind2D::DiningChair:fill="#c6b29a";break;
            case FurnitureKind2D::Shower:fill="#dcebf0";stroke="#829ca7";break;
            case FurnitureKind2D::Toilet:fill="#ffffff";stroke="#829ca7";break;
            case FurnitureKind2D::Vanity:fill="#d8e8ee";stroke="#829ca7";break;
            case FurnitureKind2D::Console:fill="#c9b99f";stroke="#8f806c";break;
        }
        const double radius=(item.kind==FurnitureKind2D::Sofa||item.kind==FurnitureKind2D::Bed||
                             item.kind==FurnitureKind2D::AccentChair)?0.10:0.035;
        svg<<"<rect x=\"0\" y=\"0\" width=\""<<item.width_m<<"\" height=\""
           <<item.depth_m<<"\" rx=\""<<radius<<"\" fill=\""<<fill
           <<"\" stroke=\""<<stroke<<"\" stroke-width=\"0.035\"/>\n";
        if(item.kind==FurnitureKind2D::Bed) {
            svg<<"<rect x=\"0.08\" y=\"0.08\" width=\""<<std::max(0.2,item.width_m*0.42)
               <<"\" height=\""<<std::min(0.42,item.depth_m*0.24)
               <<"\" rx=\"0.06\" fill=\"#ffffff\" stroke=\"#c5b9a8\" stroke-width=\"0.025\"/>\n";
            svg<<"<rect x=\""<<item.width_m*0.52<<"\" y=\"0.08\" width=\""
               <<std::max(0.2,item.width_m*0.40)<<"\" height=\""<<std::min(0.42,item.depth_m*0.24)
               <<"\" rx=\"0.06\" fill=\"#ffffff\" stroke=\"#c5b9a8\" stroke-width=\"0.025\"/>\n";
        } else if(item.kind==FurnitureKind2D::Sofa) {
            svg<<"<line x1=\"0.12\" y1=\""<<item.depth_m*0.34<<"\" x2=\""
               <<item.width_m-0.12<<"\" y2=\""<<item.depth_m*0.34
               <<"\" stroke=\"#a18a76\" stroke-width=\"0.04\"/>\n";
        } else if(item.kind==FurnitureKind2D::Sink) {
            svg<<"<ellipse cx=\""<<item.width_m*0.5<<"\" cy=\""<<item.depth_m*0.5
               <<"\" rx=\""<<item.width_m*0.30<<"\" ry=\""<<item.depth_m*0.32
               <<"\" fill=\"#e9f5f8\" stroke=\"#718d9b\" stroke-width=\"0.035\"/>\n";
        } else if(item.kind==FurnitureKind2D::Cooktop) {
            for(int burner=0;burner<4;++burner)
                svg<<"<circle cx=\""<<item.width_m*(burner%2?0.72:0.28)<<"\" cy=\""
                   <<item.depth_m*(burner/2?0.72:0.28)<<"\" r=\"0.11\" fill=\"none\" stroke=\"#d6d9dd\" stroke-width=\"0.035\"/>\n";
        } else if(item.kind==FurnitureKind2D::Toilet) {
            svg<<"<ellipse cx=\""<<item.width_m*0.5<<"\" cy=\""<<item.depth_m*0.58
               <<"\" rx=\""<<item.width_m*0.34<<"\" ry=\""<<item.depth_m*0.32
               <<"\" fill=\"#ffffff\" stroke=\"#829ca7\" stroke-width=\"0.035\"/>\n";
        } else if(item.kind==FurnitureKind2D::DiningTable) {
            svg<<"<line x1=\""<<item.width_m*0.5<<"\" y1=\"0.08\" x2=\""
               <<item.width_m*0.5<<"\" y2=\""<<item.depth_m-0.08
               <<"\" stroke=\"#8f7967\" stroke-width=\"0.025\"/>\n";
        }
        svg<<"</g>\n";
    }
    svg<<"</g>\n";
    for(const auto& wall:topology.walls)
        svg<<"<line x1=\""<<wall.start.x<<"\" y1=\""<<wall.start.y
           <<"\" x2=\""<<wall.end.x<<"\" y2=\""<<wall.end.y
           <<"\" stroke=\"#172033\" stroke-width=\"0.13\" stroke-linecap=\"square\"/>\n";
    for(const auto& opening:topology.openings) {
        const auto wall=std::find_if(topology.walls.begin(),topology.walls.end(),
            [&](const BoundarySegment2D& item){return item.id==opening.wall_id;});
        if(wall==topology.walls.end())continue;
        if(opening.offset_m<0.0||opening.width_m<=0.0||
           opening.offset_m+opening.width_m>wall->length_m+1e-6)
            return fail("An opening lies outside its source wall.");
        const double dx=(wall->end.x-wall->start.x)/wall->length_m;
        const double dy=(wall->end.y-wall->start.y)/wall->length_m;
        const Point2 a{wall->start.x+dx*opening.offset_m,
                       wall->start.y+dy*opening.offset_m};
        const Point2 b{wall->start.x+dx*(opening.offset_m+opening.width_m),
                       wall->start.y+dy*(opening.offset_m+opening.width_m)};
        const char* color=opening.kind==OpeningKind2D::Window?"#0284c7":
                          opening.kind==OpeningKind2D::EntranceDoor?"#16a34a":
                          opening.kind==OpeningKind2D::OpenPlan?"#7c3aed":"#d97706";
        svg<<"<line x1=\""<<a.x<<"\" y1=\""<<a.y<<"\" x2=\""<<b.x
           <<"\" y2=\""<<b.y<<"\" stroke=\"#ffffff\" stroke-width=\"0.25\"/>\n"
           <<"<line x1=\""<<a.x<<"\" y1=\""<<a.y<<"\" x2=\""<<b.x
           <<"\" y2=\""<<b.y<<"\" stroke=\""<<color
           <<"\" stroke-width=\"0.09\" stroke-linecap=\"round\"/>\n";
    }
    if(geometry.entrance.defined)
        svg<<"<line x1=\""<<geometry.entrance.start_x<<"\" y1=\""<<geometry.entrance.start_y
           <<"\" x2=\""<<geometry.entrance.end_x<<"\" y2=\""<<geometry.entrance.end_y
           <<"\" stroke=\"#16a34a\" stroke-width=\"0.07\"/>\n";
    for(const auto& room:geometry.rooms) {
        double cx=0.0,cy=0.0;std::size_t count=0;
        for(const auto& ring:room.rings)for(const auto& point:ring.vertices) {
            cx+=point.x;cy+=point.y;++count;
        }
        if(count==0U)continue;
        cx/=static_cast<double>(count);cy/=static_cast<double>(count);
        double area_m2=0.0;
        for(const auto& ring:room.rings)area_m2+=ring.signed_area_m2();
        // Restore upright orientation after the drawing-layer Y-axis inversion.
        svg<<"<g transform=\"translate("<<cx<<" "<<cy<<") scale(1 -1)\">\n";
        svg<<"<text x=\"0\" y=\"-0.06\" font-family=\"Arial,sans-serif\" font-size=\"0.30\" "
           <<"font-weight=\"700\" text-anchor=\"middle\" fill=\"#24364d\">";
        append_svg_escape(svg,room.name);
        svg<<"</text>\n"
           <<"<text x=\"0\" y=\"0.28\" font-family=\"Arial,sans-serif\" font-size=\"0.22\" "
           <<"text-anchor=\"middle\" fill=\"#62758a\">"
           <<std::round(std::abs(area_m2)*10.0)/10.0<<" m2</text></g>\n";
    }
    svg<<"</g>\n"
       <<"<g font-family=\"Arial,sans-serif\" font-size=\"11\" fill=\"#334155\">"
       <<"<text x=\""<<margin<<"\" y=\""<<height-8<<"\" >"
       <<geometry.width_m<<" m x "<<geometry.depth_m<<" m · Walls / openings</text>"
       <<"<text x=\""<<margin<<"\" y=\"14\">"
       <<"GREEN  ENTRANCE    AMBER  DOOR    BLUE  WINDOW    PURPLE  OPEN PLAN</text>"
       <<"</g></svg>\n";
    std::ofstream file(output_path,std::ios::binary);
    if(!file)return fail("Could not open wall topology SVG output path: "+output_path);
    file<<svg.str();
    if(!file.good())return fail("Failed while writing wall topology SVG output: "+output_path);
    return true;
}
} // namespace mbf
