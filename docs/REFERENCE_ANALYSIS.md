# Research reference: Procedural-Building-Generator

Reference: https://github.com/wojtryb/Procedural-Building-Generator (Wojciech Trybus; archived; MIT; Blender 2.82 / Python 3.8.5).

## Ideas extracted
1. Grid placement and growth: rooms receive seed cells and expand through free neighbouring cells; the source uses rectangular and L-shaped growth followed by residual-space filling.
2. Squarified treemaps: a building rectangle is recursively subdivided into room rectangles based on target areas.
3. Connectivity and openings: shared room edges are used to reason about doors; disconnected room groups are detected; exterior edges inform windows and exits.
4. Quality metrics: room area error, aspect ratio, window availability, room-to-room distances, and disconnected groups.

## What this implementation carries forward
- Both grid-growth and treemap strategies as complementary algorithms.
- Area-weighted allocation and room adjacency as first-class output.
- Deterministic seeds and separation of plan generation from 3D modelling.
- Future validation metrics inspired by the reference.

## What is redesigned
- No Blender operators, bpy state, NumPy, or Python dependency in the core.
- Typed C++20 data, explicit metric units, validation, and deterministic RNG.
- A serializable engine-neutral plan contract for desktop and Unreal clients.

## Upstream limitations observed
- The project is tightly coupled to Blender scene state and mesh polygon ordering.
- Grid boundary assumptions and tile-walker traversal are implicit.
- In squarifiedTreemaps.py, the condition references self.isHorizontal rather than calling self.isHorizontal(); this appears to be a method-call defect.
- Sample room types are a small residential demonstration, not a general typology framework.
- It is not a city-scale streaming or Unreal integration architecture.

This repository independently implements the algorithmic ideas in C++; it does not copy the Blender-dependent source files.
