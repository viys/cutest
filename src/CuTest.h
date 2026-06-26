#ifndef CU_TEST_H
#define CU_TEST_H

#include <setjmp.h>
#include <stddef.h>

/**
 * @brief CuTest library version string.
 */
#define CUTEST_VERSION "CuTest 1.7.0"

/* Compiler-related definitions */

#ifdef __CC_ARM                         /* ARM Compiler */
    #define CU_WEAK                     __weak
#elif defined (__riscv)                  /* RISC-V Compiler */
    #define CU_WEAK                     __attribute__((weak))
#elif defined (__IAR_SYSTEMS_ICC__)     /* IAR Compiler */
    #define CU_WEAK                     __weak
#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #define CU_WEAK                     __attribute__((weak))
#elif defined (__ADSPBLACKFIN__)        /* VisualDSP++ Compiler */
    #define CU_WEAK                     __attribute__((weak))
#elif defined (_MSC_VER)
    #define CU_WEAK
#elif defined (__TI_COMPILER_VERSION__)
    #define CU_WEAK
#else
    #error not supported tool chain
#endif

/* Configuration macros */

/**
 * @brief Default allocation unit for @ref CuArray storage growth.
 */
#ifndef ARRAY_INC
#define ARRAY_INC 256
#endif

/**
 * @brief Initial allocation size for @ref CuArray storage.
 */
#ifndef ARRAY_MAX
#define ARRAY_MAX 256
#endif

/**
 * @brief Maximum temporary message length used by internal fixed buffers.
 */
#ifndef HUGE_STRING_LEN
#define HUGE_STRING_LEN 8192
#endif

/**
 * @brief Default allocation unit for @ref CuString buffer growth.
 */
#ifndef STRING_INC
#define STRING_INC 256
#endif

/**
 * @brief Initial allocation size for @ref CuString buffers.
 */
#ifndef STRING_MAX
#define STRING_MAX 256
#endif

/**
 * @brief Maximum number of test cases supported by a suite.
 */
#ifndef MAX_TEST_CASES
#define MAX_TEST_CASES 1024
#endif

/**
 * @brief Number of test case slots embedded directly in a suite.
 */
#ifndef SUITE_INLINE_CAPACITY
#define SUITE_INLINE_CAPACITY 8
#endif

/**
 * @brief Allocation unit for suite test case list growth.
 */
#ifndef SUITE_INC
#define SUITE_INC 8
#endif

#ifndef CUTEST_USE_MEMORY_MIDDLEWARE
#define CUTEST_USE_MEMORY_MIDDLEWARE 0
#endif

/**
 * @brief Allocates a structure instance from the heap.
 *
 * @param TYPE Structure type to allocate.
 */
#ifndef CU_MALLOC
#if CUTEST_USE_MEMORY_MIDDLEWARE
#define CU_MALLOC(SIZE) CuMemoryMalloc((SIZE))
#else
#define CU_MALLOC(SIZE) malloc((SIZE))
#endif
#endif

/**
 * @brief Allocates a zero-initialized heap block through the memory middleware.
 *
 * @param COUNT Number of elements to allocate.
 * @param SIZE Size of each element.
 */
#ifndef CU_CALLOC
#if CUTEST_USE_MEMORY_MIDDLEWARE
#define CU_CALLOC(COUNT, SIZE) CuMemoryCalloc((COUNT), (SIZE))
#else
#define CU_CALLOC(COUNT, SIZE) calloc((COUNT), (SIZE))
#endif
#endif

/**
 * @brief Resizes a heap block through the memory middleware.
 *
 * @param PTR Pointer to the allocation that should be resized.
 * @param SIZE New size in bytes.
 */
#ifndef CU_REALLOC
#if CUTEST_USE_MEMORY_MIDDLEWARE
#define CU_REALLOC(PTR, SIZE) CuMemoryRealloc((PTR), (SIZE))
#else
#define CU_REALLOC(PTR, SIZE) realloc((PTR), (SIZE))
#endif
#endif

/**
 * @brief Allocates a structure instance from the configured memory middleware.
 *
 * @param TYPE Structure type to allocate.
 */
#define CU_ALLOC(TYPE)  ((TYPE*)CU_MALLOC(sizeof(TYPE)))

/**
 * @brief Releases heap memory previously allocated by CuTest.
 *
 * @param PTR Pointer to free.
 */
#ifndef CU_FREE
#if CUTEST_USE_MEMORY_MIDDLEWARE
#define CU_FREE(PTR)    CuMemoryFree((PTR))
#else
#define CU_FREE(PTR)    free((PTR))
#endif
#endif

