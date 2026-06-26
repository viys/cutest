#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"

#if CUTEST_USE_MEMORY_MIDDLEWARE
#include "memory/CUMemory.h"
#endif

/*-------------------------------------------------------------------------*
 * Helper functions
 *-------------------------------------------------------------------------*/

CREATE_ASSERTS(CompareAsserts)

#if CUTEST_USE_MEMORY_MIDDLEWARE
static size_t PrepareMemoryTestHeap(void) {
    return CuMemoryGetFreeSize();
}

static size_t GetMemoryTestAlignment(void) {
    size_t alignment = (size_t)CUTEST_MEMORY_ALIGNMENT;

    if (alignment == 0) {
        alignment = sizeof(void*);
    }
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

    return alignment;
}

static int IsMemoryTestPointerAligned(void* ptr) {
    return ((uintptr_t)ptr % GetMemoryTestAlignment()) == 0;
}
#endif

/*-------------------------------------------------------------------------*
 * Array Test
 *-------------------------------------------------------------------------*/

void TestCuArrayNew(CuTest* tc) {
    unsigned char testArry[ARRAY_MAX] = {0};
    CuArray* arr = CuArrayNew();
    CuAssertTrue(tc, 0 == arr->length);
    CuAssertTrue(tc, 0 != arr->size);
    CuAssertArrEquals(tc, testArry, arr->array, ARRAY_MAX);
}

#define MACRO_VAL 3

void TestCuArrayAppend(CuTest* tc) {
    unsigned char testArry1[3] = {1, 2, 3};
    unsigned char testArry2[3] = {4, 5, 6};
    unsigned char testArry3[6] = {1, 2, 3, 4, 5, 6};

    CuArray* arr = CuArrayNew();
    CuArrayAppend(arr, testArry1, 3);
    CuAssertIntEquals(tc, 3, (int)arr->length);
    CuAssertMacroEquals(tc, MACRO_VAL, (int)arr->length);
    CuAssertArrEquals(tc, testArry1, arr->array, 3);
    CuArrayAppend(arr, testArry2, 3);
    CuAssertIntEquals(tc, 6, (int)arr->length);
    CuAssertArrEquals(tc, testArry3, arr->array, 6);
}

void TestCuArrayAppendOverlappingSource(CuTest* tc) {
    unsigned char testArry1[3] = {1, 2, 3};
    unsigned char testArry2[6] = {1, 2, 3, 1, 2, 3};
    CuArray* arr = CuArrayNew();

    CuArrayAppend(arr, testArry1, 3);
    CuArrayAppend(arr, arr->array, arr->length);

    CuAssertIntEquals(tc, 6, (int)arr->length);
    CuAssertArrEquals(tc, testArry2, arr->array, 6);

    CuArrayDelete(arr);
}

void TestCuArrayAppendSingle(CuTest* tc) {
    unsigned char testArry1[3] = {1, 2, 3};
    unsigned char testArry3[6] = {1, 2, 3, 4, 5, 6};

    CuArray* arr = CuArrayNew();
    CuArrayAppend(arr, testArry1, 3);
    CuAssertIntEquals(tc, 3, (int)arr->length);
    CuAssertArrEquals(tc, testArry1, arr->array, 3);
    CuArrayAppendSingle(arr, 4);
    CuAssertIntEquals(tc, 4, (int)arr->length);
    CuArrayAppendSingle(arr, 5);
    CuAssertIntEquals(tc, 5, (int)arr->length);
    CuArrayAppendSingle(arr, 6);
    CuAssertIntEquals(tc, 6, (int)arr->length);
    CuAssertArrEquals(tc, testArry3, arr->array, 6);
}

void TestCuArrayInserts(CuTest* tc) {
    unsigned char testArry1[3] = {1, 3, 6};
    unsigned char testArry2[1] = {2};
    unsigned char testArry3[4] = {1, 2, 3, 6};
    unsigned char testArry4[2] = {4, 5};
    unsigned char testArry5[6] = {1, 2, 3, 4, 5, 6};

    CuArray* arr = CuArrayNew();
    CuArrayAppend(arr, testArry1, 3);
    CuArrayInsert(arr, testArry2, 1, 1);
    CuAssertIntEquals(tc, 4, (int)arr->length);
    CuAssertArrEquals(tc, testArry3, arr->array, 4);
    CuArrayInsert(arr, testArry4, 3, 2);
    CuAssertIntEquals(tc, 6, (int)arr->length);
    CuAssertArrEquals(tc, testArry5, arr->array, 4);
}

/* Verify copy semantics preserve content while returning a new buffer. */
void TestCuArrCopy(CuTest* tc) {
    unsigned char testArry1[3] = {1, 2, 3};
    unsigned char* copy = CuArrCopy(testArry1, 3);

    CuAssertTrue(tc, copy != testArry1);
    CuAssertArrEquals(tc, testArry1, copy, 3);

    CU_FREE(copy);
}

/* Verify stack initialization uses the default array capacity. */
void TestCuArrayInit(CuTest* tc) {
    unsigned char testArry[ARRAY_MAX] = {0};
    CuArray arr;

    CuArrayInit(&arr);

    CuAssertIntEquals(tc, 0, (int)arr.length);
    CuAssertIntEquals(tc, ARRAY_MAX, (int)arr.size);
    CuAssertArrEquals(tc, testArry, arr.array, ARRAY_MAX);

    CU_FREE(arr.array);
}

