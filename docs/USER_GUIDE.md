# User Guide

## What can I do today?

The repository is currently a developer-facing core foundation. There is no graphical application yet. You can compile the library, run its sample, and use its public C++ interfaces in another program.

## Current workflow

1. Clone and build the repository using GETTING_STARTED.md.
2. Edit examples/generate_sample.cpp to define the building footprint, seed, and room schedule.
3. Select generate_grid_growth or generate_treemap.
4. Run the sample and inspect room areas and adjacency count.
5. Run CTest after changing the core.

## Choosing an algorithm

### Grid growth

Use generate_grid_growth when you want a cell-based plan with varied room shapes. Rooms start from seed cells and grow through neighbouring free cells. Output includes cell ownership and adjacency derived from shared cell edges.

### Treemap subdivision

Use generate_treemap when you want a quick rectangular area partition. It produces room rectangles based on target-area weights. It is useful as a baseline, but it does not yet guarantee architectural access, realistic corridors, doors, windows, or furniture fit.

## Input concepts

- width_m and depth_m: building footprint dimensions in metres.
- grid_resolution: horizontal grid resolution for grid-growth generation (4–512).
- seed: deterministic random seed.
- RoomSpec: room name and target area in square metres.

Target areas are relative weights. The current implementation does not guarantee exact target areas or minimum clear dimensions.

## Output concepts

- FloorPlan: generated plan metadata.
- RoomRegion: room identity, target and actual area, bounding rectangle, and cell list (for grid plans).
- GridCell: grid coordinate and owning room ID.
- Adjacency: pair of rooms with shared boundary count information.

## Important limitations

- No wall, door, or window geometry is generated.
- No entrance or circulation constraints are applied.
- Adjacency does not mean a traversable doorway exists.
- Treemap rectangles are not rasterized into grid cells.
- Multi-floor structures and vertical circulation are not implemented.
- No Unreal plugin or desktop UI is available.

Treat outputs as procedural layout candidates for development and testing, not construction documents or code-compliant architectural designs.
