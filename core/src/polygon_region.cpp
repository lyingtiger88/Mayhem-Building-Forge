#include "mbf/polygon_region.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mbf {
namespace {
double perimeter(const Polygon2D& p) {
    double total=0.0;
    for(std::size_t i=0;i<p.vertices.size();++i) {
        const auto& a=p.vertices[i];
        const auto& b=p.vertices[(i+1U)%p.vertices.size()];
        total+=std::hypot(b.x-a.x,b.y-a.y);
    }
    return total;
}
bool contained(const Polygon2D& inner,const Polygon2D& outer,double area_tol,double tol) {
    const double intersection=polygon_intersection_area(inner,outer,tol);
    return std::isfinite(intersection)&&std::abs(intersection-polygon_area(inner))<=area_tol;
}
}

PolygonPartitionValidation validate_polygon_partition(
        const PolygonRegion2D& region,const std::vector<Polygon2D>& parts,double tolerance) {
    PolygonPartitionValidation result;
    const auto fail=[&](const std::string& message){result.errors.push_back(message);};
    if(!std::isfinite(tolerance)||tolerance<=0.0) {
        fail("Partition tolerance must be finite and positive.");
        return result;
    }
    const auto outer_check=validate_polygon(region.outer_boundary,tolerance);
    if(!outer_check.valid) {
        for(const auto& error:outer_check.errors)fail("Outer boundary: "+error);
        return result;
    }
    if(parts.empty()) {
        fail("A polygon partition must contain at least one part.");
        return result;
    }
    const double outer_area=polygon_area(region.outer_boundary);
    const double area_tolerance=std::max(tolerance*tolerance,
        std::max(outer_area,1.0)*tolerance*10.0);
    double holes_area=0.0;
    for(std::size_t i=0;i<region.holes.size();++i) {
        const auto& hole=region.holes[i];
        const auto check=validate_polygon(hole,tolerance);
        if(!check.valid) {
            for(const auto& error:check.errors)
                fail("Hole "+std::to_string(i)+": "+error);
            continue;
        }
        if(!contained(hole,region.outer_boundary,area_tolerance,tolerance))
            fail("Hole "+std::to_string(i)+" is not fully contained by the outer boundary.");
        if(shared_boundary_length(hole,region.outer_boundary,tolerance)>tolerance)
            fail("Hole "+std::to_string(i)+" touches the outer boundary.");
        holes_area+=polygon_area(hole);
        for(std::size_t j=0;j<i;++j) {
            const double overlap=polygon_intersection_area(hole,region.holes[j],tolerance);
            if(!std::isfinite(overlap)||overlap>area_tolerance)
                fail("Interior holes overlap or cannot be robustly compared.");
            if(shared_boundary_length(hole,region.holes[j],tolerance)>tolerance)
                fail("Interior holes share a boundary.");
        }
    }
    result.region_area_m2=outer_area-holes_area;
    if(result.region_area_m2<=area_tolerance) {
        fail("Net region area must be positive.");
        return result;
    }

    double parts_area=0.0;
    for(std::size_t i=0;i<parts.size();++i) {
        const auto& part=parts[i];
        const auto check=validate_polygon(part,tolerance);
        if(!check.valid) {
            for(const auto& error:check.errors)
                fail("Part "+std::to_string(i)+": "+error);
            continue;
        }
        const double area=polygon_area(part);
        parts_area+=area;
        if(!contained(part,region.outer_boundary,area_tolerance,tolerance))
            fail("Part "+std::to_string(i)+" extends outside the outer boundary.");
        for(std::size_t h=0;h<region.holes.size();++h) {
            const double overlap=polygon_intersection_area(part,region.holes[h],tolerance);
            if(!std::isfinite(overlap)||overlap>area_tolerance)
                fail("Part "+std::to_string(i)+" overlaps interior hole "+std::to_string(h)+".");
        }
        for(std::size_t j=0;j<i;++j) {
            const double overlap=polygon_intersection_area(part,parts[j],tolerance);
            if(!std::isfinite(overlap))
                fail("Could not compare parts "+std::to_string(i)+" and "+std::to_string(j)+".");
            else if(overlap>area_tolerance)
                fail("Parts "+std::to_string(i)+" and "+std::to_string(j)+" overlap.");
        }
    }
    result.covered_area_m2=parts_area;
    if(std::abs(parts_area-result.region_area_m2)>area_tolerance)
        fail("Partition has a gap or excess area: summed part area differs from net region area.");
    result.valid=result.errors.empty();
    return result;
}
} // namespace mbf