/* Verify inserts beyond the end clamp to tail position and resize when needed. */
void TestCuArrayInsertAtEndAndResize(CuTest* tc) {
    CuArray* arr = CuArrayNew();
    unsigned char tail[4] = {7, 8, 9, 10};

    for (int i = 0; i < ARRAY_MAX; ++i) {
        CuArrayAppendSingle(arr, (unsigned char)i);
    }

    CuArrayInsert(arr, tail, ARRAY_MAX + 10, 4);

    CuAssertIntEquals(tc, ARRAY_MAX + 4, (int)arr->length);
    CuAssertTrue(tc, arr->size >= ARRAY_MAX + 4);
    CuAssertIntEquals(tc, 7, arr->array[ARRAY_MAX]);
    CuAssertIntEquals(tc, 8, arr->array[ARRAY_MAX + 1]);
    CuAssertIntEquals(tc, 9, arr->array[ARRAY_MAX + 2]);
    CuAssertIntEquals(tc, 10, arr->array[ARRAY_MAX + 3]);

    CuArrayDelete(arr);
    CuArrayDelete(NULL);
}

void TestCuArrayResizes(CuTest* tc) {
    CuArray* arr = CuArrayNew();

    for (int i = 0; i < STRING_MAX * 2; i++) {
        CuArrayAppendSingle(arr, 1);
    }
    CuAssertTrue(tc, STRING_MAX * 2 == arr->length);
    CuAssertTrue(tc, STRING_MAX * 2 <= arr->size);
}

/* Verify resize shrink keeps the prefix and clamps the tracked length. */
void TestCuArrayResizeShrink(CuTest* tc) {
    unsigned char initial[5] = {10, 20, 30, 40, 50};
    unsigned char expected[3] = {10, 20, 30};
    CuArray* arr = CuArrayNew();

    CuArrayAppend(arr, initial, 5);
    CuArrayResize(arr, 3);

    CuAssertIntEquals(tc, 3, (int)arr->length);
    CuAssertIntEquals(tc, 3, (int)arr->size);
    CuAssertArrEquals(tc, expected, arr->array, 3);

    CuArrayDelete(arr);
}

CuSuite* CuArrayGetSuite(void) {
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestCuArrayNew);
    SUITE_ADD_TEST(suite, TestCuArrCopy);
    SUITE_ADD_TEST(suite, TestCuArrayInit);
    SUITE_ADD_TEST(suite, TestCuArrayAppend);
    SUITE_ADD_TEST(suite, TestCuArrayAppendOverlappingSource);
    SUITE_ADD_TEST(suite, TestCuArrayAppendSingle);
    SUITE_ADD_TEST(suite, TestCuArrayInserts);
    SUITE_ADD_TEST(suite, TestCuArrayInsertAtEndAndResize);
    SUITE_ADD_TEST(suite, TestCuArrayResizes);
    SUITE_ADD_TEST(suite, TestCuArrayResizeShrink);

    return suite;
}

/*-------------------------------------------------------------------------*
 * CuString Test
 *-------------------------------------------------------------------------*/

void TestCuStringNew(CuTest* tc) {
    CuString* str = CuStringNew();
    CuAssertTrue(tc, 0 == str->length);
    CuAssertTrue(tc, 0 != str->size);
    CuAssertStrEquals(tc, "", str->buffer);
}

void TestCuStringAppend(CuTest* tc) {
    CuString* str = CuStringNew();
    CuStringAppend(str, "hello");
    CuAssertIntEquals(tc, 5, (int)str->length);
    CuAssertStrEquals(tc, "hello", str->buffer);
    CuStringAppend(str, " world");
    CuAssertIntEquals(tc, 11, (int)str->length);
    CuAssertStrEquals(tc, "hello world", str->buffer);
}

void TestCuStringAppendNULL(CuTest* tc) {
    CuString* str = CuStringNew();
    CuStringAppend(str, NULL);
    CuAssertIntEquals(tc, 4, (int)str->length);
    CuAssertStrEquals(tc, "NULL", str->buffer);
}

void TestCuStringAppendChar(CuTest* tc) {
    CuString* str = CuStringNew();
    CuStringAppendChar(str, 'a');
    CuStringAppendChar(str, 'b');
    CuStringAppendChar(str, 'c');
    CuStringAppendChar(str, 'd');
    CuAssertIntEquals(tc, 4, (int)str->length);
    CuAssertStrEquals(tc, "abcd", str->buffer);
}

void TestCuStringInserts(CuTest* tc) {
    CuString* str = CuStringNew();
    CuStringAppend(str, "world");
    CuAssertIntEquals(tc, 5, (int)str->length);
    CuAssertStrEquals(tc, "world", str->buffer);
    CuStringInsert(str, "hell", 0);
    CuAssertIntEquals(tc, 9, (int)str->length);
    CuAssertStrEquals(tc, "hellworld", str->buffer);
    CuStringInsert(str, "o ", 4);
    CuAssertIntEquals(tc, 11, (int)str->length);
    CuAssertStrEquals(tc, "hello world", str->buffer);
    CuStringInsert(str, "!", 11);
    CuAssertIntEquals(tc, 12, (int)str->length);
    CuAssertStrEquals(tc, "hello world!", str->buffer);
}

/* Verify inserts beyond the end clamp to the string tail and trigger resize. */
void TestCuStringInsertAtEndAndResize(CuTest* tc) {
    CuString* str = CuStringNew();
    char suffix[STRING_MAX];

    memset(suffix, 'a', STRING_MAX - 1);
    suffix[STRING_MAX - 1] = '\0';

    CuStringAppend(str, "tail");
    CuStringInsert(str, suffix, STRING_MAX + 5);

    CuAssertIntEquals(tc, STRING_MAX + 3, (int)str->length);
    CuAssertTrue(tc, str->size > STRING_MAX);
    CuAssertTrue(tc, strncmp(str->buffer, "tail", 4) == 0);
    CuAssertTrue(tc, str->buffer[str->length - 1] == 'a');
}

