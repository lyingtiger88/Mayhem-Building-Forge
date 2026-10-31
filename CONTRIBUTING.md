# Contributing to Mayhem Building Forge

Thank you for your interest in MBF. The project is in early development; small, testable contributions are especially valuable.

## Before coding

1. Read README.md, docs/ARCHITECTURE.md, and ROADMAP.md.
2. Check existing issues and discussions before starting a large feature.
3. For substantial changes, open an issue describing the problem, proposed approach, and acceptance criteria.

## Development rules

- Keep the procedural core independent of Unreal Engine, Qt, Blender, and rendering APIs.
- Use C++20 and the existing CMake build.
- Keep dimensions in metric units and make units explicit in identifiers.
- Make generation deterministic when a seed is supplied.
- Validate inputs and return actionable errors.
- Add regression tests for algorithm changes and edge cases.
- Avoid silently changing serialized data contracts; version schema changes.
- Do not claim a feature is complete until its acceptance criteria and tests pass.

## Pull request checklist

- [ ] Scope and motivation are explained.
- [ ] Code builds with the documented toolchain.
- [ ] Tests are added or updated.
- [ ] Existing tests pass.
- [ ] Public API and data-contract changes are documented.
- [ ] Roadmap and README status are updated when capabilities change.
- [ ] Third-party code, assets, and licenses are attributed correctly.

## Code style

- Prefer clear names and small functions.
- Use snake_case for functions and variables, consistent with the current core.
- Use PascalCase for public data types.
- Avoid unnecessary dependencies in the core.
- Keep algorithm-specific details out of public data contracts unless consumers need them.

## Reporting bugs

Include operating system, compiler and CMake versions, commit SHA, reproduction steps, expected result, actual result, and complete error/test output.

## Licensing

By submitting a contribution, you agree that it may be distributed under the repository's MIT license. Do not submit third-party code or assets unless their license permits the intended use and attribution is included.
