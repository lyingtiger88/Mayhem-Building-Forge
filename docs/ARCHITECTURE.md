# Architecture

Mayhem Building Forge is an engine-independent C++20 architecture core with two clients: a Qt desktop authoring application and an Unreal Engine plugin. Both consume the same versioned core contract; architectural rules remain in Core.

## Current core
- FloorPlan: metric footprint, room regions, grid-cell ownership, room adjacency.
- FloorPlanGenerator: deterministic grid-growth and weighted treemap algorithms.
- Validation: basic invariants for dimensions, room identity, area, cell ownership, and adjacency.

## Generation modes
- Offline: batch-generate, validate, and bake reusable assets.
- Runtime: generate selected variants or changed structures under bounded job budgets.
- Streaming: load/unload shells and interior chunks according to player relevance.

## Determinism
Each request carries a seed and schema version. Output-changing algorithm changes require a generator-version increment or compatibility policy.

## Scale
Use instancing for repeated props, separate structure from decoration, keep collision meshes simpler than render meshes, and avoid loading all interiors simultaneously. Bake LOD/HLOD and streaming metadata offline. Multiplayer should replicate seed/version and approved state deltas rather than full meshes.