void TestCuStringResizes(CuTest* tc) {
    CuString* str = CuStringNew();
    int i;
    for (i = 0; i < STRING_MAX; ++i) {
        CuStringAppend(str, "aa");
    }
    CuAssertTrue(tc, STRING_MAX * 2 == str->length);
    CuAssertTrue(tc, STRING_MAX * 2 <= str->size);
}

/* Verify resize shrink truncates the string and keeps a terminator in range. */
void TestCuStringResizeShrink(CuTest* tc) {
    CuString str;

    CuStringInit(&str);
    CuStringAppend(&str, "hello");
    CuStringResize(&str, 4);

    CuAssertIntEquals(tc, 3, (int)str.length);
    CuAssertIntEquals(tc, 4, (int)str.size);
    CuAssertStrEquals(tc, "hel", str.buffer);

    CU_FREE(str.buffer);
}

CuSuite* CuStringGetSuite(void) {
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestCuStringNew);
    SUITE_ADD_TEST(suite, TestCuStringAppend);
    SUITE_ADD_TEST(suite, TestCuStringAppendNULL);
    SUITE_ADD_TEST(suite, TestCuStringAppendChar);
    SUITE_ADD_TEST(suite, TestCuStringInserts);
    SUITE_ADD_TEST(suite, TestCuStringInsertAtEndAndResize);
    SUITE_ADD_TEST(suite, TestCuStringResizes);
    SUITE_ADD_TEST(suite, TestCuStringResizeShrink);

    return suite;
}

/*-------------------------------------------------------------------------*
 * CuMemory Test
 *-------------------------------------------------------------------------*/

