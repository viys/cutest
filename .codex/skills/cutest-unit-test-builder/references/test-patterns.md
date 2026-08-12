# Test Patterns

## Common structure

Use the existing shape from `test/CuTestTest.c`:

```c
void TestSomething(CuTest *tc)
{
    /* setup */

    /* exercise */

    /* verify */
}
```

Register with:

```c
SUITE_ADD_TEST(suite, TestSomething);
```

## Existing style cues

- Prefer direct assertions over abstraction layers
- Reuse `CuAssertTrue`, `CuAssertIntEquals`, `CuAssertStrEquals`, and other built-in asserts
- Match the repository's pointer spacing and put opening braces on their own line in this reference repository
- Use stack objects for local `CuTest`, `CuSuite`, `CuString`, or `CuArray` when existing tests do so
- Use heap allocation only where the current API already returns heap-owned objects
- When calling `CuSuiteAdd(...)` directly, assert its return value with `CuAssertTrue(...)`
- Keep test names explicit about the behavior under test
- Keep tests independent; reset any shared port or stub state within each test's setup

## High-value patterns in CuTest-style repositories

- Verify both state fields and rendered message text when testing assertion failures
- Check resize behavior by crossing the configured threshold, not by duplicating large unrelated setup
- For insert or append operations, validate both resulting content and resulting length
- For suite behavior, validate count ordering, fail count, capacity, storage mode (`inlineList` versus heap list), and textual output when relevant
- For ownership-sensitive paths, clean up heap-owned objects the same way the surrounding tests already do

## Things to avoid

- Do not add generic helper layers for a small number of tests
- Do not rewrite existing suites just to group tests differently
- Do not change production behavior solely to simplify a test unless the user explicitly asked for that design change
