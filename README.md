# Mayhem Building Forge

An engine-independent procedural architecture system for Project Mayhem.

## Current milestone

**M0 — Building Forge Core foundation**

The first implementation focuses on deterministic 2D floor-plan generation. It adapts ideas from Wojciech Trybus' archived [Procedural-Building-Generator](https://github.com/wojtryb/Procedural-Building-Generator), especially grid-based room growth and squarified rectangular subdivision, while replacing the Blender-coupled implementation with a standalone C++20 core.

## Design principles

- Engine-independent C++20 core; Unreal integration is an adapter, not a dependency.
- Deterministic generation from an explicit seed.
- Two complementary floor-plan algorithms: grid growth and recursive treemap subdivision.
- Explicit room adjacency graph and connectivity validation.
- Metric units and stable, versioned data contracts.
- Offline generation first; runtime generation and streaming are later milestones.

## Build

Requirements: CMake 3.20+, C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Repository layout

- `core/`: public data model and generation algorithms.
- `tests/`: deterministic and geometric regression tests.
- `docs/`: architecture decisions and reference analysis.
- `third_party/NOTICE.md`: attribution for the upstream research reference.

## Reference and attribution

The upstream repository is MIT-licensed. Its concepts are used as a research reference; this repository is a clean C++ implementation and does not copy its Blender integration. See [NOTICE](third_party/NOTICE.md).

## Status

Early development. The current core produces validated 2D floor plans; 3D mesh generation, facade systems, furnished interiors, desktop UI, and Unreal plugin are planned milestones.
