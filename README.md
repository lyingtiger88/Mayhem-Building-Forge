# Mayhem Building Forge

**A procedural building-generation platform for creating explorable architecture — from floor plans to furnished interiors and production-ready Unreal Engine assets.**

Mayhem Building Forge (MBF) is planned as a hybrid desktop authoring tool and Unreal Engine integration. Its long-term purpose is to generate believable buildings at scale while keeping architectural rules and generation logic independent of any one rendering engine.

> **Status: Early foundation / pre-alpha.** The repository contains a C++20 architectural geometry core, a legacy one-bedroom prototype, an experimental topology-driven rectangular residential solver (1–4 bedrooms and 1–4 bathrooms), role-aware wall/opening generation, heuristic spatial quality scoring, and a local browser-based Parametric Lab. A full desktop editor, 3D mesh pipeline, furnishing system, and Unreal integration remain future work.

[Documentation Index](docs/README.md) · [Architecture](docs/ARCHITECTURE.md) · [Roadmap](ROADMAP.md) · [Getting Started](docs/GETTING_STARTED.md) · [User Guide](docs/USER_GUIDE.md) · [2D Geometry Plan](docs/2D_GEOMETRY_PLAN.md) · [3D Geometry Plan](docs/3D_GEOMETRY_PLAN.md) · [Reference Analysis](docs/REFERENCE_ANALYSIS.md) · [Contributing](CONTRIBUTING.md)

---

## 1. Product vision

MBF is not intended to be only a random building mesh generator. It is intended to become a controllable architectural production system:

1. Define a building brief: typology, dimensions, floors, architectural style, room schedule, and constraints.
2. Generate multiple floor-plan candidates using complementary algorithms.
3. Validate circulation, room connectivity, dimensions, access, and architectural rules.
4. Turn approved plans into structural 3D geometry.
5. Generate facades, openings, stairs, elevators, roofs, and building services.
6. Furnish interior spaces with functional, style-consistent assets.
7. Export or instantiate the result in Unreal Engine, with support for large worlds and streaming.

The guiding principle is **procedural control, not procedural randomness**. Seeds make generation repeatable; constraints and validation make it useful.

## 2. Product decisions

| Area | Decision |
|---|---|
| User interface | Both: standalone desktop application and Unreal Engine workflow |
| Interior generation | Complete interiors, not exterior shells only |
| Generation strategy | Hybrid: rule-based, algorithmic, and asset-assisted generation |
| Architectural core | Engine-independent C++ |
| Primary real-time target | Unreal Engine |
| Intended scale | Large environments and many buildings |
| Multiplayer | Later milestone; not part of the initial core |

These are product goals, not claims that all capabilities are already available.

## 3. Planned capabilities

### Building authoring
- Residential, commercial, office, industrial, and configurable building typologies.
- Building footprint, dimensions, number of floors, floor-to-floor height, and structural constraints.
- Room schedules with target areas and adjacency requirements.
- Reproducible generation seeds and editable settings.

### Floor-plan intelligence
- Grid-growth generation for varied room shapes.
- Weighted rectangular subdivision for compact layouts.
- Room adjacency graph and circulation graph.
- Doors, windows, entrances, corridors, stairs, and elevator placement.
- Connectivity and accessibility checks.
- Candidate scoring for area fit, aspect ratio, circulation, and daylight potential.

### 3D architecture and interiors
- Walls, slabs, ceilings, roofs, columns, and structural openings.
- Facade systems, windows, doors, balconies, and material rules.
- Stairs, elevators, and vertical circulation.
- Room-aware furniture and prop placement.
- Collision, navigation, lighting, and LOD-ready output.

### Production workflow
- Desktop preview and editing.
- Unreal Engine editor integration and asset baking.
- Runtime generation for bounded use cases.
- Large-world streaming and instancing.
- Versioned data contracts and deterministic regeneration.

See ROADMAP.md for phases and acceptance criteria.

## 4. Current implementation

