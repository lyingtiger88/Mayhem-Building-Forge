# Mayhem Building Forge — Product Roadmap

This roadmap distinguishes implemented code from planned capabilities. A feature is not complete merely because its data type or interface exists: each milestone has testable exit criteria.

**Status legend:** Done · In progress · Planned · Future

## Product direction

- Both a standalone desktop authoring application and Unreal Engine integration.
- Complete building interiors, not exterior shells only.
- Hybrid generation: architectural rules, geometric algorithms, and curated asset libraries.
- Large-environment readiness through offline baking, instancing, LOD/HLOD, and streaming.
- Multiplayer is deferred until single-player generation and the building model are stable.

## Milestone 0 — Repository and core foundation
**Status: Implemented (initial baseline)**

- [x] C++20 engine-independent core.
- [x] CMake project and sample executable.
- [x] Deterministic seed input.
- [x] Grid-growth floor-plan generator.
- [x] Weighted rectangular / treemap generator.
- [x] Room regions, areas, bounds, cell ownership, and adjacency output.
- [x] Basic validation and regression tests.
- [x] GitHub Actions build-and-test workflow.
- [x] Research reference analysis and attribution.
- [x] README, architecture, getting-started, user guide, and roadmap.

**Exit criteria:** Core builds in CI; fixed inputs produce repeatable results; sample and regression tests run. This does not certify architectural correctness.

## Milestone 1 — 2D architectural geometry
**Status: In progress — first polygon and SVG slice implemented**

This milestone turns abstract room allocations into an explicit, inspectable 2D architectural drawing. It is the prerequisite for reliable 3D extrusion.

- [ ] Canonical 2D coordinate system in metres, with explicit origin and axis conventions.
- [ ] Polygon, ring, line-segment, and opening data types in the engine-neutral model.
- [x] Convert grid-grown rooms into clean polygon boundaries.
- [x] Convert treemap rectangles into the same canonical polygon representation.
- [ ] Merge collinear edges and remove duplicate or zero-length segments.
- [ ] Classify boundaries as exterior walls, shared internal walls, or open circulation boundaries.
- [ ] Calculate shared wall lengths and preserve room-to-wall relationships.
- [ ] Define building entrance, corridors, room access, and circulation graph.
- [ ] Generate door candidates on valid shared boundaries and validate clear widths.
- [ ] Generate window candidates only on exterior boundaries.
- [ ] Enforce configurable room minimum area, width, depth, and aspect-ratio constraints.
- [ ] Validate footprint coverage, polygon validity, non-overlap, connectivity, and openings.
- [x] Export a human-readable 2D debug drawing (SVG; first visual-debug version).
- [ ] Add deterministic and property-based tests for geometry edge cases.

**Exit criteria:** Both algorithms produce the same canonical 2D geometry contract. Results have valid room polygons, classified walls, consistent adjacency, and connected access graphs; invalid layouts return actionable diagnostics. A plan can be inspected without Unreal Engine.

Current implementation includes room polygon extraction for grid and treemap plans, area-conservation checks, and SVG export. Wall classification, openings, circulation, and stronger topology validation remain planned. Detailed specification: [2D Geometry Plan](docs/2D_GEOMETRY_PLAN.md).

## Milestone 2 — Multi-floor building model
**Status: Planned**

- [ ] Building envelope and floor stack.
- [ ] Floor-to-floor heights and slab thickness.
- [ ] Stair and elevator core continuity.
- [ ] Vertical alignment of structural elements.
- [ ] Inter-floor circulation validation.
- [ ] Building-level data contract and versioning.

**Exit criteria:** Multiple floors have aligned floor plates and valid vertical circulation. Each floor uses the canonical 2D geometry model from Milestone 1.

## Milestone 3 — 3D structural geometry
**Status: Planned**

