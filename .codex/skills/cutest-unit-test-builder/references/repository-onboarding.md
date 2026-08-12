# Repository Onboarding

Use this reference when the skill is invoked inside an unknown repository layout.

## Goal

Determine whether the repository already uses CuTest, where tests belong, and how to extend the existing structure with minimal disruption.

## Discovery checklist

1. Search for framework anchors:
   - `CuTest.h`
   - `CuTest.c`
   - `SUITE_ADD_TEST`
   - `CuSuite`
   - `RunAllTests`
   - `GetSuite`
2. Search for test files:
   - `*Test*.c`
   - `*_test.c`
   - `test_*.c`
3. Search for build wiring:
   - `CMakeLists.txt`
   - `Makefile`
   - `*.mk`
   - `meson.build`
   - IDE or vendor project files
   - `test.ps1`, `build.ps1`, `make-tests.py`, and `generate_coverage_report.py`
4. Identify:
   - production source directories
   - test source directories
   - suite registration entry points
   - test executable target names
   - existing coverage options or scripts

## Decision rules

- If CuTest integration already exists, extend it in place.
- If CuTest source files are vendored but no tests exist yet, add the smallest local suite and registration path that matches the repository build style.
- If the repository does not include CuTest, add it only when the user explicitly requested a port or integration. Otherwise report the missing prerequisite instead of silently introducing a framework.
- If multiple test layouts exist, choose the one closest to the changed module.

## What to mirror

- file naming
- test function naming
- suite function naming
- include order
- assertion style
- build target placement
- cleanup and ownership conventions

## What not to assume

- tests are under `test/`
- aggregator file is named `AllTests.c`
- one file maps to one suite
- coverage uses `gcov`
- `CuSuiteAdd(...)` has the same return contract in every fork
- generated registries belong in the source tree
- host-side failures automatically propagate through the process exit code

## Minimal adaptation strategy

1. Read the target function and its header declaration.
2. Read one nearby production file and one nearby test file.
3. Place the new test beside the nearest comparable tests.
4. Register it in the local suite path already used by the repository.
5. Touch build files only when the repository requires explicit file lists or target updates.
