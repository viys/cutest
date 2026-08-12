# Reference Repository Map

Use this map only when modifying the CuTest repository that owns this skill.

## Authoritative files

- `src/CuTest.h`: public API, structs, assertions, suite macros, allocation routing, and size defaults
- `src/CuTest.c`: arrays, strings, tests, assertions, suites, ownership, and capacity growth
- `src/memory/CuMemory.c` and `src/memory/CUMemory.h`: optional fixed-heap memory middleware
- `src/CMakeLists.txt`: `CuTest_static` implementation target and `CuTest` interface target
- `src/scripts/make-tests.py`: configurable test-registry generator
- `src/scripts/generate_coverage_report.py`: configure, build, run CTest, and generate gcovr HTML reports
- `test/CuTestTest.c`: framework regression tests and local test-style baseline
- `test/make-tests.json`: this repository's registry configuration
- `test/AllTests.c`: generated registry; never edit manually
- `test/CMakeLists.txt`: standard, middleware, CTest, and coverage build wiring
- `test.ps1`: supported update, configure, build, run, coverage, clean, and delete entry point
- `docs/porting-guide.md`: quick guide for developers adopting this repository
- `docs/test-porting-playbook.md`: authoritative detailed porting workflow

## Build directories

- Standard tests: `build/test/`
- Middleware tests: `build/test/middleware/`
- Coverage build: `build/test/coverage/`
- Coverage HTML: `build/test/coverage-report/coverage.html`

Keep temporary configurations under `build/`. Do not maintain legacy root-level coverage directories.

## Repository workflow

1. Read the declaration, implementation, and closest tests.
2. Add focused `TestXxx` functions to `test/CuTestTest.c`.
3. Run `./test.ps1 update` to regenerate `test/AllTests.c`.
4. Run `./test.ps1` to verify standard and middleware allocation paths.
5. Run `./test.ps1 coverage` only when coverage generation or measurement is requested.