void TestCuMemoryAllocFreeAccounting(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    unsigned char* first = (unsigned char*)CuMemoryMalloc(32);
    size_t freeAfterFirst = CuMemoryGetFreeSize();
    unsigned char* second = (unsigned char*)CuMemoryMalloc(64);
    size_t freeAfterSecond = CuMemoryGetFreeSize();

    CuAssertPtrNotNull(tc, first);
    CuAssertPtrNotNull(tc, second);
    CuAssertTrue(tc, IsMemoryTestPointerAligned(first));
    CuAssertTrue(tc, IsMemoryTestPointerAligned(second));
    CuAssertTrue(tc, freeAfterFirst < freeBefore);
    CuAssertTrue(tc, freeAfterSecond <= freeAfterFirst);

    CuMemoryFree(second);
    CuMemoryFree(first);

    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryCallocZeroAndOverflow(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    unsigned char* buffer = (unsigned char*)CuMemoryCalloc(32, sizeof(unsigned char));
    size_t overflowCount = (SIZE_MAX / 2) + 1;
    size_t i;

    CuAssertPtrNotNull(tc, buffer);
    for (i = 0; i < 32; ++i) {
        CuAssertIntEquals(tc, 0, buffer[i]);
    }
    CuAssertPtrEquals(tc, NULL, CuMemoryCalloc(overflowCount, 2));
    CuAssertPtrEquals(tc, NULL, CuMemoryCalloc(0, sizeof(unsigned char)));

    CuMemoryFree(buffer);

    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryReallocNullZeroAndData(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    unsigned char* buffer = (unsigned char*)CuMemoryRealloc(NULL, 32);
    unsigned char* grown;
    unsigned char* shrunk;
    size_t i;

    CuAssertPtrNotNull(tc, buffer);
    for (i = 0; i < 32; ++i) {
        buffer[i] = (unsigned char)(i + 1);
    }

    grown = (unsigned char*)CuMemoryRealloc(buffer, 96);
    CuAssertPtrNotNull(tc, grown);
    for (i = 0; i < 32; ++i) {
        CuAssertIntEquals(tc, (int)(i + 1), grown[i]);
    }

    shrunk = (unsigned char*)CuMemoryRealloc(grown, 16);
    CuAssertPtrEquals(tc, grown, shrunk);
    for (i = 0; i < 16; ++i) {
        CuAssertIntEquals(tc, (int)(i + 1), shrunk[i]);
    }

    CuAssertPtrEquals(tc, NULL, CuMemoryRealloc(shrunk, 0));
    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryCoalescesAdjacentBlocks(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    void* extra[128];
    unsigned char* first = (unsigned char*)CuMemoryMalloc(64);
    unsigned char* second = (unsigned char*)CuMemoryMalloc(64);
    unsigned char* third = (unsigned char*)CuMemoryMalloc(64);
    unsigned char* merged;
    int extraCount = 0;

    CuAssertPtrNotNull(tc, first);
    CuAssertPtrNotNull(tc, second);
    CuAssertPtrNotNull(tc, third);

    while (extraCount < (int)(sizeof(extra) / sizeof(extra[0]))) {
        extra[extraCount] = CuMemoryMalloc(64);
        if (extra[extraCount] == NULL) {
            break;
        }
        ++extraCount;
    }

    CuMemoryFree(second);
    CuMemoryFree(third);

    merged = (unsigned char*)CuMemoryMalloc(96);
    CuAssertPtrNotNull(tc, merged);

    CuMemoryFree(merged);
    CuMemoryFree(first);
    while (extraCount-- > 0) {
        CuMemoryFree(extra[extraCount]);
    }

    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryOutOfMemoryAndReallocFailure(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    void* extra[128];
    unsigned char* base = (unsigned char*)CuMemoryMalloc(64);
    unsigned char* grown;
    int extraCount = 0;
    size_t i;
    size_t oversizedRequest;

    CuAssertPtrNotNull(tc, base);
    for (i = 0; i < 64; ++i) {
        base[i] = (unsigned char)(0xA0 + i);
    }

    while (extraCount < (int)(sizeof(extra) / sizeof(extra[0]))) {
        extra[extraCount] = CuMemoryMalloc(64);
        if (extra[extraCount] == NULL) {
            break;
        }
        ++extraCount;
    }

    oversizedRequest = CuMemoryGetFreeSize() + 1;
    CuAssertPtrEquals(tc, NULL, CuMemoryMalloc(oversizedRequest));

    grown = (unsigned char*)CuMemoryRealloc(base, freeBefore + 64);
    CuAssertPtrEquals(tc, NULL, grown);
    for (i = 0; i < 64; ++i) {
        CuAssertIntEquals(tc, (int)(0xA0 + i), base[i]);
    }

    while (extraCount-- > 0) {
        CuMemoryFree(extra[extraCount]);
    }
    CuMemoryFree(base);

    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryMinimumEverFreeSize(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    size_t minimumBefore = CuMemoryGetMinimumEverFreeSize();
    unsigned char* block = (unsigned char*)CuMemoryMalloc(48);
    size_t freeAfterAlloc = CuMemoryGetFreeSize();
    size_t minimumAfterAlloc = CuMemoryGetMinimumEverFreeSize();
    size_t expectedMinimum =
        minimumBefore < freeAfterAlloc ? minimumBefore : freeAfterAlloc;

    CuAssertPtrNotNull(tc, block);
    CuAssertTrue(tc, minimumAfterAlloc == expectedMinimum);

    CuMemoryFree(block);

    CuAssertTrue(tc, CuMemoryGetMinimumEverFreeSize() == expectedMinimum);
    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    CuAssertTrue(tc, 1);
#endif
}

void TestCuMemoryResetReinitializesHeap(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeNow = CuMemoryGetFreeSize();
    size_t minimumNow = CuMemoryGetMinimumEverFreeSize();

    CuAssertTrue(tc, minimumNow <= freeNow);
    CuAssertTrue(tc, freeNow <= CUTEST_MEMORY_HEAP_SIZE);
    return;
#endif

    CuAssertTrue(tc, 1);
}

void TestCuMemoryMacroRouting(CuTest* tc) {
#if CUTEST_USE_MEMORY_MIDDLEWARE
    size_t freeBefore = PrepareMemoryTestHeap();
    unsigned char* block = (unsigned char*)CU_MALLOC(32);
    size_t freeAfterAlloc = CuMemoryGetFreeSize();

    CuAssertPtrNotNull(tc, block);
    CuAssertTrue(tc, freeAfterAlloc < freeBefore);

    CU_FREE(block);
    CuAssertTrue(tc, CuMemoryGetFreeSize() == freeBefore);
#else
    unsigned char* block = (unsigned char*)CU_MALLOC(32);

    CuAssertPtrNotNull(tc, block);
    CU_FREE(block);
#endif
}

/*-------------------------------------------------------------------------*
 * CuTest Test
 *-------------------------------------------------------------------------*/

void TestPasses(CuTest* tc) {
    CuAssert(tc, "test should pass", 1 == 0 + 1);
}

void zTestFails(CuTest* tc) {
    CuAssert(tc, "test should fail", 1 == 1 + 1);
}

void TestCuTestNew(CuTest* tc) {
    CuTest* tc2 = CuTestNew("MyTest", TestPasses);
    CuAssertStrEquals(tc, "MyTest", tc2->name);
    CuAssertTrue(tc, !tc2->failed);
    CuAssertTrue(tc, tc2->message == NULL);
    CuAssertTrue(tc, tc2->function == TestPasses);
    CuAssertTrue(tc, tc2->ran == 0);
    CuAssertTrue(tc, tc2->jumpBuf == NULL);
}

void TestCuTestInit(CuTest* tc) {
    CuTest tc2;
    CuTestInit(&tc2, "MyTest", TestPasses);
    CuAssertStrEquals(tc, "MyTest", tc2.name);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);
    CuAssertTrue(tc, tc2.function == TestPasses);
    CuAssertTrue(tc, tc2.ran == 0);
    CuAssertTrue(tc, tc2.jumpBuf == NULL);
}

void TestCuAssert(CuTest* tc) {
    CuTest tc2;
    CuTestInit(&tc2, "MyTest", TestPasses);

    CuAssert(&tc2, "test 1", 5 == 4 + 1);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);

    CuAssert(&tc2, "test 2", 0);
    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssert didn't fail", "test 2", tc2.message);

    CuAssert(&tc2, "test 3", 1);
    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssert didn't fail", "test 2", tc2.message);

    CuAssert(&tc2, "test 4", 0);
    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssert didn't fail", "test 4", tc2.message);
}

void TestCuAssertPtrEquals_Success(CuTest* tc) {
    CuTest tc2;
    int x;

    CuTestInit(&tc2, "MyTest", TestPasses);

    /* test success case */
    CuAssertPtrEquals(&tc2, &x, &x);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);
}

void TestCuAssertPtrEquals_Failure(CuTest* tc) {
    CuTest tc2;
    int x;
    int* nullPtr = NULL;
    char expected_message[STRING_MAX];

    CuTestInit(&tc2, "MyTest", TestPasses);

    /* test failing case */
    sprintf(expected_message, "expected pointer <0x%p> but was <0x%p>",
            (void*)nullPtr, (void*)&x);
    CuAssertPtrEquals(&tc2, NULL, &x);
    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssertPtrEquals failed", expected_message,
                   tc2.message);
}

void TestCuAssertPtrNotNull_Success(CuTest* tc) {
    CuTest tc2;
    int x;

    CuTestInit(&tc2, "MyTest", TestPasses);

    /* test success case */
    CuAssertPtrNotNull(&tc2, &x);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);
}

