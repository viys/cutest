#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"

/*-------------------------------------------------------------------------*
 * CuArr
 *-------------------------------------------------------------------------*/

unsigned char* CuArrAlloc(size_t size) {
    unsigned char* newArr = (unsigned char*)calloc(size, sizeof(unsigned char));
    return newArr;
}

unsigned char* CuArrCopy(unsigned char* old, size_t len) {
    unsigned char* newArr = CuArrAlloc(len);
    if (newArr == NULL) {
        return NULL;
    }
    memcpy(newArr, old, len);
    return newArr;
}

/*-------------------------------------------------------------------------*
 * CuArray
 *-------------------------------------------------------------------------*/

void CuArrayInit(CuArray* arr) {
    arr->length = 0;
    arr->size = ARRAY_MAX;
    arr->array = CuArrAlloc(arr->size);
}

CuArray* CuArrayNew(void) {
    CuArray* arr = CU_ALLOC(CuArray);
    if (arr == NULL) {
        return NULL;
    }
    arr->length = 0;
    arr->size = ARRAY_MAX;
    arr->array = CuArrAlloc(arr->size);
    return arr;
}

void CuArrayDelete(CuArray* arr) {
    if (!arr)
        return;
    CU_FREE(arr->array);
    CU_FREE(arr);
}

void CuArrayResize(CuArray* arr, size_t newSize) {
    unsigned char* newArray =
        (unsigned char*)realloc(arr->array, sizeof(unsigned char) * newSize);
    if (newArray == NULL && newSize > 0) {
        return;
    }
    arr->array = newArray;
    arr->size = newSize;
    if (arr->length > newSize) {
        arr->length = newSize;
    }
}

static int CuArrayEnsureCapacity(CuArray* arr, size_t needed) {
    size_t newSize;

    if (needed <= arr->size) {
        return 1;
    }

    newSize = arr->size > 0 ? arr->size : ARRAY_MAX;
    while (newSize < needed) {
        size_t nextSize = newSize + ARRAY_INC;
        if (nextSize <= newSize) {
            newSize = needed;
            break;
        }
        newSize = nextSize;
    }

    CuArrayResize(arr, newSize);
    return arr->size >= needed;
}

void CuArrayAppend(CuArray* arr, unsigned char* array, size_t len) {
    if (!CuArrayEnsureCapacity(arr, arr->length + len)) {
        return;
    }
    memmove(arr->array + arr->length, array, len);
    arr->length += len;
}

void CuArrayAppendSingle(CuArray* arr, unsigned char single) {
    unsigned char* singleCopy = &single;
    CuArrayAppend(arr, singleCopy, 1);
}

void CuArrayInsert(CuArray* arr, unsigned char* array, size_t pos, size_t len) {
    if (pos >= arr->length)
        pos = arr->length;
    if (!CuArrayEnsureCapacity(arr, arr->length + len)) {
        return;
    }
    memmove(arr->array + pos + len, arr->array + pos, arr->length - pos);
    arr->length += len;
    memcpy(arr->array + pos, array, len);
}

/*-------------------------------------------------------------------------*
 * CuStr
 *-------------------------------------------------------------------------*/

char* CuStrAlloc(size_t size) {
    char* newStr = (char*)malloc(sizeof(char) * (size));
    return newStr;
}

