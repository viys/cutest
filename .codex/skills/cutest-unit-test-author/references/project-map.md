# Project Map

## Core files

- `CuTest.h`: public API, core structs, assert macros, suite macros, size constants
- `CuTest.c`: implementation of arrays, strings, tests, asserts, and suites
- `test/CuTestTest.c`: primary regression tests and best source of local test style
- `test/AllTests.c`: test aggregation entry point
- `test/CMakeLists.txt`: build integration for the test executable
- `README`: original usage documentation
- `readme.md`: expanded project notes in Chinese, including examples and file roles

## Where to place new tests

- Extend `test/CuTestTest.c` when validating framework behavior in `CuTest.c`
- Add the test to the most relevant suite function already present in `test/CuTestTest.c`
- Keep `test/AllTests.c` unchanged unless a new suite is intentionally introduced

## Typical workflow

1. Read the target function in `CuTest.c` and its declaration in `CuTest.h`
2. Search `test/CuTestTest.c` for similar behavior or similar asserts
3. Add a focused `Test...` function near related tests
4. Register the test in the matching `*GetSuite(void)` block
5. Verify build wiring in `test/CMakeLists.txt` only if files or suites changed
