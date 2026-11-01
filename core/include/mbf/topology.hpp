#pragma once
#include "mbf/building_brief.hpp"
#include <string>
#include <vector>

namespace mbf {

enum class TopologyStrategy {
    CirculationHub,
    PublicPrivateSpine,
    CompactLinear
};

struct TopologyNode {
    RoomId id{};
    std::string name;
    RoomRole role{RoomRole::Auto};
    std::uint32_t instance_index{};
};

struct TopologyEdge {
    RoomId first{};
    RoomId second{};
    bool required{false};
    double weight{1.0};
};

struct RoomTopology {
    std::uint32_t schema_version{1};
    std::uint64_t seed{};
    TopologyStrategy strategy{TopologyStrategy::CirculationHub};
    std::string strategy_name;
    std::vector<TopologyNode> nodes;
    std::vector<TopologyEdge> edges;
    bool valid{false};
    double quality_score{};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::vector<RoomTopology> generate_room_topology_candidates(
    const BuildingBrief& brief);
[[nodiscard]] bool export_room_topologies_svg(
    const std::vector<RoomTopology>& topologies, const std::string& output_path,
    std::string* error = nullptr);
[[nodiscard]] bool validate_room_topology(
    const RoomTopology& topology, const BuildingBrief& brief,
    std::vector<std::string>* diagnostics = nullptr);

} // namespace mbf
