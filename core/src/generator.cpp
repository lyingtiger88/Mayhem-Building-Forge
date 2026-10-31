#include "mbf/generator.hpp"
#include "mbf/floor_plan.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <numeric>
#include <string>
#include <stdexcept>
#include <vector>
namespace mbf {
namespace {
void check(const GenerationSettings&s,const std::vector<RoomSpec>&rs){
 if(!(s.width_m>0)||!(s.depth_m>0)||!std::isfinite(s.width_m)||!std::isfinite(s.depth_m))throw std::invalid_argument("Building dimensions must be finite and positive.");
 if(s.grid_resolution<4U||s.grid_resolution>512U)throw std::invalid_argument("Grid resolution must be 4..512.");
 if(rs.empty())throw std::invalid_argument("At least one room is required.");
 for(const auto&r:rs)if(r.name.empty()||!(r.target_area_m2>0)||!std::isfinite(r.target_area_m2))throw std::invalid_argument("Room name and target area must be valid.");
}
std::vector<double> weights(const std::vector<RoomSpec>&rs){double sum=0;for(const auto&r:rs)sum+=r.target_area_m2;std::vector<double>w;for(const auto&r:rs)w.push_back(r.target_area_m2/sum);return w;}
}
FloorPlan FloorPlanGenerator::generate_grid_growth(const GenerationSettings&s,const std::vector<RoomSpec>&rs)const{
 check(s,rs);const auto cols=s.grid_resolution;const auto rows=std::max<std::uint32_t>(4U,static_cast<std::uint32_t>(std::lround(static_cast<double>(cols)*s.depth_m/s.width_m)));const auto total=static_cast<std::size_t>(cols)*rows;
 const auto idx=[cols](std::uint32_t x,std::uint32_t y){return static_cast<std::size_t>(y)*cols+x;};
 std::vector<RoomId> owner(total,0U);std::vector<bool> used(total,false);std::mt19937_64 rng(s.seed);std::uniform_int_distribution<std::uint32_t>xd(0U,cols-1U),yd(0U,rows-1U);
 FloorPlan p;p.seed=s.seed;p.width_m=s.width_m;p.depth_m=s.depth_m;p.algorithm=FloorPlanAlgorithm::GridGrowth;const auto w=weights(rs);
 for(std::size_t i=0;i<rs.size();++i)p.rooms.push_back({static_cast<RoomId>(i),rs[i].name,rs[i].target_area_m2,0,{},{},rs[i].role});
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
namespace {
RoomRole resolved_role(const RoomSpec& room) {
    if (room.role != RoomRole::Auto) return room.role;
    std::string name = room.name;
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (name.find("living") != std::string::npos || name.find("lounge") != std::string::npos ||
        name.find("reception") != std::string::npos || name.find("sitting") != std::string::npos)
        return RoomRole::Living;
    if (name.find("kitchen") != std::string::npos) return RoomRole::Kitchen;
    if (name.find("bed") != std::string::npos || name.find("sleep") != std::string::npos)
        return RoomRole::Bedroom;
    if (name.find("bath") != std::string::npos || name.find("toilet") != std::string::npos ||
        name == "wc") return RoomRole::Bathroom;
    if (name.find("hall") != std::string::npos || name.find("corridor") != std::string::npos ||
        name.find("foyer") != std::string::npos || name.find("entry") != std::string::npos)
        return RoomRole::Hallway;
    return RoomRole::Auto;
}

double shared_length(const Rect& a, const Rect& b) {
    constexpr double eps = 1e-7;
    const double y_overlap = std::min(a.y + a.height, b.y + b.height) - std::max(a.y, b.y);
    const double x_overlap = std::min(a.x + a.width, b.x + b.width) - std::max(a.x, b.x);
    if ((std::abs(a.x + a.width - b.x) < eps || std::abs(b.x + b.width - a.x) < eps) &&
        y_overlap > eps) return y_overlap;
    if ((std::abs(a.y + a.height - b.y) < eps || std::abs(b.y + b.height - a.y) < eps) &&
        x_overlap > eps) return x_overlap;
    return 0.0;
}

double pair_preference(RoomRole a, RoomRole b) {
    if ((a == RoomRole::Living && b == RoomRole::Kitchen) ||
        (b == RoomRole::Living && a == RoomRole::Kitchen)) return 12.0;
    if ((a == RoomRole::Living && b == RoomRole::Hallway) ||
        (b == RoomRole::Living && a == RoomRole::Hallway)) return 10.0;
    if ((a == RoomRole::Hallway && (b == RoomRole::Bedroom || b == RoomRole::Bathroom)) ||
        (b == RoomRole::Hallway && (a == RoomRole::Bedroom || a == RoomRole::Bathroom))) return 11.0;
    if ((a == RoomRole::Kitchen && b == RoomRole::Hallway) ||
        (b == RoomRole::Kitchen && a == RoomRole::Hallway)) return 5.0;
    if ((a == RoomRole::Bedroom && b == RoomRole::Bathroom) ||
        (b == RoomRole::Bedroom && a == RoomRole::Bathroom)) return 2.0;
    if ((a == RoomRole::Living && b == RoomRole::Bathroom) ||
        (b == RoomRole::Living && a == RoomRole::Bathroom)) return -7.0;
    if ((a == RoomRole::Living && b == RoomRole::Bedroom) ||
        (b == RoomRole::Living && a == RoomRole::Bedroom)) return -2.0;
    return 0.0;
}

double layout_score(const FloorPlan& plan, const std::vector<RoomRole>& roles) {
    double score = 0.0;
    const double min_dimension = std::min(plan.width_m, plan.depth_m);
    const double cx = plan.width_m * 0.5, cy = plan.depth_m * 0.5;
    for (std::size_t i = 0; i < plan.rooms.size(); ++i) {
        const auto& room = plan.rooms[i];
        const auto& r = room.bounds;
        const double ratio = std::max(r.width / r.height, r.height / r.width);
        if (ratio > 2.5) score -= (ratio - 2.5) * 1.8;
        if (roles[i] == RoomRole::Living) {
            const double dx = (r.x + r.width * 0.5 - cx) / std::max(plan.width_m, 1e-9);
            const double dy = (r.y + r.height * 0.5 - cy) / std::max(plan.depth_m, 1e-9);
            score -= 9.0 * std::sqrt(dx * dx + dy * dy);
        } else if (roles[i] == RoomRole::Hallway) {
            const double dx = (r.x + r.width * 0.5 - cx) / std::max(plan.width_m, 1e-9);
            const double dy = (r.y + r.height * 0.5 - cy) / std::max(plan.depth_m, 1e-9);
            score -= 3.0 * std::sqrt(dx * dx + dy * dy);
        } else if (roles[i] == RoomRole::Bedroom) {
            const double edge_gap = std::min({r.x, r.y, plan.width_m - (r.x + r.width),
                                              plan.depth_m - (r.y + r.height)});
            score -= 5.0 * edge_gap / std::max(min_dimension, 1e-9);
        }
        for (std::size_t j = i + 1; j < plan.rooms.size(); ++j) {
            const double preference = pair_preference(roles[i], roles[j]);
            if (preference == 0.0) continue;
            const double length = shared_length(r, plan.rooms[j].bounds);
            const double scale = std::max(0.5, std::min({r.width, r.height,
                                                         plan.rooms[j].bounds.width,
                                                         plan.rooms[j].bounds.height}));
            if (length > 0.0) score += preference * (1.0 + std::min(length / scale, 1.0));
            else if (preference > 0.0) score -= preference * 1.15;
        }
    }
    return score;
}

FloorPlan make_rectangular_candidate(const GenerationSettings& s,
                                     const std::vector<RoomSpec>& rooms,
                                     const std::vector<std::size_t>& order,
                                     std::uint64_t orientation_mask) {
    const auto room_weights = weights(rooms);
    FloorPlan plan;
    plan.seed = s.seed;
    plan.width_m = s.width_m;
    plan.depth_m = s.depth_m;
    plan.algorithm = FloorPlanAlgorithm::SquarifiedTreemap;
    Rect remaining{0.0, 0.0, s.width_m, s.depth_m};
    for (std::size_t position = 0; position < order.size(); ++position) {
        const std::size_t room_index = order[position];
        Rect placed{};
        if (position + 1U == order.size()) {
            placed = remaining;
        } else {
            const double target_area = s.width_m * s.depth_m * room_weights[room_index];
            const bool horizontal = ((orientation_mask >> position) & 1ULL) != 0ULL;
            if (horizontal) {
                const double cut = target_area / remaining.width;
                if (cut <= 1e-9 || cut >= remaining.height - 1e-9)
                    throw std::runtime_error("Invalid horizontal room partition.");
                placed = {remaining.x, remaining.y, remaining.width, cut};
                remaining.y += cut;
                remaining.height -= cut;
            } else {
                const double cut = target_area / remaining.height;
                if (cut <= 1e-9 || cut >= remaining.width - 1e-9)
                    throw std::runtime_error("Invalid vertical room partition.");
                placed = {remaining.x, remaining.y, cut, remaining.height};
                remaining.x += cut;
                remaining.width -= cut;
            }
        }
        plan.rooms.resize(rooms.size());
        plan.rooms[room_index] = {static_cast<RoomId>(room_index), rooms[room_index].name,
            rooms[room_index].target_area_m2, placed.area(), placed, {}, resolved_role(rooms[room_index])};
    }
    for (std::size_t i = 0; i < plan.rooms.size(); ++i)
        for (std::size_t j = i + 1; j < plan.rooms.size(); ++j)
            if (shared_length(plan.rooms[i].bounds, plan.rooms[j].bounds) > 1e-7)
                plan.adjacency.push_back({static_cast<RoomId>(i), static_cast<RoomId>(j), 1U});
    return plan;
}
} // namespace

FloorPlan FloorPlanGenerator::generate_treemap(const GenerationSettings& s,
                                                const std::vector<RoomSpec>& rs) const {
    check(s, rs);
    std::vector<RoomRole> roles;
    roles.reserve(rs.size());
    for (const auto& room : rs) roles.push_back(resolved_role(room));

    FloorPlan best;
    double best_score = -std::numeric_limits<double>::infinity();
    const std::size_t n = rs.size();
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0U);

    const auto consider = [&](const std::vector<std::size_t>& candidate_order,
                              std::uint64_t mask, FloorPlan& current_best,
                              double& current_score) {
        try {
            auto candidate = make_rectangular_candidate(s, rs, candidate_order, mask);
            const double score = layout_score(candidate, roles);
            if (score > current_score + 1e-10) {
                current_score = score;
                current_best = std::move(candidate);
            }
        } catch (const std::runtime_error&) {
            // Invalid cuts are simply not candidates.
        }
    };

    if (n <= 7U) {
        const std::uint64_t masks = 1ULL << (n - 1U);
        do {
            for (std::uint64_t mask = 0; mask < masks; ++mask)
                consider(order, mask, best, best_score);
        } while (std::next_permutation(order.begin(), order.end()));
    } else {
        // Bound search cost for larger briefs while keeping deterministic,
        // role-aware candidate exploration.
        std::mt19937_64 rng(s.seed);
        for (std::size_t attempt = 0; attempt < 512U; ++attempt) {
            std::shuffle(order.begin(), order.end(), rng);
            const std::uint64_t mask = rng();
            consider(order, mask, best, best_score);
        }
    }

    if (best.rooms.empty())
        throw std::runtime_error("Could not produce a valid rectangular room layout.");
    return best;
}

FloorPlan FloorPlanGenerator::generate_residential_layout(
    const GenerationSettings& s, const std::vector<RoomSpec>& rs,
    EntranceSide entrance_side) const {
    check(s, rs);

    std::vector<RoomRole> roles;
    roles.reserve(rs.size());
    for (const auto& room : rs) roles.push_back(resolved_role(room));

    const auto find_role = [&](RoomRole role) -> std::size_t {
        const auto it = std::find(roles.begin(), roles.end(), role);
        return it == roles.end() ? rs.size() : static_cast<std::size_t>(it - roles.begin());
    };
    const std::size_t living = find_role(RoomRole::Living);
    const std::size_t kitchen = find_role(RoomRole::Kitchen);
    const std::size_t bedroom = find_role(RoomRole::Bedroom);
    const std::size_t bathroom = find_role(RoomRole::Bathroom);
    const std::size_t hallway = find_role(RoomRole::Hallway);
    if (living == rs.size() || kitchen == rs.size() || bedroom == rs.size() ||
        bathroom == rs.size() || hallway == rs.size()) {
        throw std::invalid_argument(
            "Topology-first residential layout requires Living, Kitchen, Bedroom, Bathroom, and Hallway roles.");
    }

    const auto adjacent = [&](const FloorPlan& plan, std::size_t a, std::size_t b) {
        return shared_length(plan.rooms[a].bounds, plan.rooms[b].bounds) > 1e-7;
    };
    const auto touches_entrance_side = [&](const Rect& r) {
        constexpr double eps = 1e-7;
        switch (entrance_side) {
            case EntranceSide::South: return std::abs(r.y) < eps;
            case EntranceSide::North: return std::abs(r.y + r.height - s.depth_m) < eps;
            case EntranceSide::West: return std::abs(r.x) < eps;
            case EntranceSide::East: return std::abs(r.x + r.width - s.width_m) < eps;
        }
        return false;
    };

    FloorPlan best;
    double best_score = -std::numeric_limits<double>::infinity();
    const std::size_t n = rs.size();
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0U);