The current C++20 core provides:
- Metric building-footprint data model.
- Room specifications and room-region output.
- Deterministic grid-growth floor-plan generation.
- Weighted rectangular / treemap-style subdivision.
- Room-cell ownership and room adjacency output.
- Polygon geometry, concave region and courtyard partition validation, polygon offsets, and uniform lot setback calculations.
- A legacy deterministic five-space / one-bedroom residential prototype with three zoning profiles.\n- An experimental topology-driven solver with three adjacency graph strategies, configurable 1–4 bedroom and 1–4 bathroom schedules, and selectable entrance facade.
- A versioned parametric Building Brief contract for site dimensions, four-side setbacks, floor count/height, room schedules, target/minimum areas, priorities, and adjacency rules.
- Input feasibility validation and explicit compatibility diagnostics against the currently implemented residential solver.
- Room, opening, dimension, overlap, access, corridor-width, and door-approach validation.\n- Role-aware exterior windows, interior doors, and open-plan connections; connected room access is required before topology-driven plan export.\n- Architectural SVG output with wall/opening overlays and normalized daylight-potential, privacy, circulation, and overall quality scores.
- Candidate generation and ranking for supported residential profiles.
- Repeated-floor building data and preliminary stair/elevator core footprint validation.
- A parameter-driven `mbf_build_house` legacy CLI and `mbf_generate_topology_house` experimental CLI.\n- A local browser-based Parametric Lab with JSON brief import/export, candidate ranking, quality metrics, and SVG downloads.
- CMake build configuration, sample executable, and automated tests.
- GitHub Actions build-and-test workflow.

### Not implemented yet
- Desktop GUI and visual floor-plan editor.
- General room-schedule solving (arbitrary bedroom/bathroom counts and custom room lists).
- 3D mesh generation.
- Doors, windows, stairs, elevators, or facade generation.
- Furniture and complete interior generation.
- Actual stair flights, landings, elevator doors, and traversable vertical circulation.
- Multi-floor plan variation and structural alignment beyond the initial repeated-floor data model.
- Unreal Engine plugin or packaged Unreal assets.
- City-scale streaming, profiling, and multiplayer.

Do not use the current core as a production architectural design tool. It is an algorithmic foundation and needs stronger geometric, connectivity, and architectural validation before production use.

## 5. Repository structure

    Mayhem-Building-Forge/
    ├── core/
    │   ├── include/mbf/       Public C++ data model and interfaces
    │   └── src/               Floor-plan algorithms and validation
    ├── docs/
    │   ├── ARCHITECTURE.md
    │   ├── GETTING_STARTED.md
    │   ├── USER_GUIDE.md
    │   └── REFERENCE_ANALYSIS.md
    ├── examples/              Runnable examples
    ├── tests/                 Core regression tests
    ├── third_party/           Third-party notices and licenses
    ├── ROADMAP.md
    ├── CONTRIBUTING.md
    ├── CMakeLists.txt
    └── README.md

## 6. Build from source

Requirements: CMake 3.20+, a C++20 compiler (MSVC 2022, GCC, or Clang), and Git.

### Windows (PowerShell)
    git clone https://github.com/lyingtiger88/Mayhem-Building-Forge.git
    cd Mayhem-Building-Forge
    cmake -S . -B build
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure
    .\build\Release\mbf_generate_sample.exe

### Linux
    git clone https://github.com/lyingtiger88/Mayhem-Building-Forge.git
    cd Mayhem-Building-Forge
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/mbf_generate_sample

For troubleshooting and expected output, see docs/GETTING_STARTED.md.


## 6. Parametric Lab (local graphical test bench)

After building the native generator, launch the local browser-based GUI on Windows:

    .\scripts\run_parametric_lab.ps1

Open http://127.0.0.1:8765 in your browser. The interface sends the Building Brief to the local Python standard-library server, which invokes the actual C++ `mbf_build_house` executable and returns its SVG output. It does not use a separate JavaScript floor-plan generator.

The Lab currently exposes site dimensions, four setbacks, floor count, requested bedroom count, seed, and the three implemented residential profiles. Select **Generate all current profiles** to compare the Balanced, Open Living, and Privacy First candidates.

Requirements: Python 3 and a successful CMake build. The server binds only to 127.0.0.1 and has no external service dependency.

**Current engine limitation:** the underlying residential solver still supports one bedroom and five spaces. The GUI makes unsupported briefs and generation diagnostics visible; it does not imply arbitrary room schedules are already solved. The current profile variants are not yet independent spatial topologies.

For details, see [Parametric Lab Guide](docs/PARAMETRIC_LAB.md).

## 8. Research reference and attribution

The initial algorithms are informed by [Wojciech Trybus' Procedural-Building-Generator](https://github.com/wojtryb/Procedural-Building-Generator), an MIT-licensed Blender project. We studied its grid-based room growth, treemap subdivision, adjacency/opening concepts, and plan-quality ideas.

MBF's current core is an independent C++ implementation; it does not embed the upstream Blender add-on. See docs/REFERENCE_ANALYSIS.md and third_party/NOTICE.md.

## 9. Project status and contribution

This project is in active early development. The immediate engineering priority is to make floor plans architecturally valid — not merely visually interesting — before starting the 3D pipeline.

Contributions are welcome once scoped through an issue or discussion. Read CONTRIBUTING.md before submitting code.

## 10. License

The MBF code is distributed under the MIT License. Third-party attributions and license texts are in third_party/.
