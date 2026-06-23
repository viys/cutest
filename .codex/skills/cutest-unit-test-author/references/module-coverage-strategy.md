# Module Coverage Strategy

Use this reference when the task is not just "write a test", but "cover this module well" or "improve generated tests".

## Design goals

- Cover behavior, not just lines
- Prefer independent and repeatable tests
- Organize tests by the structure of the module
- Keep tests fast enough to run frequently
- Make failures point clearly to the missing behavior

## Coverage checklist for a C module

For each public function, check:

- nominal input and expected output
- minimum or empty input
- threshold-crossing input
- invalid or defensive input, if the code handles it
- repeated calls on the same object
- interaction with adjacent APIs on the same object
- ownership, lifetime, or cleanup effect
- error or diagnostic message formatting, if exposed

For each mutable object or struct, check:

- initial state
- one-step mutation
- repeated mutation
- boundary mutation
- failed mutation and post-failure state

For each branch or boolean decision, check:

- true outcome
- false outcome
- each term of compound conditions when the terms influence different behavior

## How to generate better tests from requirements

Convert the requirement into:

1. observable behavior
2. input classes
3. state preconditions
4. expected state changes
5. expected text or return value

Then select the smallest set of tests that spans:

- one happy-path case
- one lower-boundary case
- one upper-boundary or resize case
- one invalid or defensive case when supported
- one multi-step interaction case when state is preserved across calls

## Applying coverage tools

If the project is built with GCC and the user wants measurable coverage, `gcov` can report:

- function summaries
- branch frequencies
- condition coverage
- path coverage

Use this information to decide what to test next:

- unexecuted function: add at least one direct call path
- untaken branch: add the input or state that flips the decision
- uncovered condition term: isolate the term with the smallest input change
- uncovered path: add only if the path corresponds to meaningful behavior

Do not chase path coverage blindly on complex code. Prefer branches and state transitions first.

## Repository-specific advice

For this CuTest repository, the highest-value gaps are usually:

- resize boundaries around `ARRAY_MAX`, `ARRAY_INC`, `STRING_MAX`, and `STRING_INC`
- null handling in string or assert helpers
- insert and append positions around start, middle, end, and beyond length
- pass or fail transitions that also affect message text
- suite aggregation order and detail rendering

Use `test/CuTestTest.c` as the canonical style reference before inventing a new pattern.
