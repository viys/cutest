---
name: cutest-unit-test-builder
description: Inspect, port, build, extend, and maintain CuTest-based tests in C repositories. Use when Codex needs to locate or add CuTest wiring, choose between target-board tests and host-side unit tests, generate or repair test registries, add regression or boundary tests, integrate CMake or project build scripts, use the optional memory middleware, or measure and improve production-code coverage with the repository's CuTest workflow.
---

# CuTest Unit Test Builder

Inspect the target repository before editing. Reuse its test layout, build system, naming, ownership, and cleanup conventions.

## Start with discovery

1. Search for `CuTest.h`, `CuTest.c`, `SUITE_ADD_TEST`, `CuSuite`, `RunAllTests`, `*GetSuite`, `make-tests.py`, test sources, and build files.
2. Identify production sources, test paths, registry ownership, executable or firmware targets, and the exact build-and-run command.
3. Read the implementation and declaration under test plus the closest comparable tests.
4. Determine whether the repository already vendors CuTest, contains partial wiring, or needs a new integration.

Read `references/repository-onboarding.md` when the repository layout or integration state is unfamiliar.

## Select the test path

- Choose host-side `unit test` for logic, state transitions, validation, formatting, caching, and behavior that compiles directly or with a small test-side port.
- Choose `board test` for interrupts, registers, exact timing, DMA, peripherals, bus signaling, power transitions, startup, or toolchain-specific runtime behavior.
- Use both only when responsibilities are clear and the paths have separate sources, registries, and build targets.
- Stop expanding host stubs when correct behavior requires recreating full platform semantics; move that behavior to board tests.

For a port or integration task, read `docs/test-porting-playbook.md` when the active repository provides it and treat it as repository-level source of truth. Otherwise read `references/porting-playbook.md`.

## Implement the smallest complete change

- Modify the user-selected target repository only. Never place migrated files in a reference repository unless it is itself the target.
- Keep CuTest core independent of product code. Put platform differences in build wiring, runner, port, runtime, or stub files.
- Preserve local `TestXxx`, suite, assertion, formatting, include, ownership, and cleanup conventions.
- Prefer the existing registry model. When using `make-tests.py`, scan only real test sources, pass explicit files from the build when practical, generate into the build directory, and depend on the generator, config, and test sources.
- Keep board runner, board port, runtime compatibility, host stubs, configs, and generated registries outside real test scan patterns.
- Use system allocation by default. Enable the CuTest memory middleware only when the environment restricts allocation or requires an isolated, bounded test heap.
- Avoid unrelated production refactors and new helpers unless repetition or isolation clearly justifies them.

Read `references/test-patterns.md` before adding framework tests or unfamiliar CuTest assertions. Read `references/project-map.md` when modifying this CuTest repository itself.

## Design tests around behavior

Cover the smallest high-value set:

- normal behavior and externally visible state
- empty, zero, `NULL`, or invalid inputs already handled by the contract
- capacity, resize, and hard-limit boundaries
- success, failure, busy, timeout, and repeated-operation transitions
- failure messages or rendered details when they are public output
- ownership and cleanup paths
- interrupt, concurrency, and timing behavior on hardware rather than through overconfident host simulation

Use `references/module-coverage-strategy.md` for module-wide coverage work instead of padding raw test count.

## Verify the real workflow

1. Regenerate registries through the repository's supported command.
2. Build the exact host executable or board firmware target the developer will use.
3. Run host tests through the registered test runner, normally CTest when available.
4. Confirm a deliberately failing assertion would produce a nonzero process status or a stable board-visible failure state.
5. If memory routing changed, verify both standard and middleware variants when the repository supports both.
6. Run coverage only when requested or already part of the workflow; measure production sources, not CuTest, tests, generated registries, ports, or stubs.

For this reference repository's current coverage command and report path, read `references/coverage-workflow.md`.

## Reference routing

- Unknown repository discovery: `references/repository-onboarding.md`
- Porting, board/unit selection, registry and build wiring: `references/porting-playbook.md`
- This repository's authoritative file map and commands: `references/project-map.md`
- Local CuTest test shape, assertions, and ownership patterns: `references/test-patterns.md`
- Behavior matrices and coverage-driven test selection: `references/module-coverage-strategy.md`
- This repository's coverage script and command: `references/coverage-workflow.md`
