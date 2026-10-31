#include "mbf/polygon.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace mbf {
namespace {
double cross(Point2 a, Point2 b, Point2 c) {
    return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
}
double signed_area(const std::vector<Point2>& v) {
    double twice=0.0;
    for(std::size_t i=0;i<v.size();++i) {
        const auto& a=v[i];const auto& b=v[(i+1)%v.size()];
        twice+=a.x*b.y-b.x*a.y;
    }
    return 0.5*twice;
}
bool in_triangle(Point2 p,Point2 a,Point2 b,Point2 c,double eps) {
    return cross(a,b,p)>=-eps&&cross(b,c,p)>=-eps&&cross(c,a,p)>=-eps;
}
using Triangle=std::array<Point2,3>;
std::vector<Triangle> triangulate(const Polygon2D& p,double eps) {
    std::vector<Point2> v=p.vertices;
    if(signed_area(v)<0.0)std::reverse(v.begin(),v.end());
    std::vector<std::size_t> indices(v.size());
    std::iota(indices.begin(),indices.end(),0U);
    std::vector<Triangle> triangles;
    // Ear clipping is deterministic for simple polygons with nondegenerate ears.
    while(indices.size()>3U) {
        bool removed=false;
        for(std::size_t j=0;j<indices.size();++j) {
            const auto a=indices[(j+indices.size()-1U)%indices.size()];
            const auto b=indices[j],c=indices[(j+1U)%indices.size()];
            if(cross(v[a],v[b],v[c])<=eps)continue;
            bool contains_other=false;
            for(const auto k:indices) {
                if(k==a||k==b||k==c)continue;
                if(in_triangle(v[k],v[a],v[b],v[c],eps)) {
                    contains_other=true;break;
                }
            }
            if(contains_other)continue;
            triangles.push_back({v[a],v[b],v[c]});
            indices.erase(indices.begin()+static_cast<std::ptrdiff_t>(j));
            removed=true;break;
        }
        if(!removed)return {};
    }
    if(indices.size()==3U&&cross(v[indices[0]],v[indices[1]],v[indices[2]])>eps)
        triangles.push_back({v[indices[0]],v[indices[1]],v[indices[2]]});
    return triangles;
}
std::vector<Point2> clip_halfplane(const std::vector<Point2>& input,
                                    Point2 a,Point2 b,double eps) {
    std::vector<Point2> output;
    if(input.empty())return output;
    Point2 prev=input.back();
    double prev_side=cross(a,b,prev);
    for(const auto& current:input) {
        const double side=cross(a,b,current);
        const bool was_inside=prev_side>=-eps;
        const bool is_inside=side>=-eps;
        if(was_inside!=is_inside) {
            const double denom=prev_side-side;
            if(std::abs(denom)>1e-16) {
                const double t=prev_side/denom;
                output.push_back({prev.x+t*(current.x-prev.x),
                                  prev.y+t*(current.y-prev.y)});
            }
        }
        if(is_inside)output.push_back(current);
        prev=current;prev_side=side;
    }
    return output;
}
double triangle_intersection_area(const Triangle& a,const Triangle& b,double eps) {
    std::vector<Point2> clipped(a.begin(),a.end());
    for(int i=0;i<3;++i)
        clipped=clip_halfplane(clipped,b[static_cast<std::size_t>(i)],
                              b[static_cast<std::size_t>((i+1)%3)],eps);
    return clipped.size()>=3U?std::abs(signed_area(clipped)):0.0;
}
} // namespace

double polygon_intersection_area(const Polygon2D& a,const Polygon2D& b,double tolerance) {
    if(!std::isfinite(tolerance)||tolerance<=0.0||
       !validate_polygon(a,tolerance).valid||!validate_polygon(b,tolerance).valid)
        return std::numeric_limits<double>::quiet_NaN();
    const auto ta=triangulate(a,tolerance*tolerance);
    const auto tb=triangulate(b,tolerance*tolerance);
    if(ta.empty()||tb.empty())return std::numeric_limits<double>::quiet_NaN();
    double area=0.0;
    for(const auto& x:ta)
        for(const auto& y:tb)
            area+=triangle_intersection_area(x,y,tolerance*tolerance);
    return area;
}
} // namespace mbf
