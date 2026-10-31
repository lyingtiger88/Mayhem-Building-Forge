#include "mbf/polygon.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    using namespace mbf;
    std::vector<Polygon2D> shapes;
    std::vector<std::string> names;
    shapes.push_back(make_rectangle(0,0,4,3)); names.push_back("Rectangle");
    shapes.push_back(make_regular_polygon({6,1.5},1.5,3)); names.push_back("Triangle");
    shapes.push_back(make_regular_polygon({10,1.5},1.5,5)); names.push_back("Pentagon");
    shapes.push_back(make_regular_polygon({14,1.5},1.5,6)); names.push_back("Hexagon");
    shapes.push_back({{{0,5},{4,5},{4,6.5},{2,6.5},{2,8},{0,8}}}); names.push_back("Concave L");
    for(std::size_t i=0;i<shapes.size();++i) {
        const auto v=validate_polygon(shapes[i]);
        if(!v.valid){std::cerr<<names[i]<<": "<<v.errors.front()<<'\n';return 1;}
        const auto c=polygon_centroid(shapes[i]);
        std::cout<<std::left<<std::setw(12)<<names[i]<<" area="<<std::fixed<<std::setprecision(2)
                 <<polygon_area(shapes[i])<<" m2 centroid=("<<c.x<<", "<<c.y<<")\n";
    }
    const auto left=make_rectangle(0,10,3,2),right=make_rectangle(3,10,2,2);
    std::cout<<"Shared edge length: "<<shared_boundary_length(left,right)<<" m (expected 2)\n";
    std::string error;
    if(!export_polygon_gallery_svg(shapes,names,"mayhem_polygon_gallery.svg",&error)){
        std::cerr<<error<<'\n';return 2;
    }
    std::cout<<"SVG exported: mayhem_polygon_gallery.svg\n";
}