void TestCuAssertPtrNotNull_Failure(CuTest* tc) {
    CuTest tc2;

    CuTestInit(&tc2, "MyTest", TestPasses);

    /* test failing case */
    CuAssertPtrNotNull(&tc2, NULL);
    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssertPtrNotNull failed", "null pointer unexpected",
                   tc2.message);
}

void TestCuTestRun(CuTest* tc) {
    CuTest tc2;
    CuTestInit(&tc2, "MyTest", zTestFails);
    CuTestRun(&tc2);

    CuAssertStrEquals(tc, "MyTest", tc2.name);
    CuAssertTrue(tc, tc2.failed);
    CuAssertTrue(tc, tc2.ran);
    CompareAsserts(tc, "TestRun failed", "test should fail", tc2.message);
}

/*-------------------------------------------------------------------------*
 * CuSuite Test
 *-------------------------------------------------------------------------*/

void TestCuSuiteInit(CuTest* tc) {
    CuSuite ts;
    int expectedCapacity = MAX_TEST_CASES < SUITE_INLINE_CAPACITY
                               ? MAX_TEST_CASES
                               : SUITE_INLINE_CAPACITY;

    CuSuiteInit(&ts);

    CuAssertTrue(tc, ts.count == 0);
    CuAssertTrue(tc, ts.failCount == 0);
    CuAssertIntEquals(tc, expectedCapacity, ts.capacity);
    CuAssertTrue(tc, ts.list == ts.inlineList);
}

void TestCuSuiteNew(CuTest* tc) {
    CuSuite* ts = CuSuiteNew();
    int expectedCapacity = MAX_TEST_CASES < SUITE_INLINE_CAPACITY
                               ? MAX_TEST_CASES
                               : SUITE_INLINE_CAPACITY;

    CuAssertTrue(tc, ts->count == 0);
    CuAssertTrue(tc, ts->failCount == 0);
    CuAssertIntEquals(tc, expectedCapacity, ts->capacity);
    CuAssertTrue(tc, ts->list == ts->inlineList);

    CuSuiteDelete(ts);
}