char* CuStrCopy(const char* old) {
    size_t len = strlen(old);
    char* newStr = CuStrAlloc(len + 1);
    if (newStr == NULL) {
        return NULL;
    }
    strcpy(newStr, old);
    return newStr;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

void CuStringInit(CuString* str) {
    str->length = 0;
    str->size = STRING_MAX;
    str->buffer = (char*)malloc(sizeof(char) * str->size);
    if (str->buffer == NULL) {
        str->size = 0;
        return;
    }
    str->buffer[0] = '\0';
}

CuString* CuStringNew(void) {
    CuString* str = CU_ALLOC(CuString);
    if (str == NULL) {
        return NULL;
    }
    str->length = 0;
    str->size = STRING_MAX;
    str->buffer = (char*)malloc(sizeof(char) * str->size);
    if (str->buffer == NULL) {
        CU_FREE(str);
        return NULL;
    }
    str->buffer[0] = '\0';
    return str;
}

void CuStringDelete(CuString* str) {
    if (!str)
        return;
    CU_FREE(str->buffer);
    CU_FREE(str);
}

void CuStringResize(CuString* str, size_t newSize) {
    char* newBuffer = (char*)realloc(str->buffer, sizeof(char) * newSize);
    if (newBuffer == NULL && newSize > 0) {
        return;
    }
    str->buffer = newBuffer;
    str->size = newSize;
    if (str->length >= newSize) {
        str->length = newSize > 0 ? newSize - 1 : 0;
        if (str->buffer != NULL) {
            str->buffer[str->length] = '\0';
        }
    }
}

static int CuStringEnsureCapacity(CuString* str, size_t needed) {
    size_t newSize;

    if (needed <= str->size) {
        return 1;
    }

    newSize = str->size > 0 ? str->size : STRING_MAX;
    while (newSize < needed) {
        size_t nextSize = newSize + STRING_INC;
        if (nextSize <= newSize) {
            newSize = needed;
            break;
        }
        newSize = nextSize;
    }

    CuStringResize(str, newSize);
    return str->size >= needed;
}

void CuStringAppend(CuString* str, const char* text) {
    size_t length;

    if (text == NULL) {
        text = "NULL";
    }

    length = strlen(text);
    if (!CuStringEnsureCapacity(str, str->length + length + 1)) {
        return;
    }
    memcpy(str->buffer + str->length, text, length + 1);
    str->length += length;
}

void CuStringAppendChar(CuString* str, char ch) {
    char text[2];
    text[0] = ch;
    text[1] = '\0';
    CuStringAppend(str, text);
}

void CuStringAppendFormat(CuString* str, const char* format, ...) {
    va_list argp;
    va_list copy;
    char stackBuffer[HUGE_STRING_LEN];
    char* buffer;
    int length;

    va_start(argp, format);
    va_copy(copy, argp);
    length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0) {
        va_end(argp);
        return;
    }

    if ((size_t)length < sizeof(stackBuffer)) {
        buffer = stackBuffer;
        vsnprintf(buffer, sizeof(stackBuffer), format, argp);
    } else {
        buffer = CuStrAlloc((size_t)length + 1);
        if (buffer == NULL) {
            va_end(argp);
            return;
        }
        vsnprintf(buffer, (size_t)length + 1, format, argp);
    }
    va_end(argp);
    CuStringAppend(str, buffer);
    if (buffer != stackBuffer) {
        CU_FREE(buffer);
    }
}

void CuStringInsert(CuString* str, const char* text, size_t pos) {
    size_t length = strlen(text);
    if (pos > str->length)
        pos = str->length;
    if (!CuStringEnsureCapacity(str, str->length + length + 1)) {
        return;
    }
    memmove(str->buffer + pos + length, str->buffer + pos,
            (str->length - pos) + 1);
    str->length += length;
    memcpy(str->buffer + pos, text, length);
}

static CuString* CuStringNewFromString(CuString* source) {
    CuString* str = CU_ALLOC(CuString);
    if (str == NULL) {
        return NULL;
    }

    *str = *source;
    source->length = 0;
    source->size = 0;
    source->buffer = NULL;
    return str;
}

/*-------------------------------------------------------------------------*
 * CuTest
 *-------------------------------------------------------------------------*/

void CuTestInit(CuTest* t, const char* name, TestFunction function) {
    t->name = CuStrCopy(name);
    t->failed = 0;
    t->ran = 0;
    t->message = NULL;
    t->function = function;
    t->jumpBuf = NULL;
}

CuTest* CuTestNew(const char* name, TestFunction function) {
    CuTest* tc = CU_ALLOC(CuTest);
    if (tc == NULL) {
        return NULL;
    }
    CuTestInit(tc, name, function);
    return tc;
}

void CuTestDelete(CuTest* t) {
    if (!t)
        return;
    CuStringDelete(t->message);
    CU_FREE(t->name);
    CU_FREE(t);
}

void CuTestRun(CuTest* tc) {
    jmp_buf buf;
    tc->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        tc->ran = 1;
        (tc->function)(tc);
    }
    tc->jumpBuf = 0;
}

static void CuFailInternal(CuTest* tc, const char* file, int line,
                           CuString* string) {
    char prefix[HUGE_STRING_LEN];
    int length;

    length = snprintf(prefix, sizeof(prefix), "%s:%d: ", file, line);
    if (length < 0) {
        return;
    }
    if ((size_t)length < sizeof(prefix)) {
        CuStringInsert(string, prefix, 0);
    } else {
        char* buf = CuStrAlloc((size_t)length + 1);
        if (buf == NULL) {
            return;
        }
        snprintf(buf, (size_t)length + 1, "%s:%d: ", file, line);
        CuStringInsert(string, buf, 0);
        CU_FREE(buf);
    }

    tc->failed = 1;
    {
        CuString* message = CuStringNewFromString(string);
        if (message != NULL) {
            CuStringDelete(tc->message);
            tc->message = message;
        } else {
            CU_FREE(string->buffer);
            string->buffer = NULL;
            string->length = 0;
            string->size = 0;
        }
    }
    if (tc->jumpBuf != 0)
        longjmp(*(tc->jumpBuf), 0);
}