/* Helper macros */

#define CREATE_ASSERTS(Asserts)                                                \
    static void Asserts(CuTest* tc, const char* message, const char* expected, \
                        CuString* actual) {                                    \
        int mismatch;                                                          \
        if (expected == NULL || actual == NULL) {                              \
            mismatch = (expected != NULL || actual != NULL);                   \
        } else {                                                               \
            const char* front = __FILE__ ":";                                  \
            const size_t frontLen = strlen(front);                             \
            const size_t expectedLen = strlen(expected);                       \
            const char* matchStr = actual->buffer;                             \
            mismatch = (strncmp(matchStr, front, frontLen) != 0);              \
            if (!mismatch) {                                                   \
                matchStr = strchr(matchStr + frontLen, ':');                   \
                mismatch |= (matchStr == NULL || strncmp(matchStr, ": ", 2));  \
                if (!mismatch) {                                               \
                    matchStr += 2;                                             \
                    mismatch |=                                                \
                        (strncmp(matchStr, expected, expectedLen) != 0);       \
                }                                                              \
            }                                                                  \
        }                                                                      \
        CuAssert_Line(tc, __FILE__, __LINE__, message, !mismatch);             \
    }

/* CuArray */

typedef struct {
    /** @brief Number of valid bytes stored in @ref array. */
    size_t length;
    /** @brief Allocated capacity of @ref array in bytes. */
    size_t size;
    /** @brief Heap buffer that stores the byte sequence. */
    unsigned char* array;
} CuArray;

unsigned char* CuArrAlloc(size_t size);
unsigned char* CuArrCopy(unsigned char* old, size_t len);

void CuArrayInit(CuArray* arr);
CuArray* CuArrayNew(void);
void CuArrayAppend(CuArray* arr, unsigned char* array, size_t len);
void CuArrayAppendSingle(CuArray* arr, unsigned char single);
void CuArrayInsert(CuArray* arr, unsigned char* array, size_t pos, size_t len);
void CuArrayResize(CuArray* arr, size_t newSize);
void CuArrayDelete(CuArray* arr);

/* CuString */

char* CuStrAlloc(size_t size);
char* CuStrCopy(const char* old);

typedef struct {
    /** @brief Number of valid characters excluding the null terminator. */
    size_t length;
    /** @brief Allocated capacity of @ref buffer in bytes. */
    size_t size;
    /** @brief Null-terminated character buffer. */
    char* buffer;
} CuString;

/**
 * @brief Initializes a string object with an empty buffer.
 *
 * @param str Pointer to the string object to initialize.
 */
void CuStringInit(CuString* str);
/**
 * @brief Allocates and initializes a new string object.
 *
 * @return Pointer to the new string object.
 */
CuString* CuStringNew(void);
/**
 * @brief Reads the contents of a file into the string.
 *
 * @param str Pointer to the destination string object.
 * @param path Path to the file to read.
 */
void CuStringRead(CuString* str, const char* path);
/**
 * @brief Appends a null-terminated string to the end of the buffer.
 *
 * @param str Pointer to the destination string object.
 * @param text Text to append. A NULL pointer is appended as "NULL".
 */
void CuStringAppend(CuString* str, const char* text);
/**
 * @brief Appends a single character to the end of the buffer.
 *
 * @param str Pointer to the destination string object.
 * @param ch Character to append.
 */
void CuStringAppendChar(CuString* str, char ch);
/**
 * @brief Appends formatted text using printf-style arguments.
 *
 * @param str Pointer to the destination string object.
 * @param format Format string.
 * @param ... Additional arguments referenced by @p format.
 */
void CuStringAppendFormat(CuString* str, const char* format, ...);
/**
 * @brief Inserts text at the specified position in the buffer.
 *
 * @param str Pointer to the destination string object.
 * @param text Text to insert.
 * @param pos Zero-based insertion position. Values past the end append text.
 */
void CuStringInsert(CuString* str, const char* text, size_t pos);
/**
 * @brief Resizes the internal string buffer.
 *
 * @param str Pointer to the string object.
 * @param newSize New buffer size in bytes.
 */
void CuStringResize(CuString* str, size_t newSize);
/**
 * @brief Releases a heap-allocated string object and its buffer.
 *
 * @param str Pointer to the string object to delete.
 */
void CuStringDelete(CuString* str);

/* CuTest */

typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest*);

/**
 * @brief Represents a single test case and its execution state.
 */
