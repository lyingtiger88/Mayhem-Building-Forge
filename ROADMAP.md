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
**Status: In progress — substantial geometry primitives exist; a limited residential generator and native-engine graphical test bench are available. The general-purpose residential planner is not complete.**

This milestone turns a user brief into multiple architecturally credible, inspectable 2D floor-plan candidates. It is the prerequisite for reliable 3D extrusion. The current fixed five-space residential layout is only a prototype and does not satisfy this milestone's exit criteria.

### Milestone 1 execution sequence
1. **M1.1 Generative topology engine:** arbitrary room schedules and genuinely different adjacency/circulation graphs. Initial graph-candidate generation, validation, and first geometric realization are implemented and connected to the Parametric Lab; arbitrary room schedules remain open.
2. **M1.2 Generalized geometric solver:** solve room polygons from topology and dimensions. An initial rectangular guillotine solver now realizes supported graph candidates; polygonal/non-rectangular realization and broader schedules remain open.
3. **M1.3 Architectural intelligence:** daylight, privacy, access, circulation, and room-function scoring. Initial normalized daylight-potential, adjacency-privacy, and circulation scores are implemented; actual sun/daylight simulation and code-based access checks remain open.
4. **M1.4 Walls and openings:** consistent walls, doors, windows, and navigable circulation. The topology-driven export now creates role-aware openings and validates reachability; robust door swings, wall thickness, and junction rules remain open.
5. **M1.5 Candidate optimization:** initial candidate ranking by normalized spatial quality is implemented; broader multi-objective optimization and geometric de-duplication remain open.
6. **M1.6 Visual validation:** a local graphical test bench now imports/exports briefs, downloads SVG plans, and compares ranked candidates; live editing, project persistence, and stress testing remain open.

The local browser-based Parametric Lab is an implemented developer test bench, not the finished desktop authoring application. It invokes the native C++ executable and must remain synchronized with its supported inputs.