void TestCuSuiteAddTest(CuTest* tc) {
    CuSuite ts;
    CuTest tc2;

    CuSuiteInit(&ts);
    CuTestInit(&tc2, "MyTest", zTestFails);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuAssertTrue(tc, ts.count == 1);

    CuAssertStrEquals(tc, "MyTest", ts.list[0]->name);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteAddSuite(CuTest* tc) {
    CuSuite* ts1 = CuSuiteNew();
    CuSuite* ts2 = CuSuiteNew();

    CuAssertTrue(tc, CuSuiteAdd(ts1, CuTestNew("TestFails1", zTestFails)));
    CuAssertTrue(tc, CuSuiteAdd(ts1, CuTestNew("TestFails2", zTestFails)));

    CuAssertTrue(tc, CuSuiteAdd(ts2, CuTestNew("TestFails3", zTestFails)));
    CuAssertTrue(tc, CuSuiteAdd(ts2, CuTestNew("TestFails4", zTestFails)));

    CuSuiteAddSuite(ts1, ts2);
    CuAssertIntEquals(tc, 4, ts1->count);
    CuAssertIntEquals(tc, 0, ts2->count);

    CuAssertStrEquals(tc, "TestFails1", ts1->list[0]->name);
    CuAssertStrEquals(tc, "TestFails2", ts1->list[1]->name);
    CuAssertStrEquals(tc, "TestFails3", ts1->list[2]->name);
    CuAssertStrEquals(tc, "TestFails4", ts1->list[3]->name);

    CuSuiteDelete(ts2);
    CuSuiteDelete(ts1);
}

void TestCuSuiteGrowsPastInlineCapacity(CuTest* tc) {
    CuSuite* ts = CuSuiteNew();
    int limit = SUITE_INLINE_CAPACITY + 2;

    if (limit > MAX_TEST_CASES) {
        limit = MAX_TEST_CASES;
    }

    for (int i = 0; i < limit; ++i) {
        CuAssertTrue(tc, CuSuiteAdd(ts, CuTestNew("Grow", TestPasses)));
    }

    CuAssertIntEquals(tc, limit, ts->count);
    CuAssertTrue(tc, ts->capacity >= limit);
    if (limit > SUITE_INLINE_CAPACITY) {
        CuAssertTrue(tc, ts->list != ts->inlineList);
    }
    if (limit > 0) {
        CuAssertStrEquals(tc, "Grow", ts->list[limit - 1]->name);
    }

    CuSuiteDelete(ts);
}

void TestCuSuiteCleanupReleasesDynamicList(CuTest* tc) {
    CuSuite ts;
    CuTest tests[SUITE_INLINE_CAPACITY + 2];
    int limit = SUITE_INLINE_CAPACITY + 2;

    if (limit > MAX_TEST_CASES) {
        limit = MAX_TEST_CASES;
    }

    CuSuiteInit(&ts);
    for (int i = 0; i < limit; ++i) {
        CuTestInit(&tests[i], "StackTest", TestPasses);
        CuAssertTrue(tc, CuSuiteAdd(&ts, &tests[i]));
    }

    if (limit > SUITE_INLINE_CAPACITY) {
        CuAssertTrue(tc, ts.list != ts.inlineList);
    }

    CuSuiteCleanup(&ts);

    CuAssertIntEquals(tc, 0, ts.count);
    CuAssertTrue(tc, ts.list == ts.inlineList);

    for (int i = 0; i < limit; ++i) {
        CU_FREE(tests[i].name);
    }
}

void TestCuSuiteRun(CuTest* tc) {
    CuSuite ts;
    CuTest tc1, tc2, tc3, tc4;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestPasses", TestPasses);
    CuTestInit(&tc2, "TestPasses", TestPasses);
    CuTestInit(&tc3, "TestFails", zTestFails);
    CuTestInit(&tc4, "TestFails", zTestFails);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc3));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc4));
    CuAssertTrue(tc, ts.count == 4);

    CuSuiteRun(&ts);
    CuAssertTrue(tc, ts.count - ts.failCount == 2);
    CuAssertTrue(tc, ts.failCount == 2);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteSummary(CuTest* tc) {
    CuSuite ts;
    CuTest tc1, tc2;
    CuString summary;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestPasses", TestPasses);
    CuTestInit(&tc2, "TestFails", zTestFails);
    CuStringInit(&summary);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuSuiteRun(&ts);

    CuSuiteSummary(&ts, &summary);

    CuAssertTrue(tc, ts.count == 2);
    CuAssertTrue(tc, ts.failCount == 1);
    CuAssertStrEquals(tc, ".F\n\n", summary.buffer);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteDetails_SingleFail(CuTest* tc) {
    CuSuite ts;
    CuTest tc1, tc2;
    CuString details;
    const char* front;
    const char* back;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestPasses", TestPasses);
    CuTestInit(&tc2, "TestFails", zTestFails);
    CuStringInit(&details);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuSuiteRun(&ts);

    CuSuiteDetails(&ts, &details);

    CuAssertTrue(tc, ts.count == 2);
    CuAssertTrue(tc, ts.failCount == 1);

    front =
        "There was 1 failure:\n"
        "1) TestFails: ";
    back =
        "test should fail\n"
        "\n!!!FAILURES!!!\n"
        "Runs: 2 Passes: 1 Fails: 1\n";

    CuAssertStrEquals(tc, back,
                      details.buffer + strlen(details.buffer) - strlen(back));
    details.buffer[strlen(front)] = 0;
    CuAssertStrEquals(tc, front, details.buffer);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteDetails_SinglePass(CuTest* tc) {
    CuSuite ts;
    CuTest tc1;
    CuString details;
    const char* expected;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestPasses", TestPasses);
    CuStringInit(&details);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuSuiteRun(&ts);

    CuSuiteDetails(&ts, &details);

    CuAssertTrue(tc, ts.count == 1);
    CuAssertTrue(tc, ts.failCount == 0);

    expected = "OK (1 test)\n";

    CuAssertStrEquals(tc, expected, details.buffer);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteDetails_MultiplePasses(CuTest* tc) {
    CuSuite ts;
    CuTest tc1, tc2;
    CuString details;
    const char* expected;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestPasses", TestPasses);
    CuTestInit(&tc2, "TestPasses", TestPasses);
    CuStringInit(&details);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuSuiteRun(&ts);

    CuSuiteDetails(&ts, &details);

    CuAssertTrue(tc, ts.count == 2);
    CuAssertTrue(tc, ts.failCount == 0);

    expected = "OK (2 tests)\n";

    CuAssertStrEquals(tc, expected, details.buffer);
    CuSuiteCleanup(&ts);
}

void TestCuSuiteDetails_MultipleFails(CuTest* tc) {
    CuSuite ts;
    CuTest tc1, tc2;
    CuString details;
    const char* front;
    const char* mid;
    const char* back;

    CuSuiteInit(&ts);
    CuTestInit(&tc1, "TestFails1", zTestFails);
    CuTestInit(&tc2, "TestFails2", zTestFails);
    CuStringInit(&details);

    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc1));
    CuAssertTrue(tc, CuSuiteAdd(&ts, &tc2));
    CuSuiteRun(&ts);

    CuSuiteDetails(&ts, &details);

    CuAssertTrue(tc, ts.count == 2);
    CuAssertTrue(tc, ts.failCount == 2);

    front =
        "There were 2 failures:\n"
        "1) TestFails1: ";
    mid =
        "test should fail\n"
        "2) TestFails2: ";
    back =
        "test should fail\n"
        "\n!!!FAILURES!!!\n"
        "Runs: 2 Passes: 0 Fails: 2\n";

    CuAssertStrEquals(tc, back,
                      details.buffer + strlen(details.buffer) - strlen(back));
    CuAssert(tc, "Couldn't find middle", strstr(details.buffer, mid) != NULL);
    details.buffer[strlen(front)] = 0;
    CuAssertStrEquals(tc, front, details.buffer);
    CuSuiteCleanup(&ts);
}

/*-------------------------------------------------------------------------*
 * Misc Test
 *-------------------------------------------------------------------------*/

void TestCuStrCopy(CuTest* tc) {
    const char* old = "hello world";
    const char* newStr = CuStrCopy(old);
    CuAssert(tc, "old is new", strcmp(old, newStr) == 0);
}

void TestCuStringAppendFormat(CuTest* tc) {
    int i;
    char* text = CuStrAlloc(301); /* long string */
    CuString* str = CuStringNew();
    for (i = 0; i < 300; ++i)
        text[i] = 'a';
    text[300] = '\0';
    CuStringAppendFormat(str, "%s", text);

    /* buffer limit raised to HUGE_STRING_LEN so no overflow */

    CuAssert(tc, "length of str->buffer is 300", 300 == strlen(str->buffer));

    CU_FREE(text);
    CuStringDelete(str);
}

/* Verify large formatted appends switch to the heap buffer path correctly. */
void TestCuStringAppendFormatHuge(CuTest* tc) {
    size_t textLen = HUGE_STRING_LEN + 32;
    char* text = CuStrAlloc(textLen + 1);
    CuString* str = CuStringNew();

    memset(text, 'b', textLen);
    text[textLen] = '\0';

    CuStringAppendFormat(str, "%s", text);

    CuAssertIntEquals(tc, (int)textLen, (int)str->length);
    CuAssertTrue(tc, str->size >= textLen + 1);
    CuAssertTrue(tc, str->buffer[0] == 'b');
    CuAssertTrue(tc, str->buffer[textLen - 1] == 'b');
    CuAssertTrue(tc, str->buffer[textLen] == '\0');

    CU_FREE(text);
    CuStringDelete(str);
}

void TestCuStringAppendFormatSelfReference(CuTest* tc) {
    CuString* str = CuStringNew();

    CuStringAppend(str, "hello");
    CuStringAppendFormat(str, "%s", str->buffer);

    CuAssertStrEquals(tc, "hellohello", str->buffer);

    CuStringDelete(str);
}

void TestFail(CuTest* tc) {
    jmp_buf buf;
    int pointReached = 0;
    CuTest* tc2 = CuTestNew("TestFails", zTestFails);
    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuFail(tc2, "hello world");
        pointReached = 1;
    }
    CuAssert(tc, "point was not reached", pointReached == 0);
}

