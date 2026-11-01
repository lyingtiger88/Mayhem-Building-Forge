# Technical Architecture

## 1. System overview

Mayhem Building Forge is designed as an engine-independent procedural architecture system. Architectural rules and generation algorithms live in the core library; user interfaces and rendering engines consume its versioned output.

The planned clients are:
- **Desktop authoring application:** project setup, building briefs, 2D/3D preview, editing, generation controls, and asset-library management.
- **Unreal Engine integration:** editor import/baking, runtime generation hooks, world placement, collision/navigation setup, and streaming integration.

The full desktop authoring application and Unreal integration remain planned. A lightweight browser-based Parametric Lab is implemented as a developer test bench; it invokes the native C++ CLI but does not yet provide full room editing or a persistent project workflow.

## 2. High-level component boundaries

    User / Project
          |
          v
    Authoring Interface
    (planned desktop UI)
          |
          v
    Building Brief + Constraints
          |
          v
    Generation Orchestrator
          |
          +---- Floor-plan algorithms
          |       +---- Grid growth
          |       +---- Weighted treemap
          |
          +---- Architectural validation
          |       +---- Geometry
          |       +---- Room connectivity
          |       +---- Circulation
          |       +---- Building constraints
          |
          v
    Versioned Building Model
          |
          +---- Structural geometry
          +---- Facade generation
          +---- Interior furnishing
          |
          v
    Export / Bake
          |
          +---- Desktop preview
          +---- Unreal Engine adapter

The current repository includes geometry primitives, a constrained five-space residential prototype, a Building Brief validation contract, and the local Parametric Lab. General topology generation, arbitrary room-schedule solving, complete architectural validation, 3D generation, and the Unreal adapter remain planned.

## 3. Core data model

### Building brief
The future building brief will describe typology, footprint, floor count, floor heights, architectural style, room schedule, constraints, and generation settings.

### Floor plan
The current FloorPlan contract contains:
- schema version and deterministic seed;
- building footprint dimensions in metres;
- selected floor-plan algorithm;
- room regions;
- grid cells for cell-based plans;
- room adjacency records.

### Room region
A room region has a stable room ID, name, target and generated area, bounding rectangle, and (for grid plans) its owned cells.

### Future building model
The next data-contract layers should represent:
- floors and vertical relationships;
- structural elements and openings;
- doors, windows, and circulation links;
- facade/material assignments;
- furniture instances and clearance envelopes;
- export metadata and generator compatibility.

## 4. Hybrid generation strategy

MBF uses complementary algorithms rather than forcing every building through one technique.

### Grid growth
A room starts from a seed cell and expands through free neighbouring cells. This provides flexible, non-rectangular room footprints. The current implementation uses a deterministic random generator and area-weighted growth priority.

### Weighted rectangular subdivision
The building footprint is partitioned into rectangular room regions based on target-area weights. It is useful for compact layouts and quick candidate generation. The current implementation is a weighted treemap-style baseline, not a complete architectural planner.

### Wall topology and room access
The topology-driven generator converts room boundaries into exterior/shared wall segments, adds an entrance opening, default interior doors and exterior windows, then applies role-aware opening rules: living/kitchen connections may be wider open-plan links; default windows are omitted from circulation/service spaces and receive role-based width, sill, and height heuristics. It then builds a room-access graph. A candidate is exported only when every room is reachable from the entrance through sufficiently wide access openings. The graphical lab displays these results as architectural SVGs with wall/opening overlays. This first pass uses heuristic centered openings and does not yet model door swings, furniture clearances, wall thickness, structural lintels, or accessibility-code compliance.

### Spatial quality evaluation
The initial spatial quality evaluator reports normalized daylight-potential, adjacency-based privacy, and circulation-connectivity scores. Daylight potential is estimated from room frontage on the exterior envelope and role-based weighting; it is not a solar/daylight simulation and does not place windows. Privacy penalties are applied to selected public/private room adjacencies. Circulation is checked against realized room adjacency connectivity. These explainable heuristic scores rank candidates but do not establish regulatory compliance.

### Rule-based orchestration
Planned orchestration will select and combine algorithms according to typology and constraints. For example, a vertical circulation core can be reserved before room allocation; wet rooms can be clustered; bedrooms can be kept away from public entrances; corridors and access requirements can be checked after candidate generation.

### Asset-assisted completion
After geometry is validated, curated asset libraries can provide doors, windows, fixtures, furniture, and decorative elements. Asset selection must respect room function, scale, style, collision, and clearance constraints.

## 5. Validation pipeline

Validation is a first-class stage, not a final visual inspection.

Planned validation layers:
1. **Input validation:** dimensions, room schedule, supported ranges, and units.
2. **Footprint validation:** complete coverage, no unintended overlaps or gaps.
3. **Room validation:** minimum area, usable dimensions, aspect ratio, and shape quality.
4. **Connectivity validation:** every required room is reachable from an entrance through valid circulation.
5. **Opening validation:** doors connect valid spaces; windows lie on exterior boundaries.
6. **Vertical validation:** stairs/elevators connect the intended floors.
7. **3D validation:** mesh integrity, opening geometry, collision, and navigation.
8. **Interior validation:** furniture clearances, object collisions, and circulation paths.

The current code implements only basic dimensions, room identity, positive area, cell ownership, and adjacency checks. It does not yet implement the complete pipeline above.

## 6. Determinism and versioning

A generation request must carry:
- a seed;
- a building-model schema version;
- a generator version;
- explicit dimensions and room inputs;
- algorithm settings.

For the same inputs, seed, and generator version, the core should produce the same logical result. Any change that intentionally alters output must update the generator version or follow a documented compatibility policy.

## 7. Desktop and Unreal responsibilities

### Desktop application (planned)
The desktop application is the authoring and inspection environment. It should not duplicate architectural algorithms. It should call the core, display its output, and store project settings and user overrides.

### Unreal adapter (planned)
The Unreal adapter translates the engine-neutral model into Unreal assets and runtime objects. It owns engine-specific concerns such as actor/component creation, material assignment, collision, navigation, LOD/HLOD, World Partition, and runtime budgets.

The core must not include Unreal headers or depend on Unreal object lifetimes.

## 8. Large-world and performance strategy

Large-scale generation requires offline and runtime responsibilities to be separated:

- **Offline generation:** create and validate stable building variants; bake meshes and metadata.
- **Runtime generation:** support bounded variants or localized changes with explicit time and memory budgets.
- **Streaming:** load and unload shells, floors, and interior chunks based on player relevance.
- **Instancing:** reuse repeated props and structural modules where possible.
- **LOD/HLOD:** prepare simplified representations for distant buildings.
- **Caching:** key generated output by brief, seed, algorithm version, and asset-library version.

Multiplayer is deferred. Once determinism and the building model are stable, replication should prefer seed/version plus approved state deltas rather than sending complete generated meshes.

## 9. Current implementation boundary

Implemented now:
- C++20 core library;
- grid-growth and weighted treemap floor-plan generation;
- initial room-topology graph candidate generation and graph validation;
- first topology-constrained rectangular floor-plan realization;
- room regions, areas, bounds, cell ownership, and adjacency;
- basic validation;
- CMake sample and regression tests.

Not implemented:
- full architectural constraints and circulation;
- multi-floor building model;
- 3D structural and facade generation;
- complete furnishing;
- full desktop authoring application (the limited browser-based Parametric Lab is available);
- Unreal plugin;
- streaming and multiplayer.

See ROADMAP.md for milestones and exit criteria.
