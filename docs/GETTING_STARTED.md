# Getting Started

This guide builds the current MBF C++ core. It does not require Unreal Engine or a desktop GUI because neither integration is available yet.

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

Current tests check deterministic grid output, grid coverage, non-empty rooms, treemap area coverage, adjacency output, and rejection of invalid dimensions. These are foundation tests, not a full architectural compliance suite.

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

Run `mbf_build_house --help` for the complete command reference.\n\n## 6. Open the graphical Parametric Lab\n\nOn Windows, run:\n\n```powershell\n.\\scripts\\run_parametric_lab.ps1\n```\n\nThen open `http://127.0.0.1:8765`. The browser UI invokes the native C++ generator through a local Python server. See [Parametric Lab Guide](PARAMETRIC_LAB.md).

## 7. Run the sample

Windows Visual Studio generator:

    .\build\Release\mbf_generate_sample.exe

Linux or single-configuration build:

    ./build/mbf_generate_sample

The sample generates a seeded floor plan and prints each room's area, cell count, and number of room adjacencies.

## 8. Modify the sample

Edit examples/generate_sample.cpp to change:
- building width and depth in metres;
- grid resolution;
- reproducible seed;
- room names and target areas.

Rebuild and run again. Current room areas are approximate weighted shares; minimum room dimensions and architectural circulation rules are not enforced yet.

## 9. Further reading

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
