#include "mbf/topology.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace mbf {
namespace {
std::string xml_escape(const std::string& value) {
    std::string out;
    for(char ch:value) {
        switch(ch) {
            case '&':out+="&amp;";break;
            case '<':out+="&lt;";break;
            case '>':out+="&gt;";break;
            case '"':out+="&quot;";break;
            case '\'':out+="&apos;";break;
            default:out+=ch;break;
        }
    }
    return out;
}
const char* role_label(RoomRole role) {
    switch(role) {
        case RoomRole::Living:return "Living";
        case RoomRole::Kitchen:return "Kitchen";
        case RoomRole::Bedroom:return "Bedroom";
        case RoomRole::Bathroom:return "Bathroom";
        case RoomRole::Hallway:return "Hallway";
        case RoomRole::Public:return "Public";
        case RoomRole::Private:return "Private";
        case RoomRole::Service:return "Service";
        case RoomRole::Circulation:return "Circulation";
        case RoomRole::Auto:return "Auto";
    }
    return "Room";
}
const char* role_color(RoomRole role) {
    switch(role) {
        case RoomRole::Living:case RoomRole::Public:return "#dceafe";
        case RoomRole::Kitchen:return "#ffedc2";
        case RoomRole::Bedroom:case RoomRole::Private:return "#d9f3e4";
        case RoomRole::Bathroom:case RoomRole::Service:return "#eadcff";
        case RoomRole::Hallway:case RoomRole::Circulation:return "#e5e7eb";
        case RoomRole::Auto:return "#f1f5f9";
    }
    return "#f1f5f9";
}
}
bool export_room_topologies_svg(const std::vector<RoomTopology>& topologies,
                                const std::string& output_path,std::string* error) {
    if(topologies.empty()) {
        if(error)*error="No topology candidates were supplied.";
        return false;
    }
    constexpr int panel_width=440,panel_height=340,margin=18;
    const int width=panel_width*static_cast<int>(topologies.size())+margin*2;
    const int height=panel_height+margin*2;
    std::ofstream out(output_path,std::ios::binary);
    if(!out) {
        if(error)*error="Could not open topology SVG output path.";
        return false;
    }
    out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<width
       <<"\" height=\""<<height<<"\" viewBox=\"0 0 "<<width<<" "<<height<<"\">\n"
       <<"<rect width=\"100%\" height=\"100%\" fill=\"#f8fafc\"/>\n";
    for(std::size_t index=0;index<topologies.size();++index) {
        const auto& graph=topologies[index];
        const double left=margin+static_cast<double>(index)*panel_width;
        const double cx=left+panel_width/2.0,cy=margin+190.0;
        const double radius=std::min(112.0,38.0+static_cast<double>(graph.nodes.size())*5.0);
        out<<"<g font-family=\"Arial,sans-serif\">\n"
           <<"<rect x=\""<<left<<"\" y=\""<<margin<<"\" width=\""<<panel_width-12
           <<"\" height=\""<<panel_height-12<<"\" rx=\"12\" fill=\"white\" stroke=\"#cbd5e1\"/>\n"
           <<"<text x=\""<<cx<<"\" y=\""<<margin+28
           <<"\" text-anchor=\"middle\" font-size=\"17\" font-weight=\"700\" fill=\"#0f172a\">"
           <<xml_escape(graph.strategy_name)<<"</text>\n"
           <<"<text x=\""<<cx<<"\" y=\""<<margin+48
           <<"\" text-anchor=\"middle\" font-size=\"11\" fill=\"#64748b\">"
           <<graph.nodes.size()<<" rooms · "<<graph.edges.size()<<" adjacency links</text>\n";
        std::map<RoomId,std::pair<double,double>> positions;
        const double count=static_cast<double>(graph.nodes.size());
        for(std::size_t i=0;i<graph.nodes.size();++i) {
            const double angle=-1.5707963267948966+6.283185307179586*static_cast<double>(i)/count;
            positions[graph.nodes[i].id]={cx+radius*std::cos(angle),cy+radius*std::sin(angle)};
        }
        for(const auto& edge:graph.edges) {
            const auto a=positions.find(edge.first),b=positions.find(edge.second);
            if(a==positions.end()||b==positions.end())continue;
            out<<"<line x1=\""<<a->second.first<<"\" y1=\""<<a->second.second
               <<"\" x2=\""<<b->second.first<<"\" y2=\""<<b->second.second
               <<"\" stroke=\""<<(edge.required?"#2563eb":"#94a3b8")
               <<"\" stroke-width=\""<<(edge.required?3:2)<<"\"/>\n";
        }
        for(const auto& node:graph.nodes) {
            const auto pos=positions.at(node.id);
            out<<"<circle cx=\""<<pos.first<<"\" cy=\""<<pos.second
               <<"\" r=\"25\" fill=\""<<role_color(node.role)
               <<"\" stroke=\"#475569\" stroke-width=\"1.4\"/>\n"
               <<"<text x=\""<<pos.first<<"\" y=\""<<pos.second-3
               <<"\" text-anchor=\"middle\" font-size=\"9\" font-weight=\"700\" fill=\"#0f172a\">"
               <<xml_escape(role_label(node.role))<<"</text>\n"
               <<"<text x=\""<<pos.first<<"\" y=\""<<pos.second+9
               <<"\" text-anchor=\"middle\" font-size=\"8\" fill=\"#334155\">#"<<node.id<<"</text>\n";
        }
        out<<"<text x=\""<<left+18<<"\" y=\""<<margin+panel_height-28
           <<"\" font-size=\"10\" fill=\"#64748b\">Blue = required adjacency · Gray = generated connection</text>\n"
           <<"</g>\n";
    }
    out<<"</svg>\n";
    if(!out) {
        if(error)*error="Failed while writing topology SVG.";
        return false;
    }
    return true;
}
} // namespace mbf
