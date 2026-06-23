---
name: cutest-unit-test-author
description: Expand and maintain unit tests for the CuTest project in this repository. Use when Codex needs to add missing test cases, generate CuTest-based unit tests from user requirements, update suite registration, extend tests for modified functions, or familiarize itself with this repository before writing C code or test code.
---

# CuTest Unit Test Author

Inspect the repository before editing. Read these files first when relevant:

- `CuTest.h`
- `CuTest.c`
- `test/CuTestTest.c`
- `test/AllTests.c`
- `test/CMakeLists.txt`
- `README`
- `readme.md`
- `make-tests.ps1`
- `make-tests.sh`

Follow the existing repository style.

- Keep changes minimal and local to the requested behavior.
- Do not introduce new test helpers unless repetition is substantial and the benefit is clear.
- Prefer extending existing suite functions instead of creating parallel registration paths.
- Preserve the current CuTest conventions: `void TestXxx(CuTest* tc)`, `CuSuite* XxxGetSuite(void)`, `SUITE_ADD_TEST(...)`.
- Keep tests independent, repeatable, and organized around the structure of the code under test.

When adding or extending unit tests:

1. Identify the touched public behavior and the directly affected internal behavior.
2. Inspect existing tests in `test/CuTestTest.c` and mirror their naming, assertion style, and setup pattern.
3. Add only the tests needed to cover the requested behavior, regression, or boundary condition.
4. Register each new test in the correct `*GetSuite(void)` function.
5. If behavior spans multiple subsystems, extend the smallest relevant suite rather than creating broad new structure.

Prioritize high-value test scenarios:

- null pointers when the API appears to accept or guard them
- empty strings or zero lengths
- insert or append positions at `0`, middle, end, and beyond current length
- resize boundaries around `ARRAY_MAX`, `ARRAY_INC`, `STRING_MAX`, and `STRING_INC`
- failure-path formatting for messages, file names, and line numbers
- pass/fail state transitions (`failed`, `ran`, `jumpBuf`)
- suite aggregation order and failure counting
- memory ownership and cleanup paths already implied by the current API

When optimizing coverage for a module, build a small behavior matrix before writing tests:

1. list exported functions and key internal state they mutate
2. list normal path, boundary path, error path, and formatting or reporting path
3. list branch points and boolean decisions
4. list interactions between consecutive operations on the same object
5. map existing tests to the matrix and add only missing high-value cells

Prefer module coverage over raw test count:

- cover each public API at least once through its intended use
- cover each branch that changes externally visible state or output
- cover state transitions, not only isolated calls
- cover failure reporting text when the module exposes it
- cover one representative path per equivalence class instead of many near-duplicates

Use data-driven thinking when many inputs exercise the same behavior:

- derive a compact set of representative inputs
- split into separate `Test...` functions when the failure message should identify the exact scenario
- keep shared setup small and explicit so each test still reads independently

When the user asks to improve existing test coverage:

1. inspect the module and the current tests
2. identify uncovered branches, conditions, and state transitions
3. add tests that close the largest behavioral gaps first
4. avoid padding coverage with assertions that only repeat already-covered behavior

When coverage tooling is available and the user wants measurement, prefer behavioral signals beyond line coverage:

- function summaries to find never-executed APIs
- branch coverage to find untaken decision outcomes
- condition coverage when boolean expressions combine multiple terms
- path coverage only for small or critical logic, because path growth is rapid

When the user asks to improve coverage in this repository, use the verified local workflow from `references/coverage-workflow.md` before proposing new tests.

When the user asks for tests for a new or changed function:

1. Read the implementation and the declaration.
2. Infer the contract from the current code and existing naming patterns.
3. Write tests for the normal path, boundary conditions, and one or two likely misuse paths if the code already defends them.
4. Avoid inventing behavior that the implementation does not promise.

When the request also involves writing production C code in this repository:

- Read the existing implementation before changing interfaces.
- Preserve current naming and file organization.
- Add comments to newly added functions.
- Avoid unrelated refactors and avoid dynamic-allocation patterns beyond what the project already uses.
- Update or add tests in the same change whenever behavior changes.

Use these project-specific references as needed:

- For repository structure and test entry points, read `references/project-map.md`.
- For common test-writing patterns in this repo, read `references/test-patterns.md`.
- For module coverage strategy and coverage-driven test generation, read `references/module-coverage-strategy.md`.
- For the repository-specific coverage build and `gcov` workflow, read `references/coverage-workflow.md`.
