# 2D Geometry Generation — Technical Plan

**Status:** Approved implementation specification  
**Roadmap target:** Milestone 1  
**Units:** Metres; coordinates are local to the building.

## 1. Purpose

The current core allocates grid cells or rectangles to rooms. It does not yet produce a complete architectural drawing. Before 3D generation, MBF needs one canonical representation of room polygons, boundaries, walls, openings, and circulation.

Both current algorithms must feed the same geometry contract. Downstream systems must not need to know whether a room originated from grid growth or treemap subdivision.

## 2. Coordinate conventions

- X increases along building width; Y increases along building depth.
- Origin (0, 0) is the lower-left corner of the building footprint.
- All lengths are double-precision metres.
- Geometric comparisons use a documented tolerance, never exact floating-point equality.
- Polygon outer rings use counter-clockwise winding; holes use clockwise winding.
- A room boundary is not automatically a wall until boundary classification is performed.

Grid conversion:
- cell_width = building_width / columns
- cell_depth = building_depth / rows
- cell (x, y) occupies [x*cell_width, (x+1)*cell_width] × [y*cell_depth, (y+1)*cell_depth].

Treemap rectangles convert directly to four-corner polygon rings.

## 3. Canonical data model

The first implementation should add engine-independent types with these semantics:

### Point2
X and Y coordinates in metres.

### Segment2
Start/end points, stable segment ID, and optional ownership/classification metadata.

### Ring2 / Polygon2
Ordered ring vertices, outer boundary, optional holes, winding/validity rules, area, and bounds.

### RoomGeometry2D
Stable room ID matching the source RoomRegion, polygon, actual area, centroid, bounds, and future room-function metadata.

### Wall2D
Stable wall ID, baseline segment, thickness, future height metadata, one or two adjacent room IDs, and classification: exterior, shared internal, party wall, or partition.

### Opening2D
Stable opening ID, host wall ID, type (door/window/archway/service), offset along wall, width, optional sill/head metadata, connected room IDs or exterior designation, and validation state.

### CirculationNode / CirculationEdge
Nodes represent entrance, corridor junction, room access, stair, or elevator. Edges represent traversable links with clear-width/accessibility metadata.

### BuildingGeometry2D
Schema/generator versions, footprint polygon, room polygons, classified walls and openings, entrance, circulation graph, diagnostics, and quality metrics.

## 4. Geometry extraction pipeline

### A. Normalize source output
1. Validate source dimensions and cell/rectangle data.
2. Map all coordinates into metres.
3. Remove duplicate points and zero-area regions.
4. Normalize polygon winding.
5. Preserve stable room IDs.

### B. Grid rooms to polygons
1. For each owned cell, emit only edges that do not border another cell of the same room.
2. Stitch exposed edges into closed boundary loops.
3. Resolve corner-touching ambiguity with a deterministic edge-following rule.
4. Merge consecutive collinear edges within tolerance.
5. Validate closure, self-intersection, and non-zero area.
6. Support multiple disjoint loops only if explicitly allowed by the room model; otherwise reject or apply a documented deterministic repair.

### C. Treemap rooms to polygons
1. Convert each rectangle into a four-vertex ring.
2. Normalize winding.
3. Preserve shared coordinates from the partition process where possible.
4. Run the same polygon and adjacency validation as grid rooms.

### D. Shared boundary map
1. Compare room boundaries and identify shared segments.
2. Split segments at T-junctions/intersections so topology is explicit.
3. Merge duplicate coincident segments.
4. Classify each segment as exterior, shared internal, or unclassified.
5. Record adjacent room IDs and exact shared length.
6. Check boundary-length and shared-interface consistency.


## 4.1 Functional room roles and adjacency-aware layout (implemented first slice)

Room specifications may declare a semantic `RoomRole`: `Living`, `Kitchen`, `Bedroom`, `Bathroom`, `Hallway`, generic `Public`, `Private`, `Service`, or `Circulation`. `Auto` infers common roles from English room names; explicit roles take precedence. Unrecognized names remain neutral rather than receiving invented semantics.

The current rectangular layout planner searches deterministic guillotine-partition candidates and scores them using:
- preferred living–kitchen and living–hallway adjacency;
- hallway access to bedrooms and bathrooms;
- a preference for bedrooms on the building perimeter;
- a penalty for direct living–bathroom adjacency;
- living/hallway centrality and room aspect-ratio quality.

