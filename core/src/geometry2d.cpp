#include "mbf/geometry2d.hpp"
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
} // namespace mbf
