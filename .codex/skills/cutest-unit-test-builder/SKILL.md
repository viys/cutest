---
name: cutest-unit-test-builder
description: Expand and maintain CuTest-based unit tests in C repositories that use this framework or a compatible integration style. Use when Codex needs to inspect an unknown repository, locate existing CuTest wiring, add missing test cases, generate new CuTest-based tests from user requirements, update suite registration, improve coverage, or write tests alongside changed production C code.
---

# CuTest Unit Test Builder

Inspect the target repository before editing. Do not assume fixed paths. First locate the framework and test wiring with repository search.

Start with repository discovery:

1. Search for `CuTest.h`, `CuTest.c`, `SUITE_ADD_TEST`, `CuSuite`, `RunAllTests`, `*GetSuite`, and build files such as `CMakeLists.txt`, `Makefile`, `*.mk`, `meson.build`, or project files.
2. Identify where production C files live, where tests live, how suites are registered, and how the test binary is built.
3. Read the closest local examples before adding tests.
4. Reuse the repository's existing test entry and build flow instead of imposing this repository's layout.

Read these files first when relevant:

- `CuTest.h`
- `CuTest.c`
- existing `*Test*.c` or `*_test.c` files
- the repository's test aggregation entry such as `AllTests.c` or equivalent
- the repository's test build files
- `README`
- `readme.md`
- `make-tests.ps1`
- `make-tests.sh`

Follow the existing repository style.

- Keep changes minimal and local to the requested behavior.
- Do not introduce new test helpers unless repetition is substantial and the benefit is clear.
- Prefer extending existing suite functions instead of creating parallel registration paths.
- Preserve the repository's current CuTest conventions. Common patterns include `void TestXxx(CuTest* tc)`, `CuSuite* XxxGetSuite(void)`, and `SUITE_ADD_TEST(...)`.
- Keep tests independent, repeatable, and organized around the structure of the code under test.
- Preserve the repository's current suite ownership model. If the local aggregator releases child suite shells after `CuSuiteAddSuite(...)`, keep doing that. If it keeps them alive, match that pattern.
- When a repository uses a `CuSuiteAdd(...)` variant that returns success status, assert the result in tests that exercise suite capacity or allocation behavior.
- If the repository does not yet contain CuTest wiring, do not invent a large framework migration unless the user explicitly asks for it. Add the smallest viable integration or stop and explain the missing prerequisite.

When adding or extending unit tests:

1. Identify the touched public behavior and the directly affected internal behavior.
2. Inspect the nearest existing tests and mirror their naming, assertion style, file placement, and setup pattern.
3. Add only the tests needed to cover the requested behavior, regression, or boundary condition.
4. Register each new test in the correct local suite or aggregation function.
5. If behavior spans multiple subsystems, extend the smallest relevant suite rather than creating broad new structure.

Prioritize high-value test scenarios:

- null pointers when the API appears to accept or guard them
- empty strings or zero lengths
- insert or append positions at `0`, middle, end, and beyond current length
- resize boundaries around `ARRAY_MAX`, `ARRAY_INC`, `STRING_MAX`, and `STRING_INC`
- suite growth boundaries around `SUITE_INLINE_CAPACITY` and `MAX_TEST_CASES`
- failure-path formatting for messages, file names, and line numbers
- pass/fail state transitions (`failed`, `ran`, `jumpBuf`)
- suite aggregation order and failure counting
- memory ownership and cleanup paths already implied by the current API, including delete-on-NULL behavior that is already supported
- overlapping source and destination buffers when appending or inserting arrays or strings if the implementation explicitly defends that path

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
- cover ownership and capacity transitions when the implementation switches between inline storage and heap-backed storage

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

When the user asks to improve coverage in a target repository:

1. detect the local compiler and test build flow
2. look for an existing coverage target or flags before inventing one
3. if the target repository matches this skill's own reference repository layout, use `references/coverage-workflow.md`
4. otherwise adapt the same coverage reasoning to the repository's existing build system

When the user asks for tests for a new or changed function:

1. Read the implementation and the declaration.
2. Infer the contract from the current code and existing naming patterns.
3. Write tests for the normal path, boundary conditions, and one or two likely misuse paths if the code already defends them.
4. Avoid inventing behavior that the implementation does not promise.

When the request also involves writing production C code in the target repository:

- Read the existing implementation before changing interfaces.
- Preserve current naming and file organization.
- Add comments to newly added functions.
- Avoid unrelated refactors and avoid dynamic-allocation patterns beyond what the project already uses.
- Update or add tests in the same change whenever behavior changes.

Use these references as needed:

- For adapting to an unknown repository layout, read `references/repository-onboarding.md`.
- For the current reference repository structure and test entry points, read `references/project-map.md`.
- For common test-writing patterns in CuTest-style repositories, read `references/test-patterns.md`.
- For module coverage strategy and coverage-driven test generation, read `references/module-coverage-strategy.md`.
- For the reference repository's coverage build and `gcov` workflow, read `references/coverage-workflow.md`.
