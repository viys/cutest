# Reference Repository Map

## Core files

- `src/CuTest.h`: public API, core structs, assert macros, suite macros, size constants
- `src/CuTest.c`: implementation of arrays, strings, tests, asserts, and suites, including suite capacity growth and owned-object cleanup
- `src/scripts/make-tests.py`: portable generator for AllTests-style aggregation sources
- `test/CuTestTest.c`: primary regression tests and best source of local style in this reference repository
- `test/make-tests.json`: repository-specific generator configuration
- `test/AllTests.c`: generated aggregation entry point that registers matched `Test...` functions
- `test/CMakeLists.txt`: build integration for the test executable
- `README`: original usage documentation
- `readme.md`: expanded project notes in Chinese, including examples and file roles

## Where to place new tests

- Extend `test/CuTestTest.c` when validating framework behavior in `CuTest.c` for this reference repository
- Name new tests `TestXxx` so the configured scanner can collect them
- Do not edit `test/AllTests.c` manually; regenerate it with `./test.ps1 update`

## Typical workflow

1. Read the target function in `CuTest.c` and its declaration in `CuTest.h`
2. Search `test/CuTestTest.c` for similar behavior or similar asserts
3. Add a focused `Test...` function near related tests
4. Regenerate `test/AllTests.c` with `./test.ps1 update`
5. Verify build wiring in `test/CMakeLists.txt` only if source files changed
