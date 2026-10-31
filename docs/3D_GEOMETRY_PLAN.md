# 3D Geometry Generation — Technical Plan

**Status:** Planned design specification  
**Roadmap target:** Milestone 3  
**Prerequisites:** Validated canonical 2D geometry (Milestone 1) and multi-floor building model (Milestone 2).

## 1. Purpose and boundary

The 3D generator consumes a validated building model and creates structural geometry: walls, slabs, ceilings, roofs, columns, and opening geometry. It must not depend on Unreal Engine, Blender, or a desktop renderer.

The Unreal adapter later translates engine-neutral mesh data into engine assets and actors. The core remains responsible for geometry correctness and stable metadata.

## 2. Coordinate system and units

- X and Y follow the 2D plan coordinate convention; Z increases upward.
- All core positions and dimensions are in metres.
- Each floor has explicit elevation and floor-to-floor height.
- Conversion to centimetres or engine units occurs only in an adapter.
- Mesh winding, front-face convention, and normal direction must be documented and tested.

## 3. Inputs

The generator requires validated building/floor geometry, wall baselines and thicknesses, wall height profiles, slab boundaries and thickness, door/window opening definitions, structural support definitions, material/surface tags, generator version, and deterministic settings.

Do not generate production meshes directly from raw room rectangles or unvalidated grid cells.

## 4. Engine-neutral mesh contract

A mesh section should contain:
- vertices in local metres;
- triangle indices;
- per-vertex normals;
- UV coordinates and optional tangents;
- material-slot identifiers;
- stable source entity IDs (wall, floor, opening, or building);
- collision classification or a separate collision mesh.

Large surfaces should be split into manageable sections with stable local origins. Geometry identity must survive export and regeneration.

## 5. Structural generation pipeline

### Walls
1. Offset wall baselines according to thickness and junction policy.
2. Resolve intersections and corners globally, not room by room.
3. Subtract door, window, and service openings from wall regions.
4. Generate front/back faces, side returns, reveals, lintels, and sills as required.
5. Assign material regions and UVs.
6. Preserve wall and opening IDs.

### Floors and ceilings
1. Generate slab polygons from floor plate boundaries.
2. Apply slab thickness and top/bottom surfaces.
3. Resolve holes for stairs, elevators, shafts, and service risers.
4. Generate ceilings separately where required.

### Roofs
- Support flat roofs first.
- Reserve pitched and complex roof profiles for later extensions.
- Close the building envelope where intended and preserve roof metadata.

### Columns and structural supports
- Place supports from explicit rules or validated anchor points.
- Avoid placing columns through doors, circulation paths, or required usable room areas.
- Automatic generation does not certify structural engineering compliance.

### Openings
- Doors and windows must be actual voids in host wall geometry.
- Frames, leaves, handles, glass, and trim are separate asset/mesh layers.
- Opening dimensions remain consistent with their 2D host-wall records.

## 6. Mesh validation

Required checks:
- all indices are in range;
- no NaN or infinite vertex coordinates;
- no zero-area triangles;
- normals are finite and consistently oriented;
- no unintended duplicate or inverted faces;
- bounds are finite and match the source entity;
- openings do not leave accidental wall caps;
- wall junctions have no unintended gaps or overlapping internal faces;
- collision geometry is simpler but spatially consistent with render geometry;
- mesh sections preserve source IDs and material slots.

Check watertightness for components intended to be closed solids; do not impose it blindly on every surface mesh.

## 7. Performance and output

- Keep structural meshes separate from decorative assets.
- Merge static geometry by sensible material/streaming chunk, not into one unmanageable building mesh.
- Reuse repeated modules and instances.
- Keep collision meshes separate from render meshes.
- Generate LOD/HLOD and Nanite-compatible output in the engine adapter or offline bake stage.
- Keep each floor/interior streaming chunk independently addressable.
- Cache by building input hash, geometry schema version, generator version, and asset-library version.

## 8. Test fixtures

Build deterministic fixtures for one rectangular room; two rooms with one shared wall and a door; exterior wall with a window; L-shaped floor plan; T/corner junctions; corridor floor; slab with stair opening; and two stacked floors with aligned vertical core.

Compare mesh invariants and bounds rather than relying only on screenshots.

## 9. Implementation sequence

1. Define 3D vector, mesh, section, and metadata types.
2. Implement wall extrusion without openings.
3. Implement global wall-junction resolution.
4. Add door/window opening generation.
5. Generate slabs, ceilings, and simple flat roof.
6. Add material regions, UVs, and collision representation.
7. Add mesh diagnostics and deterministic tests.
8. Add interchange export and Unreal adapter only after core fixtures pass.

## 10. Definition of done

Milestone 3 is complete when validated 2D geometry produces deterministic structural meshes with correct dimensions, openings, wall junctions, material regions, collision metadata, and source IDs; regression fixtures pass; and no engine-specific dependency enters the core.

## 11. Non-goals

This milestone does not provide final facade styling, furniture, lighting design, structural engineering certification, Unreal editor UI/runtime plugin, or city-scale streaming.