- [x] Add first standalone polygon primitives: regular polygons, rectangles, concave polygons, area, centroid, containment, self-intersection validation, and shared-boundary length.
- [x] Add a first balanced residential zoning prototype using explicit room polygons, a south entrance, public/private bands, a circulation spine, and modeled openings.
- [x] Canonical 2D coordinate frame: metres, south-west origin, +X east, +Y north.
- [x] Polygon, ring, point, and boundary-segment data types in the engine-neutral model.
- [x] Convert grid-grown rooms into clean polygon boundaries.
- [x] Convert treemap rectangles into the same canonical polygon representation.
- [x] Normalize collinear room edges, split partial overlaps at shared endpoints, deduplicate shared wall pieces, merge compatible adjacent segments, and discard zero-length edges. (Further grid-growth edge-case coverage remains pending.)
- [x] Classify exact matching room boundaries as exterior walls or shared internal walls.
- [x] Split wall segments around modeled door/window openings and retain explicit opening-gap records.
- [x] Represent non-door open-plan connections separately from hinged interior doors in the wall-opening model.
- [ ] Add robust junction resolution for overlapping multi-room boundaries and non-manifold cases.
- [x] Calculate boundary lengths and preserve room-to-room ownership for exact shared edges.
- [x] Build a deterministic wall-topology graph with unique junction nodes and wall connectivity.
- [x] Classify degree-2 straight/corner nodes and degree-3 T-junctions, degree-4 cross-junctions, endpoints, and complex junctions.
- [x] Build a room-access graph from entrance, interior-door, and open-plan openings; report rooms unreachable from an exterior entrance.
- [x] Validate access-opening clear widths and enforce entrance-door placement on exterior walls.
- [x] Validate minimum clear width of hallway zones and construct per-side door approach clearance envelopes; reject approaches that extend outside the served room.
- [x] Add first-pass procedural 2D furniture symbols to SVG output by room role.
- [x] Add typed furniture entities with stable IDs, room ownership, dimensions, position, rotation, mobility, and collision flags.
- [x] Add deterministic room-aware furniture placement with polygon-boundary and inter-object clearance validation.
- [x] Render furniture entities as individually identifiable SVG groups with editable transform metadata.
- [x] Add local GUI editing: select, drag, rotate by 90 degrees, reset generated positions, and export edited SVG.
- [ ] Persist edited furniture layouts back into versioned Building Brief/project data and the native generator.
- [ ] Validate furniture against door swings, window access, and continuous circulation paths; add turning-space checks.
- [x] Add a first residential topology profile with hard adjacency constraints, a south-side entrance, and preliminary room-to-room access links.
- [x] Add semantic door/window opening records with stable IDs, dimensions, sill heights, and room ownership.
- [x] Validate door/open-plan openings on shared walls and windows on exterior walls only.
- [x] Reject openings outside wall-end clearances or overlapping existing openings.
- [x] Split wall centerlines into solid segments and explicit opening gaps.
- [x] Generate deterministic default door/window candidates from classified wall topology.
- [x] Connect opening records to room-access reachability and entrance validation.
- [ ] Optimize door placement against room function, privacy, and direct entrance sightlines.
- [x] Enforce configurable room minimum area, width, depth, and aspect-ratio constraints.
- [x] Add semantic room roles and deterministic adjacency-aware layout scoring for common residential room types.
- [x] Validate footprint coverage, polygon validity, non-overlap, room access connectivity, and opening boundary/width constraints.
- [x] Add architectural quality scoring and warnings for rooms close to configured minimums.
- [x] Export a human-readable 2D debug drawing (SVG; first visual-debug version).
- [x] Add deterministic regression tests for rectangular partition area, bounds, aspect ratio, and overlap.
- [x] Add deterministic regression sweeps for 100 rectangle intersection cases, concave containment, edge touching, overlap, and missing footprint coverage.
- [ ] Add larger property-based and fuzz testing across pathological polygons and floating-point tolerances.
- [x] Replace axis-aligned bounding-box overlap checks with triangulation-based positive-area intersection tests for valid simple concave/convex room polygons; check coverage by no-overlap plus summed area inside a rectangular envelope.
- [x] Add generic polygon-region contract with a simple concave outer boundary and zero or more interior holes.
- [x] Validate room partitions against L-shaped and U-shaped envelopes, courtyard voids, outside parts, overlap, and uncovered area.
- [x] Add miter-based polygon offset/inset operations with winding support, output validation, and configurable miter limits.
- [x] Add uniform lot setback calculation and buildable-area reporting for valid simple polygon lots.
- [ ] Add robust concave offset cleanup, round/bevel joins, and per-edge/front-side setbacks; support courtyard wall offsets and narrow-neck collapse handling.
- [x] Add three deterministic residential profile variants for the initial five-space prototype; current variants alter zoning ratios but retain the same room graph/topology.
- [x] Add a local browser-based Parametric Lab that invokes the native C++ generator, previews SVG candidates, compares current profiles, and reports diagnostics.
- [x] Add bedroom/bathroom count controls, entrance-side selection, JSON brief import/export, ranked candidate comparison, quality metrics, and SVG downloads.
- [ ] Upgrade the Parametric Lab into a full authoring interface with arbitrary room schedules, live plan editing, and persistent project management.
- [x] Add an engine-neutral room-topology candidate generator with circulation-hub, public/private-spine, and compact-linear graph strategies; validate required/avoided adjacency and graph connectivity. (Graph layer only; not yet converted into floor-plan geometry.)
- [ ] Expand profiles into geometrically realized, genuinely different room graphs, including optional vestibules and alternative corridor topologies.
- [x] Add initial normalized daylight-potential, privacy, circulation, and overall scores; rank topology-derived candidates by overall score.
- [ ] Improve quality scoring with solar exposure, entrance sightlines, room-level adjacency strength, and detailed circulation efficiency.
- [x] Add a command-line parametric house-generation workflow accepting footprint dimensions, floor count, floor height, profile, seed, and SVG output path.
- [x] Validate generated candidates and physical circulation before selecting/exporting a plan.
- [ ] Expand the building brief UI/CLI to arbitrary room types, per-room target areas, priorities, and user-authored adjacency/exclusion rules. The current GUI supports 1–4 bedrooms and 1–4 bathrooms in the default residential schedule.
- [x] Expose rectangular site dimensions and four-side setbacks in the CLI and Parametric Lab.
- [x] Add configurable entrance facade (south/north/east/west) to the topology-driven brief.
- [ ] Add arbitrary polygonal plots, site orientation, accessibility targets, and local rule profiles to the user-facing brief.
- [x] Add an initial topology-constrained rectangular partition solver that realizes required graph adjacencies, respects avoided role pairs, and enforces per-room minimum area and dimensions.
- [ ] Generalize topology-to-geometry solving beyond rectangular guillotine partitions and the current default residential brief.