void TestAssertStrEquals(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertStrEquals", zTestFails);

    const char* expected = "expected <hello> but was <world>";
    const char* expectedMsg = "some text: expected <hello> but was <world>";

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertStrEquals(tc2, "hello", "world");
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals failed", expected, tc2->message);
    if (setjmp(buf) == 0) {
        CuAssertStrEquals_Msg(tc2, "some text", "hello", "world");
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals failed", expectedMsg, tc2->message);
}

void TestAssertStrEquals_NULL(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertStrEquals_NULL", zTestFails);

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertStrEquals(tc2, NULL, NULL);
    }
    CuAssertTrue(tc, !tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_NULL failed", NULL, tc2->message);
    if (setjmp(buf) == 0) {
        CuAssertStrEquals_Msg(tc2, "some text", NULL, NULL);
    }
    CuAssertTrue(tc, !tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_NULL failed", NULL, tc2->message);
}

void TestAssertStrEquals_FailNULLStr(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertStrEquals_FailNULLStr", zTestFails);

    const char* expected = "expected <hello> but was <NULL>";
    const char* expectedMsg = "some text: expected <hello> but was <NULL>";

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertStrEquals(tc2, "hello", NULL);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_FailNULLStr failed", expected,
                   tc2->message);
    if (setjmp(buf) == 0) {
        CuAssertStrEquals_Msg(tc2, "some text", "hello", NULL);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_FailNULLStr failed", expectedMsg,
                   tc2->message);
}

void TestAssertStrEquals_FailStrNULL(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertStrEquals_FailStrNULL", zTestFails);

    const char* expected = "expected <NULL> but was <hello>";
    const char* expectedMsg = "some text: expected <NULL> but was <hello>";

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertStrEquals(tc2, NULL, "hello");
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_FailStrNULL failed", expected,
                   tc2->message);
    if (setjmp(buf) == 0) {
        CuAssertStrEquals_Msg(tc2, "some text", NULL, "hello");
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals_FailStrNULL failed", expectedMsg,
                   tc2->message);
}

void TestAssertIntEquals(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertIntEquals", zTestFails);
    const char* expected = "expected <42> but was <32>";
    const char* expectedMsg = "some text: expected <42> but was <32>";
    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertIntEquals(tc2, 42, 32);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertIntEquals failed", expected, tc2->message);
    if (setjmp(buf) == 0) {
        CuAssertIntEquals_Msg(tc2, "some text", 42, 32);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertStrEquals failed", expectedMsg, tc2->message);
}

void TestAssertDblEquals(CuTest* tc) {
    jmp_buf buf;
    double x = 3.33;
    double y = 10.0 / 3.0;
    CuTest* tc2 = CuTestNew("TestAssertDblEquals", zTestFails);
    char expected[STRING_MAX];
    char expectedMsg[STRING_MAX];
    sprintf(expected, "expected <%lf> but was <%lf>", x, y);
    sprintf(expectedMsg, "some text: expected <%lf> but was <%lf>", x, y);

    CuTestInit(tc2, "TestAssertDblEquals", TestPasses);

    CuAssertDblEquals(tc2, x, x, 0.0);
    CuAssertTrue(tc, !tc2->failed);
    CuAssertTrue(tc, tc2->message == NULL);

    CuAssertDblEquals(tc2, x, y, 0.01);
    CuAssertTrue(tc, !tc2->failed);
    CuAssertTrue(tc, tc2->message == NULL);

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertDblEquals(tc2, x, y, 0.001);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertDblEquals failed", expected, tc2->message);
    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertDblEquals_Msg(tc2, "some text", x, y, 0.001);
    }
    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertDblEquals failed", expectedMsg, tc2->message);
}

