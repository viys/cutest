---
name: cutest-unit-test-builder
description: Expand, port, and maintain CuTest-based tests in C repositories that use this framework or a compatible integration style. Use when Codex needs to inspect an unknown repository, locate or add CuTest wiring, port CuTest into a target project, choose between board test and host-side unit test integration, add missing test cases, generate registry files, update suite registration, improve coverage, or write tests alongside changed production C code.
---

# CuTest Unit Test Builder

Inspect the target repository before editing. Do not assume fixed paths. First locate the framework and test wiring with repository search.

When the task is about porting CuTest into another repository, treat that other repository as the only valid landing zone for ported files. Do not place the migrated test files, build wiring, or generated registries into this reference repository unless the user explicitly asked to modify this repository itself.

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

If the request is about porting or integrating CuTest rather than only adding test cases, also read:

- `docs/test-porting-playbook.md`
- `references/porting-playbook.md`

For porting tasks, treat `docs/test-porting-playbook.md` as the repository-level source of truth and use `references/porting-playbook.md` only as the skill's compact companion summary.

Follow the existing repository style.

- Keep changes minimal and local to the requested behavior.
- Do not introduce new test helpers unless repetition is substantial and the benefit is clear.
- Prefer extending existing suite functions instead of creating parallel registration paths.
- Preserve the repository's current CuTest conventions. Common patterns include `void TestXxx(CuTest* tc)`, `CuSuite* XxxGetSuite(void)`, and `SUITE_ADD_TEST(...)`.
- Keep tests independent, repeatable, and organized around the structure of the code under test.
- Preserve the repository's current suite ownership model. If the local aggregator releases child suite shells after `CuSuiteAddSuite(...)`, keep doing that. If it keeps them alive, match that pattern.
- When a repository uses a `CuSuiteAdd(...)` variant that returns success status, assert the result in tests that exercise suite capacity or allocation behavior.
- If the repository does not yet contain CuTest wiring, do not invent a large framework migration unless the user explicitly asks for it. Add the smallest viable integration or stop and explain the missing prerequisite.

When the user asks to port or strengthen CuTest integration:

1. Identify whether the repository already vendors CuTest core files, a fork, or only partial wiring.
2. Decide whether the requested path is `board test`, host-side `unit test`, or both.
3. Write the ported files into the target repository, not the current reference repository.
4. Reuse the repository's build system instead of imposing a foreign test harness layout.
5. Prefer generated registry files over hand-maintained registration lists when the repository already uses or is willing to adopt script-based aggregation.
6. Keep porting guidance generic inside the skill. Do not hardcode chip, SDK, project path, or product-specific macro assumptions.

For test-path selection, use this decision rule:

- Prefer `board test` when the behavior depends on interrupts, timing, registers, peripherals, bus signaling, power transitions, or other real hardware effects.
- Prefer host-side `unit test` when the behavior is mostly logic, state transitions, parameter handling, cache updates, formatting, or boundary checking.
- For host-side `unit test`, first check whether the module and its public headers can already compile in the host environment. Only add a small test-side compatibility header when direct compilation is not viable.
- If the module strongly depends on full platform behavior, stop trying to force host compilation and move the test to `board test`.

For CuTest memory strategy, decide instead of assuming:

- If the target environment already provides stable `malloc`/`calloc`/`realloc`/`free` and the repository is comfortable using them in tests, a memory middleware layer may be unnecessary.
- If the environment restricts or isolates dynamic allocation, or the repository wants test memory separated from production allocation, prefer CuTest memory middleware.
- Avoid broad global allocator overrides unless the repository already depends on that pattern.

When adding or extending unit tests:

1. Identify the touched public behavior and the directly affected internal behavior.
2. Inspect the nearest existing tests and mirror their naming, assertion style, file placement, and setup pattern.
3. Add only the tests needed to cover the requested behavior, regression, or boundary condition.
4. Register each new test in the correct local suite or aggregation function.
5. If behavior spans multiple subsystems, extend the smallest relevant suite rather than creating broad new structure.

When porting CuTest into a repository that does not yet have the requested test path:

1. Identify the smallest set of CuTest source files that must be copied or wired in.
2. Identify whether registry generation should use one config for `board test`, one config for `unit test`, or both.
3. Keep scan patterns narrow so generated registries only collect real test files, not runner, port, runtime, stub, or config files.
4. For `board test`, keep runner, port, and runtime compatibility files separate from real test sources.
5. For `unit test`, prefer a registry with `main()` when that removes unnecessary extra layers.
6. Put vendored CuTest core files, generated registries, test sources, and build wiring inside the target repository using its own directory conventions or the structure required by `docs/test-porting-playbook.md`.
7. Touch root build wiring only where needed to enable the chosen test path.
8. Validate the concrete invocation path the user will run, not only file structure.

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
- For porting CuTest into a repository or choosing between board and host-side test paths, read `references/porting-playbook.md`.
- For the current reference repository structure and test entry points, read `references/project-map.md`.
- For common test-writing patterns in CuTest-style repositories, read `references/test-patterns.md`.
- For module coverage strategy and coverage-driven test generation, read `references/module-coverage-strategy.md`.
- For the reference repository's coverage build and `gcov` workflow, read `references/coverage-workflow.md`.