For small briefs (up to seven rooms), the planner evaluates permutations and cut orientations exhaustively. Larger briefs use a bounded deterministic seeded candidate search. The selected result preserves room IDs and returns rooms in input order.

These are configurable design heuristics, not universal architectural rules. They are a starting residential profile; future building typologies must supply their own role and adjacency profiles. Adjacency is not yet a door/access graph: a shared boundary does not by itself prove usable circulation. No building-code or structural compliance is implied.

## 5. Walls, doors, windows, and circulation

### Walls
- Every classified boundary becomes a wall candidate unless a rule marks it as an open circulation boundary.
- Shared walls reference both adjacent rooms; exterior walls reference one room and the exterior.
- Thickness is metadata on the 2D baseline, not baked into it.
- Resolve wall junctions globally; do not extrude each room independently and create overlapping corners.

### Doors
- Doors may be proposed only on shared room boundaries or designated exterior entrance boundaries.
- Width must fit the host segment with required end clearances.
- Doors connect two valid spaces, or a room and the exterior.
- Update the circulation graph only after a door candidate passes validation.
- Room adjacency alone does not imply a traversable doorway.

### Windows
- Windows may be proposed only on exterior walls unless an explicit interior glazing rule exists.
- Window width, sill height, head height, and spacing are constraints.
- Never place windows on party walls or room-to-room shared boundaries.

### Corridors and entrance
- The entrance is an explicit point/segment on the exterior footprint.
- Corridors are explicit circulation regions, not merely leftover space.
- Every required habitable room must be reachable from an entrance through valid door/corridor edges.
- Minimum clear widths are configurable by building profile; no single code requirement is treated as universal.

## 6. Validation and diagnostics

The pipeline reports structured diagnostics, not only a boolean.

Required checks:
- finite coordinates and positive footprint dimensions;
- closed, non-self-intersecting polygon rings;
- correct winding and positive room area;
- room polygons contained by the footprint;
- no unintended room overlaps or uncovered interior gaps;
- consistent shared boundaries and wall ownership;
- no zero-length or duplicate wall segments;
- doors hosted by valid walls and within wall extents;
- windows hosted on exterior walls;
- access graph connectivity for required rooms;
- minimum clear widths and configured room dimensions.

Each diagnostic contains a stable code, severity, affected entity IDs, and a human-readable explanation.

## 7. Debug and interchange output

Before 3D work, produce a visual 2D output that can be reviewed without Unreal Engine.

Preferred first output: SVG, because it is text-based, diff-friendly, and viewable in a browser. It should show:
- footprint and room fills/labels;
- internal and exterior walls;
- doors and windows;
- entrance and circulation paths;
- invalid geometry and diagnostic labels.

Also provide a versioned machine-readable format for round-trip tests. JSON is a suitable first candidate.

## 8. Testing strategy

Fixtures must cover one/many rooms, rectangles, concave and L-shaped rooms, long/short shared edges, T/four-way junctions, non-square cells, corner-touching rooms (not adjacency), too-narrow door hosts, windows on internal walls, disconnected rooms, and missing entrances.

Property tests should verify area conservation, no unintended overlap, footprint containment, and stable topology across repeated runs.

## 9. Implementation sequence

1. Add geometry primitives and tolerance policy.
2. Add grid boundary extraction and polygon stitching.
3. Add treemap-to-polygon conversion.
4. Add shared-boundary splitting, merging, and classification.
5. Add wall records and exact shared lengths.
6. Add entrance, corridor, door, and window constraints.
7. Add structured validation diagnostics.
8. Add SVG debug export and regression fixtures.

## 10. Definition of done

Milestone 1 is complete only when both algorithms produce the same canonical geometry types; polygon validity and footprint checks pass; overlaps/gaps are detected; shared walls are represented once with correct room IDs; doors/windows obey host-wall constraints; required rooms are reachable; SVG output is inspectable; and deterministic automated tests pass.

## 11. Non-goals

This milestone does not implement 3D meshes, furniture, final building-code compliance, Unreal actors, or multi-floor stairs/elevators. The 3D stage consumes the validated output of this document.