/* Verify array assert failures report the first mismatched position and value. */
void TestAssertArrEquals_Failure(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestAssertArrEquals_Failure", zTestFails);
    unsigned char expected[4] = {1, 2, 3, 4};
    unsigned char actual[4] = {1, 9, 3, 4};
    const char* expectedMsg =
        "array mismatch: expected <pos 1: 0x02> but was <pos 1: 0x09>";

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuAssertArrEquals_Msg(tc2, "array mismatch", expected, actual, 4);
    }

    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuAssertArrEquals failed", expectedMsg, tc2->message);
}

/* Verify array equality accepts matching buffers and paired NULL inputs. */
void TestAssertArrEquals_Success(CuTest* tc) {
    CuTest tc2;
    unsigned char expected[4] = {1, 2, 3, 4};
    unsigned char actual[4] = {1, 2, 3, 4};

    CuTestInit(&tc2, "TestAssertArrEquals_Success", TestPasses);

    CuAssertArrEquals(&tc2, expected, actual, 4);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);

    CuAssertArrEquals(&tc2, NULL, NULL, 0);
    CuAssertTrue(tc, !tc2.failed);
    CuAssertTrue(tc, tc2.message == NULL);

    CU_FREE(tc2.name);
}

/* Verify pointer equality failures keep the caller-supplied message prefix. */
void TestCuAssertPtrEquals_Msg_Failure(CuTest* tc) {
    CuTest tc2;
    int expectedValue = 1;
    int actualValue = 2;
    char expectedMessage[STRING_MAX];

    CuTestInit(&tc2, "TestCuAssertPtrEquals_Msg_Failure", TestPasses);

    sprintf(expectedMessage,
            "pointer mismatch: expected pointer <0x%p> but was <0x%p>",
            (void*)&expectedValue, (void*)&actualValue);
    CuAssertPtrEquals_Msg(&tc2, "pointer mismatch", &expectedValue,
                          &actualValue);

    CuAssertTrue(tc, tc2.failed);
    CompareAsserts(tc, "CuAssertPtrEquals_Msg failed", expectedMessage,
                   tc2.message);

    CU_FREE(tc2.name);
    CuStringDelete(tc2.message);
}

/* Verify not-null failures keep the caller-supplied message prefix. */
void TestCuAssertPtrNotNull_Msg_Failure(CuTest* tc) {
    CuTest tc2;

    CuTestInit(&tc2, "TestCuAssertPtrNotNull_Msg_Failure", TestPasses);

    CuAssertPtrNotNull_Msg(&tc2, "pointer missing", NULL);

    CuAssertTrue(tc, tc2.failed);
    CuAssertPtrNotNull(tc, tc2.message);
    CuAssert(tc, "CuAssertPtrNotNull_Msg failed",
             strstr(tc2.message->buffer, "pointer missing") != NULL);

    CU_FREE(tc2.name);
    CuStringDelete(tc2.message);
}

/* Verify CuFail_Line keeps both the prefix message and payload. */
void TestCuFailLine_MessagePrefix(CuTest* tc) {
    jmp_buf buf;
    CuTest* tc2 = CuTestNew("TestCuFailLine_MessagePrefix", zTestFails);
    const char* expected = "prefix: payload";

    tc2->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        CuFail_Line(tc2, __FILE__, __LINE__, "prefix", "payload");
    }

    CuAssertTrue(tc, tc2->failed);
    CompareAsserts(tc, "CuFail_Line prefix failed", expected, tc2->message);

    CuTestDelete(tc2);
}

/* Verify CuTestDelete tolerates both populated and null test pointers. */
void TestCuTestDelete(CuTest* tc) {
    CuTest* tc2 = CuTestNew("DeleteMe", TestPasses);

    CuFail_Line(tc2, __FILE__, __LINE__, "prefix", "payload");
    CuTestDelete(tc2);
    CuTestDelete(NULL);

    CuAssertTrue(tc, 1);
}

/* Verify CuSuiteDelete releases owned tests without crashing. */
void TestCuSuiteDelete(CuTest* tc) {
    CuSuite* suite = CuSuiteNew();

    CuAssertTrue(tc, CuSuiteAdd(suite, CuTestNew("Delete1", TestPasses)));
    CuAssertTrue(tc, CuSuiteAdd(suite, CuTestNew("Delete2", zTestFails)));
    CuSuiteDelete(suite);

    CuAssertTrue(tc, 1);
}

/* Verify suites reject null test cases and keep their state unchanged. */
void TestCuSuiteAddRejectsNull(CuTest* tc) {
    CuSuite ts;

    CuSuiteInit(&ts);

    CuAssertTrue(tc, !CuSuiteAdd(&ts, NULL));
    CuAssertIntEquals(tc, 0, ts.count);
    CuAssertIntEquals(tc, 0, ts.failCount);

    CuSuiteCleanup(&ts);
    CuSuiteCleanup(NULL);
}

/* Verify empty suites still emit stable summary and detail output. */
void TestCuSuiteEmptyOutput(CuTest* tc) {
    CuSuite ts;
    CuString summary;
    CuString details;

    CuSuiteInit(&ts);
    CuStringInit(&summary);
    CuStringInit(&details);

    CuSuiteSummary(&ts, &summary);
    CuSuiteDetails(&ts, &details);

    CuAssertStrEquals(tc, "\n\n", summary.buffer);
    CuAssertStrEquals(tc, "OK (0 tests)\n", details.buffer);

    CU_FREE(summary.buffer);
    CU_FREE(details.buffer);
    CuSuiteCleanup(&ts);
}