void CuFail_Line(CuTest* tc, const char* file, int line, const char* message2,
                 const char* message) {
    CuString string;

    CuStringInit(&string);
    if (message2 != NULL) {
        CuStringAppend(&string, message2);
        CuStringAppend(&string, ": ");
    }
    CuStringAppend(&string, message);
    CuFailInternal(tc, file, line, &string);
}

void CuAssert_Line(CuTest* tc, const char* file, int line, const char* message,
                   int condition) {
    if (condition)
        return;
    CuFail_Line(tc, file, line, NULL, message);
}

void CuAssertArrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, unsigned char* expected,
                               unsigned char* actual, size_t len) {
    size_t i = 0;
    CuString string;

    if ((expected == NULL && actual == NULL) ||
        (expected != NULL && actual != NULL)) {

        for (i = 0; i < len; i++) {
            if (expected[i] != actual[i]) {
                break;
            }
        }

        if (i >= len) {
            return;
        }
    }

    CuStringInit(&string);
    if (message != NULL) {
        CuStringAppend(&string, message);
        CuStringAppend(&string, ": ");
    }
    CuStringAppend(&string, "expected <");
    CuStringAppendFormat(&string, "pos %d: 0x%02x", i, expected[i]);
    CuStringAppend(&string, "> but was <");
    CuStringAppendFormat(&string, "pos %d: 0x%02x", i, actual[i]);
    CuStringAppend(&string, ">");
    CuFailInternal(tc, file, line, &string);
}

void CuAssertStrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, const char* expected,
                               const char* actual) {
    CuString string;
    if ((expected == NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) == 0)) {
        return;
    }

    CuStringInit(&string);
    if (message != NULL) {
        CuStringAppend(&string, message);
        CuStringAppend(&string, ": ");
    }
    CuStringAppend(&string, "expected <");
    CuStringAppend(&string, expected);
    CuStringAppend(&string, "> but was <");
    CuStringAppend(&string, actual);
    CuStringAppend(&string, ">");
    CuFailInternal(tc, file, line, &string);
}

void CuAssertIntEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, int expected, int actual) {
    char buf[STRING_MAX];
    if (expected == actual)
        return;
    sprintf(buf, "expected <%d> but was <%d>", expected, actual);
    CuFail_Line(tc, file, line, message, buf);
}

void CuAssertDblEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, double expected,
                               double actual, double delta) {
    char buf[STRING_MAX];
    if (fabs(expected - actual) <= delta)
        return;
    sprintf(buf, "expected <%f> but was <%f>", expected, actual);

    CuFail_Line(tc, file, line, message, buf);
}

void CuAssertPtrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, void* expected,
                               void* actual) {
    char buf[STRING_MAX];
    if (expected == actual)
        return;
    sprintf(buf, "expected pointer <0x%p> but was <0x%p>", expected, actual);
    CuFail_Line(tc, file, line, message, buf);
}

/*-------------------------------------------------------------------------*
 * CuSuite
 *-------------------------------------------------------------------------*/

void CuSuiteInit(CuSuite* testSuite) {
    testSuite->count = 0;
    testSuite->failCount = 0;
    testSuite->capacity = MAX_TEST_CASES < SUITE_INLINE_CAPACITY
                              ? MAX_TEST_CASES
                              : SUITE_INLINE_CAPACITY;
    testSuite->list = testSuite->inlineList;
    memset(testSuite->inlineList, 0, sizeof(testSuite->inlineList));
}

static int CuSuiteUsesHeapList(CuSuite* testSuite) {
    return testSuite->list != testSuite->inlineList;
}

static void CuSuiteFreeList(CuSuite* testSuite) {
    if (CuSuiteUsesHeapList(testSuite)) {
        CU_FREE(testSuite->list);
    }
    testSuite->list = testSuite->inlineList;
    testSuite->capacity = MAX_TEST_CASES < SUITE_INLINE_CAPACITY
                              ? MAX_TEST_CASES
                              : SUITE_INLINE_CAPACITY;
}

