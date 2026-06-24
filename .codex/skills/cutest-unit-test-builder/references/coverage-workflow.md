# Coverage Workflow

Use this reference when the task is to measure or improve test coverage in this reference repository.

## Verified local build flow

This reference repository already supports a coverage build through CMake option `CUTEST_ENABLE_COVERAGE`.

On this machine, the verified commands are:

```powershell
cmake -S test -B build-coverage-gcc -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DCUTEST_ENABLE_COVERAGE=ON
cmake --build build-coverage-gcc
.\build-coverage-gcc\cutest.exe
```

Generate `gcov` reports with:

```powershell
Set-Location build-coverage-gcc
gcov .\lib_build\CMakeFiles\CuTest_static.dir\CuTest.c.obj .\CMakeFiles\cutest.dir\CuTestTest.c.obj
```

The coverage option currently targets GCC or Clang style `gcov` flows. Do not assume the same workflow works with MSVC.

## How to use the report

Use the coverage report to rank missing tests:

1. unexecuted public API
2. partially covered branch-heavy logic
3. state-transition logic with only happy-path tests
4. formatting or failure-reporting code that is rarely exercised

Prefer adding tests that close real behavioral gaps:

- flip an uncovered branch outcome
- cross a resize threshold
- exercise null or empty defensive handling
- verify failure text emitted by assert paths
- verify repeated operations on the same object

Avoid low-value additions:

- duplicating cases that hit the same branch and state transition
- chasing path coverage through incidental combinations
- adding broad helpers just to reduce a few lines of test setup

## Repository-specific priorities

For `CuTest.c`, coverage gaps are most likely to matter in:

- `CuArrayResize`, `CuArrayInsert`, and append edge cases around `ARRAY_MAX` / `ARRAY_INC`
- `CuStringResize`, `CuStringInsert`, and append formatting around `STRING_MAX` / `STRING_INC`
- failure-message formatting and assert mismatch paths
- suite detail rendering when failures accumulate

For `test/CuTestTest.c`, use the file as the style baseline rather than a coverage target. New tests should match its direct assertion style and suite registration pattern.

## Recommended agent behavior

When asked to improve coverage:

1. build and run the coverage target if the environment supports it
2. inspect the `gcov` output for the module under discussion
3. map uncovered lines back to branch, boundary, or state-transition scenarios
4. add the smallest high-value tests needed
5. rerun the coverage flow only if the user asked for measurement or the workflow is already in use
