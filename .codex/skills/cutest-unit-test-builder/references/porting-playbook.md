# CuTest Porting Playbook

Use this reference when the task is to port or strengthen CuTest integration in a target C repository rather than only add test cases.

Read `docs/test-porting-playbook.md` first when it exists in the current repository. That document is the repository-level source of truth for this skill. This file is only a compact skill reference.

## Contents

- Goal and core rules
- Minimum files and test-path selection
- Memory and directory structure
- Registry and build-system rules
- Host ports, board runners, and coverage
- Porting workflow and pitfalls

## Goal

Choose and wire the smallest viable CuTest integration for the repository:

- `board test` on real hardware
- host-side `unit test`
- or both, when the repository benefits from split responsibilities

## Core rules

- Keep CuTest core independent from product-specific code.
- Write the ported files into the target repository that the user asked to modify, not into the current reference repository unless that repository is itself the target.
- Reuse the target repository's build system and naming style.
- Prefer generated registries over hand-maintained test lists when aggregation is already script-friendly.
- Keep real test sources separate from runner, port, runtime, and compatibility files.
- Do not bake chip, SDK, absolute path, or product-specific macro assumptions into generic porting logic.

## Minimum CuTest files

Always identify these first:

- `CuTest.c`
- `CuTest.h`
- `make-tests.py` or equivalent generator if registry generation is desired

Add memory middleware only when the target environment needs it.
Add `generate_coverage_report.py` only when host-side coverage reporting is desired.

## Choosing board test vs unit test

Choose `board test` when the behavior depends on:

- interrupts
- exact timing
- register effects
- real peripherals
- bus or pin signaling
- wake or sleep paths
- toolchain-specific runtime behavior

Choose host-side `unit test` when the behavior is mainly:

- logic
- state transitions
- bounds checking
- cache updates
- input validation
- formatting or reporting

If a module mostly fits host testing but has a few platform dependencies:

1. try compiling it directly with the repository's existing public headers
2. if that fails, add only the smallest test-side compatibility layer needed
3. if it still depends on full platform semantics, move it back to `board test`

## Memory strategy

Do not assume memory middleware is mandatory.

Use system allocation directly when:

- the environment already supports stable `malloc` family functions
- test isolation does not require a separate allocator
- the repository is comfortable using standard dynamic allocation in tests

Use CuTest memory middleware when:

- standard allocation is unavailable or undesirable
- test allocations should be isolated from production allocation
- memory limits should be controlled more explicitly

Avoid global allocator overrides unless the repository already depends on that pattern.

## Recommended structure

One common layout is:

```text
project/
├── cutest/
│   └── src/
│       ├── CuTest.c
│       ├── CuTest.h
│       ├── CMakeLists.txt
│       ├── memory/
│       └── scripts/
│           ├── make-tests.py
│           └── generate_coverage_report.py
├── tests/
│   ├── config/
│   │   ├── board-tests.json
│   │   └── unit-tests.json
│   ├── board/
│   │   ├── board_runner.c
│   │   ├── board_port.c
│   │   ├── board_runtime.c
│   │   ├── board_test_config.h
│   │   └── board_test_*.c
│   └── unit/
│       ├── unit_port.c
│       └── unit_test_*.c
├── cmake/
│   ├── board_tests.cmake
│   └── unit_tests.cmake
└── build script
```

Treat this as a pattern, not a hard requirement. Match the target repository when it already has a stronger local convention.

Generate registries under the build directory. Do not commit or hand-edit them unless the target repository explicitly requires committed generated sources and verifies their consistency.

## Registry generation

Maintain separate generator configs for separate test paths:

- `board-tests.json`
- `unit-tests.json`

Recommended behavior:

- `board-tests.json` scans only real board test sources and usually generates a suite entry without `main()`
- `unit-tests.json` scans only real host-side test sources and may generate a registry with `main()`
- host-side `main()` returns nonzero when allocation or any test fails

Keep scan rules narrow:

- include real test files only
- exclude runner, port, runtime, compatibility, and config files
- pass the build system's explicit test source list through `--files` when practical
- make the registry depend on the generator, JSON config, and all selected test sources

## Build-system rules

For `board test`:

- add board-specific sources only to the board test path
- keep runner and runtime compatibility separate from production entry logic
- connect the test path through the repository's existing firmware build where possible

For host-side `unit test`:

- keep host build wiring separate from cross-compilation wiring
- prefer a dedicated build option or target rather than mixing host logic into firmware targets
- reuse build directories and configuration when possible to avoid full reconfigure on each run
- register executables with CTest when CMake is used
- split targets when modules require conflicting stubs or compile definitions

## Host-side ports and board runners

For host tests, try compiling production headers and sources directly before adding `unit_port.c` or stubs. Implement only directly required symbols, keep state resettable between tests, and do not simulate registers, exact time, interrupts, or peripherals as if that proved hardware behavior.

For board tests, separate the suite runner, output port, and minimal runtime compatibility. Make the final pass/fail state observable by automation. Respect watchdog, interrupt, DMA, concurrency, and output-buffer behavior when the runner holds or exits.

## Coverage

When the target project needs coverage, use the public repository script at `cutest/src/scripts/generate_coverage_report.py`. Call it from the target project's own top-level script with project-specific paths, filters, and CMake arguments. Keep those values out of the generic Python script.

Measure production sources only. Exclude CuTest, tests, generated registries, ports, and stubs. Do not borrow private project paths, fixed source lists, product identifiers, or coverage scripts.

## Porting workflow

1. Discover existing CuTest files, test layout, and build wiring.
2. Decide whether the user needs `board test`, `unit test`, or both.
3. Decide whether direct host compilation is possible or a small compatibility layer is needed.
4. Decide whether memory middleware is needed.
5. Create or update generator configs for each active test path.
6. Wire registry generation into the build or test script.
7. Add the smallest runner or compatibility files required by the chosen path.
8. Verify failure propagation through process status or a stable board-visible result.
9. Verify the exact command the developer will use to build and run tests.
10. Add the repository coverage script only when coverage is requested.

## What to avoid

- forcing one repository's directory structure onto another
- assuming a stub header is always required for host tests
- assuming memory middleware is always required
- mixing board and host test scan patterns into one generator config
- modifying CuTest core files to solve repository-specific integration problems unless the defect is truly generic
- copying private project paths, macros, product names, fixed source lists, or coverage tooling into reusable guidance