static int CuSuiteEnsureCapacity(CuSuite* testSuite, int needed) {
    int newCapacity;
    CuTest** newList;

    if (needed <= testSuite->capacity) {
        return 1;
    }
    if (needed > MAX_TEST_CASES) {
        return 0;
    }

    newCapacity = testSuite->capacity > 0 ? testSuite->capacity : 1;
    while (newCapacity < needed) {
        int nextCapacity;

        /* Guard the addition before it happens so signed overflow is avoided. */
        if (newCapacity > MAX_TEST_CASES - SUITE_INC) {
            nextCapacity = MAX_TEST_CASES;
        } else {
            nextCapacity = newCapacity + SUITE_INC;
        }
        newCapacity = nextCapacity;
    }

    if (CuSuiteUsesHeapList(testSuite)) {
        newList = (CuTest**)realloc(testSuite->list,
                                    (size_t)newCapacity * sizeof(CuTest*));
    } else {
        newList = (CuTest**)malloc((size_t)newCapacity * sizeof(CuTest*));
        if (newList != NULL && testSuite->count > 0) {
            memcpy(newList, testSuite->list,
                   (size_t)testSuite->count * sizeof(CuTest*));
        }
    }
    if (newList == NULL) {
        return 0;
    }

    if (newCapacity > testSuite->count) {
        memset(newList + testSuite->count, 0,
               (size_t)(newCapacity - testSuite->count) * sizeof(CuTest*));
    }
    testSuite->list = newList;
    testSuite->capacity = newCapacity;
    return 1;
}

CuSuite* CuSuiteNew(void) {
    CuSuite* testSuite = CU_ALLOC(CuSuite);
    if (testSuite == NULL) {
        return NULL;
    }
    CuSuiteInit(testSuite);
    return testSuite;
}

void CuSuiteCleanup(CuSuite* testSuite) {
    if (!testSuite)
        return;
    CuSuiteFreeList(testSuite);
    testSuite->count = 0;
    testSuite->failCount = 0;
    memset(testSuite->inlineList, 0, sizeof(testSuite->inlineList));
}

void CuSuiteDelete(CuSuite* testSuite) {
    unsigned int n;
    if (!testSuite)
        return;
    for (n = 0; n < (unsigned int)testSuite->count; n++) {
        if (testSuite->list[n]) {
            CuTestDelete(testSuite->list[n]);
        }
    }
    CuSuiteCleanup(testSuite);
    CU_FREE(testSuite);
}

int CuSuiteAdd(CuSuite* testSuite, CuTest* testCase) {
    assert(testSuite->count < MAX_TEST_CASES);
    if (testCase == NULL ||
        !CuSuiteEnsureCapacity(testSuite, testSuite->count + 1)) {
        return 0;
    }
    testSuite->list[testSuite->count] = testCase;
    testSuite->count++;
    return 1;
}

void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2) {
    int i;
    int remaining = 0;
    for (i = 0; i < testSuite2->count; ++i) {
        CuTest* testCase = testSuite2->list[i];
        if (CuSuiteAdd(testSuite, testCase)) {
            testSuite2->list[i] = NULL;
        } else {
            testSuite2->list[remaining++] = testCase;
        }
    }
    for (i = remaining; i < testSuite2->count; ++i) {
        testSuite2->list[i] = NULL;
    }
    testSuite2->count = remaining;
}

void CuSuiteRun(CuSuite* testSuite) {
    int i;
    for (i = 0; i < testSuite->count; ++i) {
        CuTest* testCase = testSuite->list[i];
        CuTestRun(testCase);
        if (testCase->failed) {
            testSuite->failCount += 1;
        }
    }
}

void CuSuiteSummary(CuSuite* testSuite, CuString* summary) {
    int i;
    for (i = 0; i < testSuite->count; ++i) {
        CuTest* testCase = testSuite->list[i];
        CuStringAppend(summary, testCase->failed ? "F" : ".");
    }
    CuStringAppend(summary, "\n\n");
}

void CuSuiteDetails(CuSuite* testSuite, CuString* details) {
    int i;
    int failCount = 0;

    if (testSuite->failCount == 0) {
        int passCount = testSuite->count - testSuite->failCount;
        const char* testWord = passCount == 1 ? "test" : "tests";
        CuStringAppendFormat(details, "OK (%d %s)\n", passCount, testWord);
    } else {
        if (testSuite->failCount == 1)
            CuStringAppend(details, "There was 1 failure:\n");
        else
            CuStringAppendFormat(details, "There were %d failures:\n",
                                 testSuite->failCount);

        for (i = 0; i < testSuite->count; ++i) {
            CuTest* testCase = testSuite->list[i];
            if (testCase->failed) {
                failCount++;
                CuStringAppendFormat(details, "%d) %s: %s\n", failCount,
                                     testCase->name, testCase->message->buffer);
            }
        }
        CuStringAppend(details, "\n!!!FAILURES!!!\n");

        CuStringAppendFormat(details, "Runs: %d ", testSuite->count);
        CuStringAppendFormat(details, "Passes: %d ",
                             testSuite->count - testSuite->failCount);
        CuStringAppendFormat(details, "Fails: %d\n", testSuite->failCount);
    }
}
