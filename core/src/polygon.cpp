#include "mbf/polygon.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace mbf {
namespace {
constexpr double pi = 3.14159265358979323846;
double cross(Point2 a, Point2 b, Point2 c) {
    return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
}
double distance(Point2 a, Point2 b) { return std::hypot(a.x-b.x,a.y-b.y); }
bool on_segment(Point2 p, Point2 a, Point2 b, double eps) {
    return std::abs(cross(a,b,p)) <= eps &&
        p.x >= std::min(a.x,b.x)-eps && p.x <= std::max(a.x,b.x)+eps &&
        p.y >= std::min(a.y,b.y)-eps && p.y <= std::max(a.y,b.y)+eps;
}
bool segments_intersect(Point2 a, Point2 b, Point2 c, Point2 d, double eps) {
    const double o1=cross(a,b,c),o2=cross(a,b,d),o3=cross(c,d,a),o4=cross(c,d,b);
    if (((o1>eps&&o2< -eps)||(o1< -eps&&o2>eps)) &&
        ((o3>eps&&o4< -eps)||(o3< -eps&&o4>eps))) return true;
    return on_segment(c,a,b,eps)||on_segment(d,a,b,eps)||
           on_segment(a,c,d,eps)||on_segment(b,c,d,eps);
}
void escape_xml(std::ostream& out,const std::string& s) {
    for(char c:s) { if(c=='&')out<<"&amp;"; else if(c=='<')out<<"&lt;";
        else if(c=='>')out<<"&gt;"; else if(c=='"')out<<"&quot;"; else out<<c; }
}
}
Polygon2D make_rectangle(double x,double y,double w,double h) {
    return {{{x,y},{x+w,y},{x+w,y+h},{x,y+h}}};
}
Polygon2D make_regular_polygon(Point2 c,double r,std::uint32_t n,double rotation) {
    if(!std::isfinite(c.x)||!std::isfinite(c.y)||!std::isfinite(r)||r<=0||n<3||n>512)
        return {};
    Polygon2D p; p.vertices.reserve(n);
    for(std::uint32_t i=0;i<n;++i) {
        const double angle=rotation+2.0*pi*static_cast<double>(i)/static_cast<double>(n);
        p.vertices.push_back({c.x+r*std::cos(angle),c.y+r*std::sin(angle)});
    }
    return p;
}
double polygon_signed_area(const Polygon2D& p) noexcept {
    if(p.vertices.size()<3)return 0;
    double sum=0;
    for(std::size_t i=0;i<p.vertices.size();++i) {
        const auto&a=p.vertices[i];const auto&b=p.vertices[(i+1)%p.vertices.size()];
        sum+=a.x*b.y-b.x*a.y;
    }
    return sum*0.5;
}
double polygon_area(const Polygon2D& p) noexcept { return std::abs(polygon_signed_area(p)); }
Point2 polygon_centroid(const Polygon2D& p) {
    const double a6=polygon_signed_area(p)*6.0;
    if(std::abs(a6)<1e-14) return {};
    double x=0,y=0;
    for(std::size_t i=0;i<p.vertices.size();++i) {
        const auto&a=p.vertices[i];const auto&b=p.vertices[(i+1)%p.vertices.size()];
        const double k=a.x*b.y-b.x*a.y;x+=(a.x+b.x)*k;y+=(a.y+b.y)*k;
    }
    return {x/a6,y/a6};
}
PolygonValidation validate_polygon(const Polygon2D& p,double eps) {
    PolygonValidation r;
    if(p.vertices.size()<3){r.errors.push_back("A polygon needs at least three vertices.");return r;}
    for(const auto&v:p.vertices) if(!std::isfinite(v.x)||!std::isfinite(v.y))
        r.errors.push_back("Polygon contains a non-finite coordinate.");
    if(!r.errors.empty())return r;
    for(std::size_t i=0;i<p.vertices.size();++i) {
        const auto&a=p.vertices[i];const auto&b=p.vertices[(i+1)%p.vertices.size()];
        if(distance(a,b)<=eps)r.errors.push_back("Polygon contains a zero-length edge.");
        for(std::size_t j=i+1;j<p.vertices.size();++j) {
            if(j==i || j==(i+1)%p.vertices.size() || i==(j+1)%p.vertices.size())continue;
            const auto&c=p.vertices[j];const auto&d=p.vertices[(j+1)%p.vertices.size()];
            if(segments_intersect(a,b,c,d,eps))r.errors.push_back("Polygon boundary self-intersects.");
        }
    }
    if(polygon_area(p)<=eps*eps)r.errors.push_back("Polygon area must be positive.");
    r.valid=r.errors.empty();return r;
}
bool point_in_polygon(const Polygon2D& p,Point2 q,double eps) {
    bool inside=false;
    for(std::size_t i=0,j=p.vertices.size()-1;i<p.vertices.size();j=i++) {
        const auto&a=p.vertices[j];const auto&b=p.vertices[i];
        if(on_segment(q,a,b,eps))return true;
        if(((a.y>q.y)!=(b.y>q.y)) &&
           q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;
    }
    return inside;
}
double shared_boundary_length(const Polygon2D&a,const Polygon2D&b,double eps) {
    double total=0;
    for(std::size_t i=0;i<a.vertices.size();++i) {
        const auto&p=a.vertices[i];const auto&q=a.vertices[(i+1)%a.vertices.size()];
        const double len=distance(p,q);if(len<=eps)continue;
        const double dx=(q.x-p.x)/len,dy=(q.y-p.y)/len;
        for(std::size_t j=0;j<b.vertices.size();++j) {
            const auto&r=b.vertices[j];const auto&s=b.vertices[(j+1)%b.vertices.size()];
            if(std::abs(cross(p,q,r))>eps*len||std::abs(cross(p,q,s))>eps*len)continue;
            const double rproj=(r.x-p.x)*dx+(r.y-p.y)*dy;
            const double sproj=(s.x-p.x)*dx+(s.y-p.y)*dy;
            const double overlap=std::min(len,std::max(rproj,sproj))-std::max(0.0,std::min(rproj,sproj));
            if(overlap>eps)total+=overlap;
        }
    }
    return total;
}
bool export_polygon_gallery_svg(const std::vector<Polygon2D>& polygons,
 const std::vector<std::string>& labels,const std::string& path,std::string* error) {
    if(polygons.empty()||path.empty()){if(error)*error="Polygons and output path are required.";return false;}
    double minx=std::numeric_limits<double>::max(),miny=minx,maxx=-minx,maxy=-minx;
    for(const auto&p:polygons)for(const auto&v:p.vertices){
        minx=std::min(minx,v.x);miny=std::min(miny,v.y);maxx=std::max(maxx,v.x);maxy=std::max(maxy,v.y);
    }
    if(!std::isfinite(minx)||maxx<=minx||maxy<=miny){if(error)*error="Polygon gallery has invalid bounds.";return false;}
    constexpr double pad=36,scale=70;
    const double width=(maxx-minx)*scale+2*pad,height=(maxy-miny)*scale+2*pad;
    const char* colors[]={"#bfdbfe","#bbf7d0","#fde68a","#fbcfe8","#ddd6fe","#a5f3fc"};
    std::ofstream f(path);
    if(!f){if(error)*error="Could not open SVG output path.";return false;}
    f<<std::fixed<<std::setprecision(3);
    f<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<width<<"\" height=\""<<height
     <<"\" viewBox=\"0 0 "<<width<<" "<<height<<"\">\n<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    for(std::size_t i=0;i<polygons.size();++i){
        const auto&p=polygons[i];const auto check=validate_polygon(p);
        if(!check.valid){if(error)*error="Invalid polygon in gallery: "+check.errors.front();return false;}
        f<<"<polygon points=\"";
        for(const auto&v:p.vertices)f<<(pad+(v.x-minx)*scale)<<","<<(pad+(maxy-v.y)*scale)<<" ";
        f<<"\" fill=\""<<colors[i%6]<<"\" stroke=\"#1e293b\" stroke-width=\"2\"/>\n";
        const auto c=polygon_centroid(p);
        f<<"<text x=\""<<(pad+(c.x-minx)*scale)<<"\" y=\""<<(pad+(maxy-c.y)*scale)
         <<"\" text-anchor=\"middle\" dominant-baseline=\"middle\" font-family=\"Arial\" font-size=\"13\">";
        if(i<labels.size())escape_xml(f,labels[i]);else f<<"Polygon "<<i+1;
        f<<"</text>\n";
    }
    f<<"</svg>\n";
    if(!f.good()){if(error)*error="Failed writing SVG.";return false;}return true;
}
} // namespace mbf