**Exit criteria:** A user-defined room schedule and site brief produce multiple genuinely different candidates. Every accepted candidate has valid room polygons, correct wall/opening ownership, connected and sufficiently wide circulation, no overlap/gaps, and documented diagnostics. Candidate scoring includes room dimensions, adjacency, privacy, daylight potential, and circulation. The Parametric Lab can inspect and compare candidates. Automated tests cover supported typologies and pathological geometry. No 3D milestone begins until these criteria are met.

The polygon engine and region validator provide reusable geometry foundations. The legacy profile generator remains a fixed five-space, one-bedroom prototype. Alongside it, the topology-driven generator now supports 1–4 bedrooms and 1–4 bathrooms, three distinct adjacency-graph strategies, a first rectangular geometry solver, facade-aware entrance placement, role-aware doors/windows, room-access validation, and heuristic daylight/privacy/circulation scoring. The Parametric Lab invokes these native executables and displays ranked SVG candidates. Arbitrary room schedules, non-rectangular geometric realization, physical daylight simulation, door swings, structural validation, and full desktop authoring remain open. Grid-growth polygon extraction remains available for internal experiments. Detailed specification: [2D Geometry Plan](docs/2D_GEOMETRY_PLAN.md).

## Milestone 2 — Multi-floor building model
**Status: In progress — initial engine-neutral floor-stack contract and vertical-core validation implemented**

- [x] Add a versioned building-level data contract containing ordered floor records and vertical-core definitions.
- [x] Generate deterministic repeated floor stacks from a validated canonical 2D residential plan.
- [x] Store per-floor elevation, floor-to-floor height, slab thickness, and independent 2D layout data.
- [x] Validate consecutive floor indices, elevation progression, floor-plate alignment, slab limits, and each floor's 2D plan.
- [x] Model stair/elevator core footprints with stable IDs and per-floor room mappings; validate containment and non-overlap.
- [ ] Generate and validate actual stair flights, landings, elevator door openings, and inter-floor traversable links.
- [ ] Support floor-specific plan variations while preserving structural grid and shaft alignment.
- [ ] Validate vertical structural elements (columns, shear walls, load-bearing walls) across floor transitions.
- [ ] Add multi-floor serialization, schema migration, and reference fixtures.

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

## Dynamic interior physics and destructibility requirement (accepted architecture decision, 2026-10-31)

All environment elements within the player's active vicinity must support the dynamic behavior appropriate to their type. This includes building parts and fixtures, interior furnishings, appliances, movable props, vehicles, and relevant nearby environmental objects. Interior furnishings and interactable props must be independent entities—not permanently baked into a room mesh or locked to their authored transform. The player-vicinity rule is the activation boundary for dynamic simulation; it is not a city-wide always-on simulation requirement.