    const auto consider = [&](const std::vector<std::size_t>& candidate_order,
                              std::uint64_t mask, FloorPlan& current_best,
                              double& current_score) {
        try {
            auto candidate = make_rectangular_candidate(s, rs, candidate_order, mask);
            candidate.algorithm = FloorPlanAlgorithm::ResidentialTopologyFirst;

            // These are topology constraints, not soft preferences. A layout
            // that misses any required relationship is rejected outright.
            if (!adjacent(candidate, living, kitchen) ||
                !adjacent(candidate, living, hallway) ||
                !adjacent(candidate, hallway, kitchen) ||
                !adjacent(candidate, hallway, bedroom) ||
                !adjacent(candidate, hallway, bathroom) ||
                !touches_entrance_side(candidate.rooms[hallway].bounds)) return;

            const Rect& h = candidate.rooms[hallway].bounds;
            const double side_length =
                (entrance_side == EntranceSide::South || entrance_side == EntranceSide::North)
                    ? h.width : h.height;
            const double door_width = std::min(1.0, side_length - 0.35);
            if (door_width < 0.9) return;

            double score = layout_score(candidate, roles);
            const auto center = [](const Rect& r) {
                return std::pair<double,double>{r.x + r.width * 0.5, r.y + r.height * 0.5};
            };
            const auto [lx, ly] = center(candidate.rooms[living].bounds);
            const auto [kx, ky] = center(candidate.rooms[kitchen].bounds);
            const auto [bx, by] = center(candidate.rooms[bedroom].bounds);
            const auto [hx, hy] = center(h);
            const double diagonal = std::hypot(s.width_m, s.depth_m);

            // Balanced conventional profile: public rooms stay in the
            // southern/central part; private bedroom is deeper from entry;
            // the hall is anchored to the requested entrance facade.
            score += 7.0 * (1.0 - std::clamp((ly + ky) / (2.0 * s.depth_m), 0.0, 1.0));
            score += 5.0 * std::hypot(lx - kx, ly - ky) * -1.0 / std::max(diagonal, 1e-9);
            const double entry_x = (entrance_side == EntranceSide::East) ? s.width_m :
                                   (entrance_side == EntranceSide::West) ? 0.0 : s.width_m * 0.5;
            const double entry_y = (entrance_side == EntranceSide::North) ? s.depth_m :
                                   (entrance_side == EntranceSide::South) ? 0.0 : s.depth_m * 0.5;
            const double bedroom_entry_distance =
                std::hypot(bx - entry_x, by - entry_y) / std::max(diagonal, 1e-9);
            score += 5.0 * bedroom_entry_distance;
            score -= 2.0 * std::hypot(hx - entry_x, hy - entry_y) / std::max(diagonal, 1e-9);

            if (score <= current_score + 1e-10) return;

            candidate.entrance.defined = true;
            candidate.entrance.side = entrance_side;
            candidate.entrance.room_id = static_cast<RoomId>(hallway);
            candidate.entrance.clear_width_m = door_width;
            const double half = door_width * 0.5;
            switch (entrance_side) {
                case EntranceSide::South:
                    candidate.entrance.start_x = hx - half;
                    candidate.entrance.end_x = hx + half;
                    candidate.entrance.start_y = candidate.entrance.end_y = 0.0;
                    break;
                case EntranceSide::North:
                    candidate.entrance.start_x = hx - half;
                    candidate.entrance.end_x = hx + half;
                    candidate.entrance.start_y = candidate.entrance.end_y = s.depth_m;
                    break;
                case EntranceSide::West:
                    candidate.entrance.start_x = candidate.entrance.end_x = 0.0;
                    candidate.entrance.start_y = hy - half;
                    candidate.entrance.end_y = hy + half;
                    break;
                case EntranceSide::East:
                    candidate.entrance.start_x = candidate.entrance.end_x = s.width_m;
                    candidate.entrance.start_y = hy - half;
                    candidate.entrance.end_y = hy + half;
                    break;
            }

            candidate.circulation_links.clear();
            for (std::size_t i = 0; i < candidate.rooms.size(); ++i) {
                if (i == hallway) continue;
                const double length = shared_length(h, candidate.rooms[i].bounds);
                if (length > 1e-7) {
                    candidate.circulation_links.push_back({
                        static_cast<RoomId>(hallway), static_cast<RoomId>(i), length
                    });
                }
            }
            // An open-plan living/kitchen connection is represented separately
            // from corridor access; a future door/opening pass decides its form.
            candidate.circulation_links.push_back({
                static_cast<RoomId>(living), static_cast<RoomId>(kitchen),
                shared_length(candidate.rooms[living].bounds, candidate.rooms[kitchen].bounds)
            });

            current_score = score;
            current_best = std::move(candidate);
        } catch (const std::runtime_error&) {
            // Invalid guillotine cuts are not valid candidate layouts.
        }
    };

    if (n <= 7U) {
        const std::uint64_t masks = 1ULL << (n - 1U);
        do {
            for (std::uint64_t mask = 0; mask < masks; ++mask)
                consider(order, mask, best, best_score);
        } while (std::next_permutation(order.begin(), order.end()));
    } else {
        std::mt19937_64 rng(s.seed);
        for (std::size_t attempt = 0; attempt < 4096U; ++attempt) {
            std::shuffle(order.begin(), order.end(), rng);
            consider(order, rng(), best, best_score);
        }
    }

    if (best.rooms.empty()) {
        throw std::runtime_error(
            "No residential layout satisfies the required room topology and entrance constraints. "
            "Try changing the footprint or room area targets.");
    }
    return best;
}

} // namespace mbf
