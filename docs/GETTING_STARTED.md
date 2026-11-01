# Getting Started

This guide builds the MBF C++ core and launches the local browser-based Parametric Lab. Unreal Engine integration and the full desktop authoring application are not yet available.

## 1. Prerequisites

- Git
- CMake 3.20+
- C++20 toolchain:
  - Windows: Visual Studio 2022 with Desktop development with C++
  - Linux: GCC or Clang

Check installations:

    cmake --version
    git --version
    g++ --version

On Windows, run the commands from a Visual Studio Developer PowerShell.

## 2. Clone

    git clone https://github.com/lyingtiger88/Mayhem-Building-Forge.git
    cd Mayhem-Building-Forge

## 3. Configure and build

Windows:

    cmake -S . -B build
    cmake --build build --config Release

Linux:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

## 4. Run tests

    ctest --test-dir build -C Release --output-on-failure

For a single-configuration Linux generator, omit -C Release.

Current tests cover deterministic geometry, topology graph constraints, topology-to-rectangle solving, facade entrances, room access through openings, role-aware windows/open-plan links, spatial-quality scoring, and Parametric Lab request validation. These are development tests, not a full architectural compliance suite.

## 5. Generate a parametric residential house

Build the `mbf_build_house` target, then run it from the repository root:

Windows:
```powershell
.\build\Release\mbf_build_house.exe --width 12 --depth 9 --floors 2 --height 3.2 --profile best --seed 42 --output house.svg
```

The command validates candidate plans and physical circulation, selects the highest-scoring supported profile (or a requested profile), validates the repeated-floor model, and exports an SVG floor plan.

Supported inputs in this prototype:
- `--width` and `--depth`: site dimensions in metres.
- `--setback-south`, `--setback-north`, `--setback-east`, `--setback-west`: site setbacks in metres; these are subtracted to calculate the buildable footprint.
- `--bedrooms`: requested bedroom count; the current layout solver supports exactly one and reports other counts as unsupported rather than silently ignoring them.
- `--floors`: 1–200 repeated floors.
- `--height`: floor-to-floor height in metres.
- `--profile`: `best`, `balanced`, `open`, or `privacy`.
- `--seed`: reproducible seed.
- `--output`: SVG destination.

Current limitation: the planner creates a five-space, one-bedroom residential typology. The Building Brief API can validate room schedules and adjacency requests, but the layout solver does not yet generate arbitrary room counts or custom room schedules. Site setbacks are validated and applied; local zoning/code rules, orientation variants, and 3D meshes are not yet implemented. Multi-floor output currently repeats the same validated floor plan.

Run `mbf_build_house --help` for the complete command reference.

The initial room-topology graph generator can also be run independently:

```powershell
.\\build\\Release\\mbf_topology_preview.exe --bedrooms 3 --output topologies.svg
```

This exports relationship graphs for inspection. It does not yet solve those graphs into room polygons.

## 6. Generate a topology-driven residential plan

The experimental topology solver accepts 1–4 bedrooms, 1–4 bathrooms, and an entrance facade:

```powershell
.\build\Release\mbf_generate_topology_house.exe --width 18 --depth 12 --bedrooms 3 --bathrooms 2 --entrance-side east --floors 1 `
  --living-area 28 --kitchen-area 14 --bedroom-area 16 --bathroom-area 7 --hallway-area 9 `
  --seed 42 --output-prefix house
```

It exports SVG candidates only when the current geometry, wall-opening, and room-access checks pass. This solver currently supports one floor and rectangular guillotine partitions.

The standalone graph preview is:

```powershell
.\\build\\Release\\mbf_topology_preview.exe --bedrooms 3 --bathrooms 2 --output topologies.svg
```

## 7. Open the graphical Parametric Lab

On Windows, run:

```powershell
.\\scripts\\run_parametric_lab.ps1
```

Then open `http://127.0.0.1:8765`. The browser UI invokes the native C++ generator through a local Python server. See [Parametric Lab Guide](PARAMETRIC_LAB.md).

## 8. Run the sample

Windows Visual Studio generator:

    .\build\Release\mbf_generate_sample.exe

Linux or single-configuration build:

    ./build/mbf_generate_sample

The sample generates a seeded floor plan and prints each room's area, cell count, and number of room adjacencies.

## 9. Modify the sample

Edit examples/generate_sample.cpp to change:
- building width and depth in metres;
- grid resolution;
- reproducible seed;
- room names and target areas.

Rebuild and run again. Current room areas are approximate weighted shares; minimum room dimensions and architectural circulation rules are not enforced yet.

## 10. Further reading

- Product capabilities and limitations: README.md
- Technical design: ARCHITECTURE.md
- Delivery phases: ../ROADMAP.md
- User workflow: USER_GUIDE.md
- Research source: REFERENCE_ANALYSIS.md

## Troubleshooting

**CMake cannot find a compiler:** use a Visual Studio Developer PowerShell on Windows, or install a C++ compiler and build tools on Linux.

**CMake is too old:** install CMake 3.20 or newer and ensure the executable is on PATH.

**The executable is in a different folder:** Visual Studio places binaries under build/Release; single-configuration generators usually place them directly under build/.

**A test fails:** capture the complete CTest output, compiler version, operating system, and commit SHA.