Required capabilities:
- Every prop has a stable semantic ID, asset reference, room association, transform, bounds, mass/inertia profile, friction/restitution, collision profile, mobility mode, and damage/destruction profile.
- Support authoring-time placement and runtime manipulation: select, move, rotate, replace, remove, duplicate, and save/restore layout.
- Runtime physics must support collisions, pushes, impacts, impulses, pressure/force effects, and blast-like radial forces. Props can slide, topple, tip, break, or remain fixed according to their physical profile and constraints.
- Distinguish movable, constrained, anchored, and static props. Heavy furniture may resist motion; wall-mounted or built-in objects can use constraints; fragile decoration can break at lower thresholds.
- Destructible props use pre-fractured Geometry Collections or an appropriate damage-state system. Not every asset should be fractured: use intact-to-damaged visual states for inexpensive objects and full Chaos fracture only where justified.
- Damage can propagate only through explicitly modeled connections (for example, stacked or constrained objects); do not imply physically accurate structural propagation without a validated model.
- Persist object transforms, damage state, missing/broken state, and relevant physics state so the room does not reset unexpectedly after reload or save/load.
- Use player-proximity-driven activation as the governing rule: every relevant object and environmental element within the player's active vicinity is eligible for its full required dynamic behavior, including interaction, physics, damage, and destruction. This applies to the whole nearby environment—not only interior furniture. Do not arbitrarily downgrade nearby objects to simplified behavior merely to save performance; manage cost through scoped activation, culling, physics sleep when genuinely at rest, and sensible asset-specific simulation policies.
- Outside the active player vicinity, suspend expensive live simulation and retain the authoritative world state (transforms, damage, broken/missing state, and other gameplay changes). On re-entry, restore that state and reactivate the required dynamics. Instancing or baked/static representations are permitted only for distant or non-interactive content, not as a substitute for independent behavior inside the active vicinity.
- Keep authoring metadata and stable IDs in MayhemBuildingData; Unreal owns live simulation state and reports permitted edits/state changes back through the synchronization layer.

Unreal implementation direction:
- Use Chaos rigid-body simulation for movable furniture and props.
- Use Physics Constraints for attached, hinged, stacked, or limited-motion objects where appropriate.
- Use Chaos Geometry Collections and damage thresholds for selected destructible assets; use Physics Fields for localized force/strain effects such as impacts and blast impulses.
- Expose interaction and damage controls through the dynamic Blueprint building actor, while implementing high-frequency physics and state management in native C++ components.
- Add a per-asset policy: `Static`, `Movable`, `Constrained`, `Destructible`, or `Hybrid`.

This is an accepted product requirement and architecture decision, not an implemented feature yet. It extends Milestone 5 (Complete Interior Generation) and Milestone 7 (Unreal Engine Integration); data IDs, prop metadata, persistence, and performance policy must be designed before the Unreal runtime implementation.

## Milestone 5 — Complete interior generation
**Status: Planned**

- [ ] Room function profiles and furnishing templates.
- [ ] Independent semantic prop entities with stable IDs, transforms, room association, physical properties, mobility and damage profiles.
- [ ] Editable furniture placement and runtime move/rotate/replace/remove workflows with layout persistence.
- [ ] Furniture footprints, clearance envelopes, and circulation paths; connect furnishing collision checks to the physical circulation envelopes.
- [ ] Movable, constrained, anchored, and static prop behavior; impact, impulse, pressure and radial-force responses.
- [ ] Selective destructibility for suitable props, including damage states and Chaos Geometry Collections where appropriate.
- [ ] Persist prop transforms and damage/broken states across save/load.
- [ ] Distance-based physics budgets, sleep/disable policies, and instancing for non-interactive repeated props.
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

## Dynamic Unreal Blueprint requirement (accepted architecture decision, 2026-10-31)

Buildings must not be delivered to Unreal Engine only as one merged Static Mesh. The integration must create a reusable, editable Blueprint Actor (for example, `BP_MayhemBuilding`) backed by a native C++ runtime base class and structured building data.

Required behavior:
- Mayhem's authoring/generation side remains the source of truth for building geometry, room IDs, wall/opening definitions, floor data, materials, and variation parameters.
- Export a versioned, engine-neutral building package (initially JSON plus generated mesh/asset references) with stable IDs and a content hash.
- An Unreal Editor plugin imports or synchronizes that package and creates/updates a Blueprint child of `AMayhemBuildingActor`; it must preserve the Blueprint asset identity and user-authored settings when regenerating.
- The Blueprint exposes editable parameters and component groups for floors, walls, doors, windows, roofs, interiors, materials, and collision/navigation settings. Components must retain semantic IDs so individual elements can be selected, replaced, or adjusted.
- A clear `Apply/Regenerate from Mayhem` workflow updates the Unreal representation when the source plan changes; stale generated components are reconciled by stable ID rather than deleting user-authored content.
- Support editor-time preview/rebuild and a separate runtime generation path with explicit performance and asset-cooking constraints. Do not assume editor-only asset creation APIs are available in packaged runtime builds.
- Blueprint Construction Script is appropriate for editor-visible parameter-driven setup; runtime behavior and heavier geometry generation belong in the native C++ Actor/component layer, callable and extendable from Blueprint.
- Keep an optional baked/optimized representation for shipping, while retaining the editable procedural Blueprint source asset.

