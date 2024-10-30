#ifndef CU_TEST_H
#define CU_TEST_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#define CUTEST_VERSION "CuTest 1.6.1"

/* Helper functions */

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
    size_t length;
    size_t size;
    unsigned char* array;
} CuArray;

#define ARRAY_MAX 256
#define ARRAY_INC 256

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

#define CU_ALLOC(TYPE)  ((TYPE*)malloc(sizeof(TYPE)))

#define HUGE_STRING_LEN 8192
#define STRING_MAX      256
#define STRING_INC      256

typedef struct {
    size_t length;
    size_t size;
    char* buffer;
} CuString;

void CuStringInit(CuString* str);
CuString* CuStringNew(void);
void CuStringRead(CuString* str, const char* path);
void CuStringAppend(CuString* str, const char* text);
void CuStringAppendChar(CuString* str, char ch);
void CuStringAppendFormat(CuString* str, const char* format, ...);
void CuStringInsert(CuString* str, const char* text, size_t pos);
void CuStringResize(CuString* str, size_t newSize);
void CuStringDelete(CuString* str);

/* CuTest */

typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest*);

struct CuTest {
    char* name;
    TestFunction function;
    int failed;
    int ran;
    CuString* message;
    jmp_buf* jumpBuf;
};

void CuTestInit(CuTest* t, const char* name, TestFunction function);
CuTest* CuTestNew(const char* name, TestFunction function);
void CuTestRun(CuTest* tc);
void CuTestDelete(CuTest* t);

/* Internal versions of assert functions -- use the public versions */
void CuFail_Line(CuTest* tc, const char* file, int line, const char* message2,
                 const char* message);
void CuAssert_Line(CuTest* tc, const char* file, int line, const char* message,
                   int condition);
void CuAssertStrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, const char* expected,
                               const char* actual);
void CuAssertArrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, unsigned char* expected,
                               unsigned char* actual, size_t len);
void CuAssertIntEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, int expected, int actual);
void CuAssertDblEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, double expected,
                               double actual, double delta);
void CuAssertPtrEquals_LineMsg(CuTest* tc, const char* file, int line,
                               const char* message, void* expected,
                               void* actual);

/* public assert functions */

#define CuFail(tc, ms) CuFail_Line((tc), __FILE__, __LINE__, NULL, (ms))
#define CuAssert(tc, ms, cond) \
    CuAssert_Line((tc), __FILE__, __LINE__, (ms), (cond))
#define CuAssertTrue(tc, cond) \
    CuAssert_Line((tc), __FILE__, __LINE__, "assert failed", (cond))

#define CuAssertStrEquals(tc, ex, ac) \
    CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertStrEquals_Msg(tc, ms, ex, ac) \
    CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
#define CuAssertMacroEquals(tc, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, ("Not "#ex), (ex), (ac))
#define CuAssertIntEquals(tc, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertIntEquals_Msg(tc, ms, ex, ac) \
    CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
#define CuAssertDblEquals(tc, ex, ac, dl) \
    CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac), (dl))
#define CuAssertDblEquals_Msg(tc, ms, ex, ac, dl) \
    CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac), (dl))
#define CuAssertArrEquals(tc, ex, ac, len) \
    CuAssertArrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac), (len))
#define CuAssertArrEquals_Msg(tc, ms, ex, ac, len) \
    CuAssertArrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac), (len))
#define CuAssertPtrEquals(tc, ex, ac) \
    CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertPtrEquals_Msg(tc, ms, ex, ac) \
    CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))

#define CuAssertPtrNotNull(tc, p)                                      \
    CuAssert_Line((tc), __FILE__, __LINE__, "null pointer unexpected", \
                  ((p) != NULL))
#define CuAssertPtrNotNull_Msg(tc, msg, p) \
    CuAssert_Line((tc), __FILE__, __LINE__, (msg), ((p) != NULL))

/* CuSuite */

#define MAX_TEST_CASES              1024

#define SUITE_ADD_TEST(SUITE, TEST) CuSuiteAdd(SUITE, CuTestNew(#TEST, TEST))

typedef struct {
    int count;
    CuTest* list[MAX_TEST_CASES];
    int failCount;

} CuSuite;

void CuSuiteInit(CuSuite* testSuite);
CuSuite* CuSuiteNew(void);
void CuSuiteDelete(CuSuite* testSuite);
void CuSuiteAdd(CuSuite* testSuite, CuTest* testCase);
void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2);
void CuSuiteRun(CuSuite* testSuite);
void CuSuiteSummary(CuSuite* testSuite, CuString* summary);
void CuSuiteDetails(CuSuite* testSuite, CuString* details);

#endif /* CU_TEST_H */
