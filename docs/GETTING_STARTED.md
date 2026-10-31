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

## 5. Run the sample

Windows Visual Studio generator:

    .\build\Release\mbf_generate_sample.exe

Linux or single-configuration build:

    ./build/mbf_generate_sample

The sample generates a seeded floor plan and prints each room's area, cell count, and number of room adjacencies.

## 6. Modify the sample

Edit examples/generate_sample.cpp to change:
- building width and depth in metres;
- grid resolution;
- reproducible seed;
- room names and target areas.

Rebuild and run again. Current room areas are approximate weighted shares; minimum room dimensions and architectural circulation rules are not enforced yet.

## 7. Further reading

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
