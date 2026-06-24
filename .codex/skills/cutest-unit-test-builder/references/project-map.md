# Reference Repository Map

## Core files

- `CuTest.h`: public API, core structs, assert macros, suite macros, size constants
- `CuTest.c`: implementation of arrays, strings, tests, asserts, and suites, including suite capacity growth and owned-object cleanup
- `test/CuTestTest.c`: primary regression tests and best source of local style in this reference repository
- `test/AllTests.c`: test aggregation entry point; currently aggregates `CuGetSuite()`, `CuArrayGetSuite()`, and `CuStringGetSuite()` and releases each child suite shell after aggregation
- `test/CMakeLists.txt`: build integration for the test executable
- `README`: original usage documentation
- `readme.md`: expanded project notes in Chinese, including examples and file roles

## Where to place new tests

- Extend `test/CuTestTest.c` when validating framework behavior in `CuTest.c` for this reference repository
- Add the test to the most relevant existing suite function in `test/CuTestTest.c`: `CuGetSuite()`, `CuArrayGetSuite()`, or `CuStringGetSuite()`
- Keep `test/AllTests.c` unchanged unless a new suite is intentionally introduced
- If a new suite is intentionally introduced here, preserve the current top-level ownership pattern in `RunAllTests()`

## Typical workflow

1. Read the target function in `CuTest.c` and its declaration in `CuTest.h`
2. Search `test/CuTestTest.c` for similar behavior or similar asserts
3. Add a focused `Test...` function near related tests
4. Register the test in the matching `*GetSuite(void)` block
5. Verify build wiring in `test/CMakeLists.txt` only if files or suites changed