- [ ] Extrude 2D wall segments into wall meshes with correct thickness and junction handling.
- [ ] Generate floor slabs, ceilings, roofs, columns, and structural supports.
- [ ] Create real door and window openings in wall geometry.
- [ ] Generate reveals, lintels, sills, and frame attachment anchors.
- [ ] Produce consistent normals, UVs, material slots, and collision geometry.
- [ ] Preserve room, wall, floor, and opening IDs in mesh metadata.
- [ ] Validate mesh indices, degenerate triangles, normals, bounds, and opening clearances.
- [ ] Keep render mesh, collision mesh, and navigation metadata separate.
- [ ] Add deterministic mesh-generation tests and reference fixtures.

**Exit criteria:** A validated 2D plan can be extruded into a structurally coherent 3D building shell with real openings, stable IDs, and tested geometry. The geometry core has no Unreal dependency.

Detailed specification: [3D Geometry Plan](docs/3D_GEOMETRY_PLAN.md).

## Milestone 4 — Facades and architectural styles
**Status: Planned**

- [ ] Facade grammar and style profiles.
- [ ] Window, balcony, entrance, roofline, and material composition.
- [ ] Style constraints by typology and floor height.
- [ ] Asset-library metadata and compatibility rules.
- [ ] Variation controls that preserve structural validity.

**Exit criteria:** Distinct facade variants can be generated without breaking structural geometry.

## Milestone 5 — Complete interior generation
**Status: Planned**

- [ ] Room function profiles and furnishing templates.
- [ ] Furniture footprints, clearance envelopes, and circulation paths.
- [ ] Kitchens, bathrooms, bedrooms, offices, retail, and shared spaces.
- [ ] Asset placement, rotation, scale, and collision checks.
- [ ] Lighting and navigation-ready metadata.
- [ ] Interior quality scoring and manual overrides.

**Exit criteria:** Supported room types can be furnished with usable circulation and no critical object collisions.

## Milestone 6 — Desktop authoring application
**Status: Planned**

- [ ] Project browser and building brief editor.
- [ ] 2D floor-plan preview and room editing.
- [ ] 3D preview, camera controls, and generation variants.
- [ ] Parameter panels, seed controls, and validation diagnostics.
- [ ] Asset-library browser and export settings.
- [ ] Save/load projects and version migration.

**Exit criteria:** A user can create, inspect, edit, regenerate, and export a building without editing source code.

## Milestone 7 — Unreal Engine integration
**Status: Planned**

- [ ] Unreal plugin module and supported engine-version policy.
- [ ] Import/bake workflow for generated building data.
- [ ] Instanced props and material mapping.
- [ ] Collision, navigation, LOD/HLOD, and Nanite suitability checks.
- [ ] Runtime generation API with explicit budgets.
- [ ] World Partition and streaming integration.
- [ ] Example Unreal project and packaged demo.

**Exit criteria:** A generated building can be imported, baked, placed, streamed, and explored in a documented Unreal sample project.

## Milestone 8 — Large-world production readiness
**Status: Future**

- [ ] Batch city/block generation.
- [ ] Asynchronous generation jobs and caching.
- [ ] Memory and frame-time budgets.
- [ ] Asset deduplication and instancing.
- [ ] Profiling benchmarks and stress tests.
- [ ] Save-state and regeneration compatibility.

**Exit criteria:** Benchmark scenes meet documented performance and memory targets.

## Milestone 9 — Multiplayer and collaboration
**Status: Future / deferred**

- [ ] Replication based on generator version, seed, and state deltas.
- [ ] Collaborative building authoring.
- [ ] Conflict resolution and deterministic regeneration.
- [ ] Network performance and authority model.

Multiplayer is intentionally not an early milestone. It should be designed after determinism and the building data model stabilize.

## Current execution order

1. Complete Milestone 1: canonical 2D geometry, walls, openings, circulation, and SVG/debug output.
2. Complete Milestone 2: multi-floor model and vertical circulation.
3. Complete Milestone 3: 3D structural geometry generated from the validated 2D model.
4. Continue with facades and complete interiors.

Do not jump to 3D meshes before the 2D geometry contract and validation are reliable.
