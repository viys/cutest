# Coverage Workflow

Use this reference to generate or interpret coverage for the CuTest repository that owns this skill.

## Supported entry point

Use the repository command:

```powershell
./test.ps1 coverage
```

Do not reconstruct an older direct `gcov` flow or create root-level coverage directories. `test.ps1` calls:

```text
src/scripts/generate_coverage_report.py
```

with this repository's current arguments:

- CMake source: `test/`
- Coverage build: `build/test/coverage/`
- HTML output: `build/test/coverage-report/coverage.html`
- Source filter: `.*src.*\.c$`
- CMake option: `CUTEST_ENABLE_COVERAGE=ON`

The Python script performs a fresh CMake configure, clean build, CTest run, and `gcovr --html-details` generation.

## Requirements

Require CMake, CTest, Python, `gcovr`, and a GCC or compatible Clang coverage toolchain. This workflow does not support MSVC coverage data.

If the command fails, diagnose the first failing stage instead of bypassing the script:

1. required command discovery
2. CMake configuration
3. compilation or linking
4. CTest execution
5. gcovr report generation

## Using the report

Rank missing tests by behavior:

1. unexecuted public APIs
2. untaken branches that change visible state or output
3. capacity and resize boundaries
4. failure-message and detail-rendering paths
5. repeated-operation and ownership transitions

Use `test/CuTestTest.c` as the style baseline, not as a coverage target. Add the smallest tests that close real gaps, regenerate with `./test.ps1 update`, run `./test.ps1`, and rerun coverage when the user requested updated measurement.

## Porting the workflow

When another project adopts coverage, copy or vendor this repository's `src/scripts/generate_coverage_report.py` under `cutest/src/scripts/`. Keep the generic Python script parameter-driven. Pass the target project's source root, CMake source directory, build directory, report directory, production-source filters, and test-enabling CMake arguments from its own top-level script.

Exclude CuTest, test sources, generated registries, ports, and stubs from production coverage. Do not copy a private project's coverage script, fixed module list, absolute path, product name, or toolchain path into this skill or the public porting documentation.
