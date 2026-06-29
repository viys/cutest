# CuTest Porting Playbook

Use this reference when the task is to port or strengthen CuTest integration in a target C repository rather than only add test cases.

## Goal

Choose and wire the smallest viable CuTest integration for the repository:

- `board test` on real hardware
- host-side `unit test`
- or both, when the repository benefits from split responsibilities

## Core rules

- Keep CuTest core independent from product-specific code.
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
│   ├── CuTest.c
│   ├── CuTest.h
│   ├── memory/
│   └── scripts/
├── tests/
│   ├── board/
│   │   ├── board_registry.c
│   │   ├── board_runner.c
│   │   ├── board_port.c
│   │   ├── board_runtime.c
│   │   ├── board_test_config.h
│   │   └── board_test_*.c
│   └── unit/
│       ├── unit_registry.c
│       ├── unit_test_config.h
│       ├── unit_compat.h
│       └── unit_test_*.c
├── cmake/
│   ├── board_tests.cmake
│   └── unit_tests.cmake
└── build script
```

Treat this as a pattern, not a hard requirement. Match the target repository when it already has a stronger local convention.

## Registry generation

Maintain separate generator configs for separate test paths:

- `board-tests.json`
- `unit-tests.json`

Recommended behavior:

- `board-tests.json` scans only real board test sources and usually generates a suite entry without `main()`
- `unit-tests.json` scans only real host-side test sources and may generate a registry with `main()`

Keep scan rules narrow:

- include real test files only
- exclude runner, port, runtime, compatibility, and config files

## Build-system rules

For `board test`:

- add board-specific sources only to the board test path
- keep runner and runtime compatibility separate from production entry logic
- connect the test path through the repository's existing firmware build where possible

For host-side `unit test`:

- keep host build wiring separate from cross-compilation wiring
- prefer a dedicated build option or target rather than mixing host logic into firmware targets
- reuse build directories and configuration when possible to avoid full reconfigure on each run

## Porting workflow

1. Discover existing CuTest files, test layout, and build wiring.
2. Decide whether the user needs `board test`, `unit test`, or both.
3. Decide whether direct host compilation is possible or a small compatibility layer is needed.
4. Decide whether memory middleware is needed.
5. Create or update generator configs for each active test path.
6. Wire registry generation into the build or test script.
7. Add the smallest runner or compatibility files required by the chosen path.
8. Verify the exact command the user will use to build and run tests.

## What to avoid

- forcing one repository's directory structure onto another
- assuming a stub header is always required for host tests
- assuming memory middleware is always required
- mixing board and host test scan patterns into one generator config
- modifying CuTest core files to solve repository-specific integration problems unless the defect is truly generic