struct CuTest {
    /** @brief Human-readable test case name. */
    char* name;
    /** @brief Test entry function. */
    TestFunction function;
    /** @brief Non-zero after an assertion failure occurs. */
    int failed;
    /** @brief Non-zero after the test function has been entered. */
    int ran;
    /** @brief Failure message captured for the latest test run. */
    CuString* message;
    /** @brief Jump target used to abort execution on assertion failure. */
    jmp_buf* jumpBuf;
};

/**
 * @brief Initializes a test case object.
 *
 * @param t Pointer to the test case object to initialize.
 * @param name Test case name.
 * @param function Test function executed by the runner.
 */
void CuTestInit(CuTest* t, const char* name, TestFunction function);
/**
 * @brief Allocates and initializes a new test case object.
 *
 * @param name Test case name.
 * @param function Test function executed by the runner.
 *
 * @return Pointer to the new test case object.
 */
CuTest* CuTestNew(const char* name, TestFunction function);
/**
 * @brief Executes a test case and captures assertion failures.
 *
 * @param tc Pointer to the test case to run.
 */
void CuTestRun(CuTest* tc);
/**
 * @brief Releases a heap-allocated test case object.
 *
 * @param t Pointer to the test case to delete.
 */
void CuTestDelete(CuTest* t);

/* Internal versions of assert functions -- use the public versions */
/**
 * @brief Fails the current test with file and line information.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the failure occurred.
 * @param line Source line where the failure occurred.
 * @param message2 Optional prefix message.
 * @param message Failure message body.
 */
void CuFail_Line(CuTest* tc, const char* file, int line, const char* message2,
                 const char* message);
/**
 * @brief Verifies a condition and fails the test when it evaluates to false.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Failure message used when the condition is false.
 * @param condition Non-zero when the assertion passes.
 */
void CuAssert_Line(CuTest* tc, const char* file, int line, const char* message,
                   int condition);
/**
 * @brief Compares two strings and fails the test when they differ.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Optional user message.
 * @param expected Expected string.
 * @param actual Actual string.
 */
void CuAssertStrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, const char* expected,
                               const char* actual);
/**
 * @brief Compares two byte arrays and fails the test on the first mismatch.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Optional user message.
 * @param expected Expected byte array.
 * @param actual Actual byte array.
 * @param len Number of bytes to compare.
 */
void CuAssertArrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, unsigned char* expected,
                               unsigned char* actual, size_t len);
/**
 * @brief Compares two integers and fails the test when they differ.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Optional user message.
 * @param expected Expected integer value.
 * @param actual Actual integer value.
 */
void CuAssertIntEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, int expected, int actual);
/**
 * @brief Compares two floating-point values within a tolerance.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Optional user message.
 * @param expected Expected floating-point value.
 * @param actual Actual floating-point value.
 * @param delta Allowed absolute difference.
 */
void CuAssertDblEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, double expected,
                               double actual, double delta);
/**
 * @brief Compares two pointers and fails the test when they differ.
 *
 * @param tc Pointer to the current test case.
 * @param file Source file where the assertion is evaluated.
 * @param line Source line where the assertion is evaluated.
 * @param message Optional user message.
 * @param expected Expected pointer value.
 * @param actual Actual pointer value.
 */
void CuAssertPtrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, void* expected,
                               void* actual);

/* public assert functions */

/**
 * @brief Fails the current test immediately with a message.
 */
#define CuFail(tc, ms) CuFail_Line((tc), __FILE__, __LINE__, NULL, (ms))
/**
 * @brief Asserts that a condition is true.
 */
#define CuAssert(tc, ms, cond) \
    CuAssert_Line((tc), __FILE__, __LINE__, (ms), (cond))
/**
 * @brief Asserts that a condition is true using a default message.
 */
#define CuAssertTrue(tc, cond) \
    CuAssert_Line((tc), __FILE__, __LINE__, "assert failed", (cond))

/**
 * @brief Asserts that two strings are equal.
 */
#define CuAssertStrEquals(tc, ex, ac) \
    CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
/**
 * @brief Asserts that two strings are equal with a custom message.
 */
#define CuAssertStrEquals_Msg(tc, ms, ex, ac) \
    CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
/**
 * @brief Asserts that a macro expansion matches the expected integer value.
 */
#define CuAssertMacroEquals(tc, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, ("Not "#ex), (ex), (ac))
/**
 * @brief Asserts that two integers are equal.
 */
#define CuAssertIntEquals(tc, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
/**
 * @brief Asserts that two integers are equal with a custom message.
 */
