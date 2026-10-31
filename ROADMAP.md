# Mayhem Building Forge — Product Roadmap

This roadmap is the delivery plan. It separates implemented work from targets so planned features are never mistaken for existing capabilities.

**Status legend:** Done · In progress · Planned · Future

## Product direction

- Desktop authoring application and Unreal Engine integration.
- Full building interiors.
- Hybrid procedural generation: architectural rules + geometry algorithms + curated asset libraries.
- Large-environment readiness through instancing, baking, LOD/HLOD, and streaming.
- Multiplayer is deliberately deferred until single-player generation and the data model are stable.

## Milestone 0 — Repository and core foundation
**Status: Implemented (initial baseline)**

- [x] C++20 engine-independent core.
- [x] CMake project and sample executable.
- [x] Deterministic seed input.
- [x] Grid-growth floor-plan generator.
- [x] Weighted rectangular / treemap generator.
- [x] Room geometry, area, cell ownership, and adjacency output.
- [x] Basic validation and automated regression tests.
- [x] GitHub Actions build-and-test workflow.
- [x] Research reference analysis and attribution.

**Exit criteria:** Core builds in CI; fixed inputs produce repeatable results; sample and regression tests run. This does not certify architectural correctness.

## Milestone 1 — Architectural floor-plan intelligence
**Status: Planned / next priority**

- [ ] Stable room and building typology schema.
- [ ] Explicit grid dimensions and coordinate mapping in serialized output.
- [ ] Shared-boundary measurements, not only edge counts.
- [ ] Circulation graph: entrance, corridor, room access, vertical core.
- [ ] Door candidates with clear-width and wall constraints.
- [ ] Window candidates from exterior-facing boundaries.
- [ ] Connectivity validation: no inaccessible rooms or isolated circulation components.
- [ ] Room minimum-area and aspect-ratio constraints.
- [ ] Candidate scoring and actionable generation diagnostics.
- [ ] Randomized regression and property-based tests.

**Exit criteria:** Every accepted plan passes geometric coverage, connectivity, access, and configured dimensional constraints.

## Milestone 2 — Multi-floor building model
**Status: Planned**

- [ ] Building envelope and floor stack.
- [ ] Floor-to-floor heights and slab thickness.
- [ ] Stair and elevator core continuity.
- [ ] Vertical alignment of structural elements.
- [ ] Inter-floor circulation validation.
- [ ] Building-level data contract and versioning.

**Exit criteria:** Multi-floor buildings have aligned floor plates and valid vertical circulation.

## Milestone 3 — 3D structural generation
**Status: Planned**

- [ ] Wall, slab, ceiling, roof, and column generation.
- [ ] Door and window openings integrated into wall geometry.
- [ ] Interior/exterior wall thickness and junction handling.
- [ ] UV/material slots and collision geometry.
- [ ] Geometry validation and mesh diagnostics.
- [ ] Deterministic mesh output.

**Exit criteria:** Approved plans produce clean structural meshes with tested openings, collisions, and materials.

## Milestone 4 — Facades and architectural styles
**Status: Planned**

- [ ] Facade grammar and style profiles.
- [ ] Window, balcony, entrance, roofline, and material composition.
- [ ] Style constraints by typology and floor height.
- [ ] Asset-library metadata and compatibility rules.
- [ ] Variation controls that preserve structural validity.

**Exit criteria:** Distinct facade variants can be generated without breaking the structure.

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

**Exit criteria:** Benchmark scenes meet documented performance and memory targets on reference hardware.

## Milestone 9 — Multiplayer and collaboration
**Status: Future / deferred**

- [ ] Replication based on generator version, seed, and state deltas.
- [ ] Collaborative building authoring.
- [ ] Conflict resolution and deterministic regeneration.
- [ ] Network performance and authority model.

Multiplayer is intentionally not an early milestone. It should be designed after the building data model and generation determinism stabilize.

## Current sprint — Next engineering work

1. Harden grid and treemap geometry contracts.
2. Add shared-boundary measurements and room connectivity tests.
3. Introduce entrance, corridor, and door constraints.
4. Add a machine-readable floor-plan export format.
5. Produce a visual debug renderer for generated 2D plans.

The sprint is complete only when outputs can be inspected and tested, not merely when code has been committed.