Planned integration architecture:
1. `MayhemBuildingData`: versioned interchange schema independent of Unreal.
2. `AMayhemBuildingActor`: native C++ base actor exposing Blueprint-readable/editable properties and regeneration functions.
3. `UMayhemBuildingComponent` family: semantic building parts and generated geometry.
4. Unreal Editor plugin/importer: create/update Blueprint assets, synchronize data, report validation errors.
5. Blueprint child template: user-facing dynamic building asset placed in levels.
6. Bake pipeline: convert generated components to optimized static assets without losing source linkage.

This is a product requirement and architecture decision, not an implemented feature yet. It belongs to Milestone 7; its data contract and stable semantic IDs must be designed before the Unreal plugin is implemented.

## Milestone 7 — Unreal Engine integration
**Status: Planned**

- [ ] Unreal plugin module and supported engine-version policy.
- [ ] Native C++ base actor and semantic procedural component architecture, exposed to Blueprint.
- [ ] Versioned MayhemBuildingData interchange schema with stable room/wall/opening/floor IDs and content hashes.
- [ ] Import/sync workflow that creates or updates a reusable Blueprint child while preserving user-authored settings and asset identity.
- [ ] Blueprint-exposed parameters and Apply/Regenerate-from-Mayhem workflow.
- [ ] Separate editor-time rebuild and packaged runtime generation paths.
- [ ] Optional bake-to-optimized-assets workflow with source linkage.
- [ ] Import/bake workflow for generated building data.
- [ ] Instanced props and material mapping, excluding independently simulated/damageable props.
- [ ] Chaos rigid-body, constraints, Physics Fields, and selective Geometry Collection destruction integration.
- [ ] Blueprint/C++ runtime controls for prop interaction, damage, and persistence.
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

## Residential variation decision (2026-10-31)

The first balanced south-entry residential layout is accepted as one selectable design variation, not as a universal template. A first variation/profile API is now implemented. It generates Balanced Conventional (baseline), Open Living (larger public zone and living footprint), and Privacy First (deeper circulation spine and shifted private-zone proportions) candidates from the same footprint and room brief. Candidates are checked with the current polygon, coverage, overlap, opening-boundary, room-dimension, and access-connectivity validators, then sorted by the current geometric quality score. This is an initial zoning-profile variation system, not yet a full room-topology search: the three profiles still use the same five room roles and south-entry circulation concept. A future iteration must add optional vestibules, alternative corridor graphs, room-count briefs, and door-placement/privacy scoring to achieve stronger topological diversity.

## Current execution order

The project is now prioritizing a visible, user-driven vertical slice rather than adding more isolated backend interfaces:

1. Make the current parametric house generator easy to run and inspect; keep its supported inputs and limitations explicit.
2. Expand the building brief to support arbitrary room schedules, plot/access constraints, orientation, and architectural priorities; generate and rank candidates against hard constraints and soft quality objectives.
3. Complete multi-floor vertical circulation (actual stairs, landings, elevator openings, and traversable links).
4. Generate the first 3D shell from an accepted plan: walls with thickness, floor slab, roof, doors, and windows.
5. Add a visual authoring interface and then full interiors/furnishing.
6. Build the Unreal Blueprint integration after the engine-neutral building data contract and 3D geometry are stable.

A milestone is not considered user-visible complete until the user can run it, inspect an artifact, and see validation results. Do not describe a data contract or API as a finished user-facing feature.