#define CuAssertIntEquals_Msg(tc, ms, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
/**
 * @brief Asserts that two floating-point values are equal within a tolerance.
 */
#define CuAssertDblEquals(tc, ex, ac, dl) \
    CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac), (dl))
/**
 * @brief Asserts that two floating-point values are equal within a tolerance
 *        and reports a custom message on failure.
 */
#define CuAssertDblEquals_Msg(tc, ms, ex, ac, dl) \
    CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac), (dl))
/**
 * @brief Asserts that two byte arrays are equal.
 */
#define CuAssertArrEquals(tc, ex, ac, len) \
    CuAssertArrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac), (len))
/**
 * @brief Asserts that two byte arrays are equal with a custom message.
 */
#define CuAssertArrEquals_Msg(tc, ms, ex, ac, len) \
    CuAssertArrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac), (len))
/**
 * @brief Asserts that two pointers are equal.
 */
#define CuAssertPtrEquals(tc, ex, ac) \
    CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
/**
 * @brief Asserts that two pointers are equal with a custom message.
 */
#define CuAssertPtrEquals_Msg(tc, ms, ex, ac) \
    CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))

/**
 * @brief Asserts that a pointer is not NULL.
 */
#define CuAssertPtrNotNull(tc, p)                                      \
    CuAssert_Line((tc), __FILE__, __LINE__, "null pointer unexpected", \
                  ((p) != NULL))
/**
 * @brief Asserts that a pointer is not NULL with a custom message.
 */
#define CuAssertPtrNotNull_Msg(tc, msg, p) \
    CuAssert_Line((tc), __FILE__, __LINE__, (msg), ((p) != NULL))

/* CuSuite */

/**
 * @brief Creates a test case from a function and adds it to a suite.
 */
#define SUITE_ADD_TEST(SUITE, TEST) \
    do { \
        CuTest* testCase = CuTestNew(#TEST, TEST); \
        if (!CuSuiteAdd((SUITE), testCase)) { \
            CuTestDelete(testCase); \
        } \
    } while (0)

typedef struct {
    /** @brief Number of valid test entries in @ref list. */
    int count;
    /** @brief Number of test cases that failed during the last run. */
    int failCount;
    /** @brief Allocated capacity of @ref list. */
    int capacity;
    /** @brief Pointer to the active test case storage. */
    CuTest** list;
    /** @brief Small embedded storage used before heap growth is needed. */
    CuTest* inlineList[SUITE_INLINE_CAPACITY];
} CuSuite;

/**
 * @brief Initializes a test suite object.
 *
 * @param testSuite Pointer to the test suite object to initialize.
 */
void CuSuiteInit(CuSuite* testSuite);
/**
 * @brief Allocates and initializes a new test suite object.
 *
 * @return Pointer to the new test suite object.
 */
CuSuite* CuSuiteNew(void);
/**
 * @brief Releases dynamic storage owned by a suite without deleting test cases.
 *
 * @param testSuite Pointer to the suite to clean up.
 */
void CuSuiteCleanup(CuSuite* testSuite);
/**
 * @brief Releases a heap-allocated test suite and its owned test cases.
 *
 * @param testSuite Pointer to the test suite to delete.
 */
void CuSuiteDelete(CuSuite* testSuite);
/**
 * @brief Adds a test case to the suite.
 *
 * @param testSuite Pointer to the destination suite.
 * @param testCase Pointer to the test case to add.
 *
 * @return Non-zero when the test case was added.
 */
int CuSuiteAdd(CuSuite* testSuite, CuTest* testCase);
/**
 * @brief Appends all test cases from one suite into another suite.
 *
 * @param testSuite Pointer to the destination suite.
 * @param testSuite2 Pointer to the source suite. Successfully appended test
 *                   cases move to @p testSuite and are removed from this suite.
 */
void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2);
/**
 * @brief Runs every test case currently stored in the suite.
 *
 * @param testSuite Pointer to the suite to execute.
 */
void CuSuiteRun(CuSuite* testSuite);
/**
 * @brief Writes the compact pass/fail summary string for a suite.
 *
 * @param testSuite Pointer to the suite to summarize.
 * @param summary Pointer to the destination string object.
 */
void CuSuiteSummary(CuSuite* testSuite, CuString* summary);
/**
 * @brief Writes the detailed execution report for a suite.
 *
 * @param testSuite Pointer to the suite to describe.
 * @param details Pointer to the destination string object.
 */
void CuSuiteDetails(CuSuite* testSuite, CuString* details);

#endif /* CU_TEST_H */
