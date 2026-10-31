#include "mbf/polygon_offset.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mbf {
namespace {
struct Vec { double x{}, y{}; };
Vec subtract(Point2 a, Point2 b) { return {a.x-b.x,a.y-b.y}; }
double cross(Vec a, Vec b) { return a.x*b.y-a.y*b.x; }
Point2 add(Point2 p, Vec v) { return {p.x+v.x,p.y+v.y}; }
bool line_intersection(Point2 p, Vec d, Point2 q, Vec e,
                       double tolerance, Point2& out) {
    const double denominator=cross(d,e);
    if(std::abs(denominator)<=tolerance)return false;
    const Vec qp=subtract(q,p);
    const double t=cross(qp,e)/denominator;
    out=add(p,{d.x*t,d.y*t});
    return std::isfinite(out.x)&&std::isfinite(out.y);
}
}

double point_segment_distance(Point2 p,Point2 a,Point2 b) {
    const double dx=b.x-a.x,dy=b.y-a.y;
    const double length2=dx*dx+dy*dy;
    if(length2<=1e-24)return std::hypot(p.x-a.x,p.y-a.y);
    const double t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/length2,0.0,1.0);
    return std::hypot(p.x-(a.x+t*dx),p.y-(a.y+t*dy));
}

PolygonOffsetResult offset_polygon(const Polygon2D& input,double offset,
                                   double miter_limit,double tolerance) {
    PolygonOffsetResult result;
    result.requested_offset_m=offset;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(offset)||!std::isfinite(miter_limit)||miter_limit<1.0||
       !std::isfinite(tolerance)||tolerance<=0.0) {
        fail("Offset, miter limit, and tolerance must be finite; miter limit must be at least 1 and tolerance positive.");
        return result;
    }
    const auto check=validate_polygon(input,tolerance);
    if(!check.valid) {
        for(const auto& error:check.errors)fail("Input polygon: "+error);
        return result;
    }
    if(std::abs(offset)<=tolerance) {
        result.polygon=input;result.valid=true;return result;
    }
    const double orientation=polygon_signed_area(input)>=0.0?1.0:-1.0;
    const std::size_t count=input.vertices.size();
    std::vector<Vec> directions(count),normals(count);
    for(std::size_t i=0;i<count;++i) {
        const auto a=input.vertices[i];
        const auto b=input.vertices[(i+1U)%count];
        const double length=std::hypot(b.x-a.x,b.y-a.y);
        if(length<=tolerance){fail("Input polygon contains a degenerate edge.");return result;}
        directions[i]={(b.x-a.x)/length,(b.y-a.y)/length};
        // For CCW boundaries, outward is to the right; reverse for CW.
        normals[i]={orientation*directions[i].y,-orientation*directions[i].x};
    }
    result.polygon.vertices.reserve(count);
    for(std::size_t i=0;i<count;++i) {
        const std::size_t previous=(i+count-1U)%count;
        const Point2 vertex=input.vertices[i];
        const Point2 p=add(vertex,{normals[previous].x*offset,normals[previous].y*offset});
        const Point2 q=add(vertex,{normals[i].x*offset,normals[i].y*offset});
        Point2 intersection{};
        if(!line_intersection(p,directions[previous],q,directions[i],
                              tolerance,intersection)) {
            fail("Adjacent offset edges are parallel or numerically unstable.");
            return result;
        }
        const double miter=std::hypot(intersection.x-vertex.x,intersection.y-vertex.y);
        if(miter>std::abs(offset)*miter_limit+tolerance) {
            fail("Offset corner exceeds the configured miter limit.");
            return result;
        }
        result.polygon.vertices.push_back(intersection);
    }
    const auto output_check=validate_polygon(result.polygon,tolerance);
    if(!output_check.valid) {
        for(const auto& error:output_check.errors)fail("Offset result: "+error);
        return result;
    }
    if(offset<0.0) {
        if(polygon_area(result.polygon)>=polygon_area(input)-tolerance) {
            fail("Inward offset did not reduce polygon area; the requested inset may have collapsed.");
            return result;
        }
        for(const auto& vertex:result.polygon.vertices) {
            if(!point_in_polygon(input,vertex,tolerance)) {
                fail("Inward offset crossed outside the source polygon; the requested inset has collapsed.");
                return result;
            }
            double nearest=std::numeric_limits<double>::max();
            for(std::size_t i=0;i<input.vertices.size();++i)
                nearest=std::min(nearest,point_segment_distance(vertex,input.vertices[i],
                    input.vertices[(i+1U)%input.vertices.size()]));
            if(nearest+ tolerance < std::abs(offset)) {
                fail("Inward offset does not preserve the requested boundary clearance; the inset has collapsed.");
                return result;
            }
        }
    }
    if(offset>0.0 && polygon_area(result.polygon)<=polygon_area(input)+tolerance) {
        fail("Outward offset did not increase polygon area.");
        return result;
    }
    result.valid=true;
    return result;
}

BuildableAreaResult compute_buildable_area(const Polygon2D& lot,double setback,
                                            double tolerance) {
    BuildableAreaResult result;
    result.lot_boundary=lot;
    result.setback_m=setback;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(setback)||setback<0.0) {
        fail("Setback must be finite and non-negative.");
        return result;
    }
    const auto lot_check=validate_polygon(lot,tolerance);
    if(!lot_check.valid) {
        for(const auto& error:lot_check.errors)fail("Lot boundary: "+error);
        return result;
    }
    result.lot_area_m2=polygon_area(lot);
    if(setback<=tolerance) {
        result.buildable_boundary=lot;
        result.buildable_area_m2=result.lot_area_m2;
        result.valid=true;
        return result;
    }
    const auto inset=offset_polygon(lot,-setback,4.0,tolerance);
    if(!inset.valid) {
        for(const auto& error:inset.errors)fail("Buildable area: "+error);
        return result;
    }
    result.buildable_boundary=inset.polygon;
    result.buildable_area_m2=polygon_area(inset.polygon);
    result.valid=true;
    return result;
}
} // namespace mbf
