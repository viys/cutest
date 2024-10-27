# CuTest 简介

Cutest 是一个轻量级的 C/C++ 单元测试框架，旨在提供简单、易用的测试功能。它的主要特点包括：

1. 简洁性：Cutest 以简洁的语法使得编写测试用例变得容易，降低了学习曲线。
2. 灵活性：支持多种测试风格，可以根据需要进行定制。
3. 单头文件：Cutest 仅包含一个头文件，便于集成到现有项目中。
4. 断言支持：提供多种断言宏，帮助验证代码的行为。
5. 报告生成：在测试执行后，Cutest 可以生成详细的测试报告，便于查看测试结果。

# CuTest 解析

## 项目地址

https://github.com/viys/cutest

> cutest 官网无法下载到源码, 遂作出更改并整理

```Bash
# 运行项目
cd cutest/test/
mkdir build
cd build/
cmake ..        # windows 使用 cmake -G "MinGW Makefiles" ..
make
./cutest
```

## CuTest 库函数解析

### CREATE_ASSERTS

> 宏函数创建辅助函数

```C
/* Helper functions */

#define CREATE_ASSERTS(Asserts) \
void Asserts(CuTest* tc, const char* message, const char* expected, CuString *actual) { \
    int mismatch; \
    if (expected == NULL || actual == NULL) { \
        mismatch = (expected != NULL || actual != NULL); \
    } else { \
        const char *front = __FILE__ ":"; \
        const size_t frontLen = strlen(front); \
        const size_t expectedLen = strlen(expected); \
        const char *matchStr = actual->buffer; \
        mismatch = (strncmp(matchStr, front, frontLen) != 0); \
        if (!mismatch) { \
            matchStr = strchr(matchStr + frontLen, ':'); \
            mismatch |= (matchStr == NULL || strncmp(matchStr, ": ", 2)); \
            if (!mismatch) { \
                matchStr += 2; \
                mismatch |= (strncmp(matchStr, expected, expectedLen) != 0); \
            } \
        } \
    } \
    CuAssert_Line(tc, __FILE__, __LINE__, message, !mismatch); \
}
```

#### 函数说明使用

```C
CREATE_ASSERTS(CompareAsserts)    // 创建名为 CompareAsserts 的辅助函数
```

### CuString 相关

> `strlen()` 函数计算字符串长度时，它会返回字符串中字符的数量，不包括终止符。

```C
/* CuString */

char* CuStrAlloc(size_t size);
char* CuStrCopy(const char* old);

#define CU_ALLOC(TYPE)      ((TYPE*) malloc(sizeof(TYPE)))

#define HUGE_STRING_LEN 8192
#define STRING_MAX      256
#define STRING_INC      256

typedef struct
{
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
```

#### CuString 对象

> 由 `CuStringNew()` 函数创建或使用 `CuStringInit()` 手动初始化 ，为该库基础的字符串相关的类型。
>
> 凡涉及该对象的 buffer 成员所指向的数组中均已包含字符串终止符 `0` 。

```C
typedef struct
{
    size_t length;    // 字符串的长度，不包含 /0 即终止符
    size_t size;      // 开辟的字符串空间大小
    char* buffer;     // 指向字符串的指针
} CuString;
```

#### char* CuStrAlloc (size_t size)

> 创建 `size` 大小的字符串空间, 创建字符串空间的时候涉及到 `/0` 时，`szie` 应比最大字符串字符数大1。

```C
char* text = CuStrAlloc(301);    // 创建 301 个字节大小的字符串空间
```

#### char* CuStrCopy (const char* old)

> 创建可以容纳制定字符串大小的空间，该空间大小为指定字符串字符数 +1。

```C
char* text = CuStrCopy("12345");    // 创建 5+1 个字节的字符串空间
```

#### void CuStringInit (CuString* str)

> 初始化 `CuString` 对象并创建 `STRING_MAX` 大小的内存空间，默认为 256 字节。

```C
CuString str;
CuStringInit(&str);    // 初始化 CuString 对象
```

#### CuString* CuStringNew (void)

> 创建初始化后的 `CuString` 对象并返回其地址。
>
> 其对应的内存空间同样为 `STRING_MAX` 字节。

```C
CuString* str = CuStringNew();    // 创建初始化后的 CuString 对象
```

#### void CuStringRead (CuString* str, const char* path)

> 需使用者自行构建。
>
> 例如从自己的数据源或文件中读取相关的字符串。

#### void CuStringAppend (CuString* str, const char* text)

> 字符串拼接函数。
>
> 当预先分配的内存不足时重新分配更大的内存空间，新的空间大小默认为 `所有字符数+1+STRING_INC` 。

```C
CuString* str = CuStringNew();
CuStringAppend(str, "hello");     // 给空的字符串拼接上 "hello"
CuStringAppend(str, " world");    // 给 "hello" 字符串拼接上 " world"
```

#### void CuStringAppendChar (CuString* str, char ch)

> 字符拼接函数。
>
> 当预先分配的内存不足时重新分配更大的内存空间，新的空间大小默认为 `所有字符数+1+STRING_INC` 。
>
> 拼接完成后依然是一个完整的字符串，其存在字符串终止符 `/0` 。

```C
CuString* str = CuStringNew();
CuStringAppendChar(str, 'a');    // 给空的字符串拼接上 'a' 字符
CuStringAppendChar(str, 'b');    // 给字符串 "a" 拼接上 'b' 字符
CuStringAppendChar(str, 'c');    // 给字符串 "ab" 拼接上 'c' 字符
CuStringAppendChar(str, 'd');    // 给字符串 "abc" 拼接上 'd' 字符
```

#### void CuStringAppendFormat (CuString* str, const char* format, ...)

> 使用变参格式化字符串并追加到 `str` 。
>
> 当 `CuString` 对象的内存空间不足时其同样会自动扩容。
>
> 字符串格式化时开辟了一个大小为 `HUGE_STRING_LEN` 的临时变量空间，可根据自己的场景需求改变该值。

```C
CuString* str = CuStringNew();
CuStringAppendFormat(str, "%s", "hello");    // 给空的字符串拼接上 "hello"
```

#### void CuStringInsert (CuString* str, const char* text, size_t pos)

> 在指定位置插入字符串。
>
> 当 `CuString` 对象的内存空间不足时其同样会自动扩容。

```C
CuString* str = CuStringNew();
CuStringAppend(str, "world");      // 给空的字符串拼接上 "hello"
CuStringInsert(str, "hell", 0);    // 在 0 处插入字符串 "hell" , 成为字符串 "hellworld"
CuStringInsert(str, "o ", 4);      // 在 4 处插入字符串 "o " , 成为字符串 "hello world"
CuStringInsert(str, "!", 11);      // 在 11 处插入字符串 "!" , 成为字符串 "hello world!"
```

#### void CuStringResize (CuString* str, size_t newSize)

> 重新分配 `CuString` 对象内存空间。

```C
CuString* str = CuStringNew();
CuStringResize(str, STRING_INC * 2);    // 重新分配 CuString 对象的内存空间
```

#### void CuStringDelete (CuString *str)

> 删除（释放） `CuString` 对象。

```C
CuString* str = CuStringNew();
CuStringDelete(str);    // 删除 CuString 对象
```

### CuTest 相关

```C
typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest *);

struct CuTest
{
    char* name;
    TestFunction function;
    int failed;
    int ran;
    CuString *message;
    jmp_buf *jumpBuf;
};

void CuTestInit(CuTest* t, const char* name, TestFunction function);
CuTest* CuTestNew(const char* name, TestFunction function);
void CuTestRun(CuTest* tc);
void CuTestDelete(CuTest *t);
```

#### CuTest 对象

> 测试样例。

```C
typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest *);

struct CuTest
{
    char* name;              // 测试用例的名称，用于在测试报告中标识测试
    TestFunction function;   // 指向测试用例函数的指针，该函数包含了测试逻辑
    int failed;              // 表示测试用例是否失败的标记，如果为非零则表示失败
    int ran;                 // 表示测试用例是否已经执行的标记，如果为非零则表示已执行
    CuString *message;       // 指向CuString结构的指针，用于存储测试失败时的错误消息
    jmp_buf *jumpBuf;        // 指向jmp_buf结构的指针，用于异常处理和测试失败时的跳转
};
```

#### void CuTestInit (CuTest* t, const char* name, TestFunction function)

> 初始化 `CuTest` 对象。

```C
CuTest tc;
CuTestInit(&tc, "MyTest", TestFunction);    // 初始化 CuTest 对象
```

#### CuTest* CuTestNew (const char* name, TestFunction function)

> 创建并初始化 `CuTest` 对象。

```C
CuTest* tc = CuTestNew("MyTest", TestFunction);    // 创建并初始化 CuTest 对象
```

#### void CuTestRun (CuTest* tc);

> 运行测试样例。

```C
CuTest* tc = CuTestNew("MyTest", TestFunction);    // 创建并初始化 CuTest 对象
CuTestRun(tc);                                     // 运行测试样例
```

#### void CuTestDelete (CuTest *t);

> 删除（释放） `CuTest` 对象。

```C
CuTest* tc = CuTestNew("MyTest", TestFunction);    // 创建并初始化 CuTest 对象
CuTestDelete(tc);
```

### CuSuite 相关

```C
/* CuSuite */

#define MAX_TEST_CASES  1024

#define SUITE_ADD_TEST(SUITE,TEST)  CuSuiteAdd(SUITE, CuTestNew(#TEST, TEST))

typedef struct
{
    int count;
    CuTest* list[MAX_TEST_CASES];
    int failCount;

} CuSuite;

void CuSuiteInit(CuSuite* testSuite);
CuSuite* CuSuiteNew(void);
void CuSuiteDelete(CuSuite *testSuite);
void CuSuiteAdd(CuSuite* testSuite, CuTest *testCase);
void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2);
void CuSuiteRun(CuSuite* testSuite);
void CuSuiteSummary(CuSuite* testSuite, CuString* summary);
void CuSuiteDetails(CuSuite* testSuite, CuString* details);
```

#### CuSuite 对象

> 测试套件。

```C
#define MAX_TEST_CASES  1024    // 定义了测试套件中可以包含的最大测试用例数量

typedef struct
{
    int count;                       // 测试套件中当前注册的测试用例数量
    CuTest* list[MAX_TEST_CASES];    // 数组，存储测试套件中的所有测试用例指针
    int failCount;                   // 测试套件中失败的测试用例数量

} CuSuite;
```

#### void CuSuiteInit (CuSuite* testSuite)

> 初始化 `CuSuite` 对象。

```C
CuSuite suite；
CuSuiteInit(suite);    // 初始化 CuSuite 对象
```

#### CuSuite* CuSuiteNew (void)

> 创建并初始化 `CuSuite` 对象。

```C
CuSuite* suite = CuSuiteNew();    // 创建并初始化 CuSuite 对象
```

#### void CuSuiteDelete (CuSuite *testSuite)

> 删除 `CuSuite` 对象。

```C
CuSuite* suite = CuSuiteNew();    // 创建并初始化 CuSuite 对象
CuSuiteDelete(suite);             // 删除 CuSuite 对象
```

#### void CuSuiteAdd (CuSuite* testSuite, CuTest *testCase)

> 给测试套件添加测试样例。

```C
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc1 = CuTestNew("MyTest1", TestFunction1);    // 创建样例 1
CuTest* tc2 = CuTestNew("MyTest2", TestFunction2);    // 创建样例 2

CuSuiteAdd(&ts, &tc1);                                // 给测试套件添加测试样例 1
CuSuiteAdd(&ts, &tc2);                                // 给测试套件添加测试样例 2
```

#### SUITE_ADD_TEST (SUITE,TEST)

> 快速添加测试样例。

```C
    CuSuite* suite = CuSuiteNew();           // 创建测试套件

    SUITE_ADD_TEST(suite, TestFunction1);    // 给测试套件添加测试样例 1
    SUITE_ADD_TEST(suite, TestFunction2);    // 给测试套件添加测试样例 2
```

#### void CuSuiteAddSuite (CuSuite* testSuite, CuSuite* testSuite2)

> 测试套件拼接。

```C
    CuSuite* ts1 = CuSuiteNew();                             // 创建测试套件 1
    CuSuite* ts2 = CuSuiteNew();                             // 创建测试套件 2

    CuSuiteAdd(ts1, CuTestNew("MyTest1", TestFunction1));    // 给测试套件 1 添加测试样例 1
    CuSuiteAdd(ts1, CuTestNew("MyTest2", TestFunction2));    // 给测试套件 1 添加测试样例 2

    CuSuiteAdd(ts2, CuTestNew("MyTest3", TestFunction2));    // 给测试套件 2 添加测试样例 3
    CuSuiteAdd(ts2, CuTestNew("MyTest4", TestFunction4));    // 给测试套件 2 添加测试样例 4

    CuSuiteAddSuite(ts1, ts2);                               // 拼接测试样例
```

#### void CuSuiteRun (CuSuite* testSuite)

> 运行测试套件。

```C
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc = CuTestNew("MyTest", TestFunction);       // 创建样例

CuSuiteAdd(&ts, &tc);                                 // 给测试套件添加测试样例
CuSuiteRun(&ts);                                      // 运行测试套件
```

#### void CuSuiteSummary (CuSuite* testSuite, CuString* summary)

> 获取测试套间的测试总结（返回值）。

```C
CuString* summary = CuStringNew();                    // 创建初始化后的 CuString 对象
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc = CuTestNew("MyTest", TestFunction);       // 创建样例

CuSuiteAdd(&ts, &tc);                                 // 给测试套件添加测试样例
CuSuiteRun(&ts);                                      // 运行测试套件
CuSuiteSummary(&ts, &summary);                        // 获取测试套间的测试总结
```

#### void CuSuiteDetails (CuSuite* testSuite, CuString* details)

> 获取测试套件详细的测试结果。

```C
CuString* details = CuStringNew();                    // 创建初始化后的 CuString 对象
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc = CuTestNew("MyTest", TestFunction);       // 创建样例

CuSuiteAdd(&ts, &tc);                                 // 给测试套件添加测试样例
CuSuiteRun(&ts);                                      // 运行测试套件
CuSuiteDetails(&ts, &details);                        // 获取测试套件详细的测试结果
```

### Assert 公共断言相关

> 验证测试用例的预期结果是否与实际结果相符。
>
> 该部分由宏进行了一层封装，其返回值都为 `void` 类型。
>
> 它们的第一个参数全为 `CuTest` 测试样例对象。
>
> 使用方法：
>
> ```C
> void TestFunction(CuTest* tc) {
>     CuAssertxxx(tc, ...);
> }
> ```

```C
#define CuFail(tc, ms)                        CuFail_Line(  (tc), __FILE__, __LINE__, NULL, (ms))
#define CuAssert(tc, ms, cond)                CuAssert_Line((tc), __FILE__, __LINE__, (ms), (cond))
#define CuAssertTrue(tc, cond)                CuAssert_Line((tc), __FILE__, __LINE__, "assert failed", (cond))

#define CuAssertStrEquals(tc,ex,ac)           CuAssertStrEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define CuAssertStrEquals_Msg(tc,ms,ex,ac)    CuAssertStrEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))
#define CuAssertIntEquals(tc,ex,ac)           CuAssertIntEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define CuAssertIntEquals_Msg(tc,ms,ex,ac)    CuAssertIntEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))
#define CuAssertDblEquals(tc,ex,ac,dl)        CuAssertDblEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac),(dl))
#define CuAssertDblEquals_Msg(tc,ms,ex,ac,dl) CuAssertDblEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac),(dl))
#define CuAssertPtrEquals(tc,ex,ac)           CuAssertPtrEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define CuAssertPtrEquals_Msg(tc,ms,ex,ac)    CuAssertPtrEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))

#define CuAssertPtrNotNull(tc,p)        CuAssert_Line((tc),__FILE__,__LINE__,"null pointer unexpected",((p) != NULL))
#define CuAssertPtrNotNull_Msg(tc,msg,p) CuAssert_Line((tc),__FILE__,__LINE__,(msg),((p) != NULL))
```

#### CuFail (tc, ms)

> **标记失败函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ms` 是一个字符串，描述了为什么测试失败。
>
> 当调用 `CuFail` 时，它会立即停止当前测试用例的执行，并将测试标记为失败。这允许开发者在测试过程中的任何地方插入断言，如果特定的条件没有满足，就可以立即报告错误。

函数使用举例：

```C
CuFail(ts, "massage");
```

#### CuAssert (tc, ms, cond)

> **条件断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ms` 是一个字符串，描述了失败时的额外信息。
>
> `cond` 是需要检查的条件表达式。如果这个表达式的计算结果为真（非零），则测试通过；如果结果为假（零），则测试失败，并记录错误信息，包括文件名和行号以及提供的错误消息。

函数使用举例：

```C
CuAssert(tc, "message", true);
```

#### CuAssertTrue (tc, cond)

> **条件判断断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `cond`是需要检查的条件表达式。
>
> 如果这个表达式的结果为真（非零），则测试通过；如果结果为假（零），则测试失败。
>
> 测试失败时，会记录错误信息，包括文件名和行号。

函数使用举例：

```C
CuAssertTrue(tc, 0 == 0);
```

#### CuAssertStrEquals (tc,ex,ac)

> **字符串对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是期望的字符串。
>
> `ac` 是实际的字符串，即被测试函数的返回值或某个变量的值。
>
> 如果 `ex` 和 `ac` 两个字符串相等，测试通过；如果不相等，测试失败，并记录错误信息，包括文件名、行号以及预期值和实际值。

函数使用举例：

```C
CuAssertStrEquals(tc, "MyTest", "MyTest");
```

#### CuAssertStrEquals_Msg (tc,ms,ex,ac)

> 在 `CuAssertStrEquals()` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertStrEquals(tc, "massage", "MyTest", "MyTest");
```

#### CuAssertIntEquals (tc,ex,ac)

> **整数对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是期望的整数。
>
> `ac` 是实际的整数，即被测试函数的返回值或某个变量的值。
>
> 如果 `ex` 和 `ac` 两个整数相等，测试通过；如果不相等，测试失败，并记录错误信息，包括文件名、行号以及预期值和实际值

函数使用举例：

```C
CuAssertIntEquals(tc, 1, 1);
```

#### CuAssertIntEquals_Msg (tc,ms,ex,ac)

> 在 `CuAssertIntEquals_Msg` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertIntEquals(tc, "massage", 1, 1);
```

#### CuAssertDblEquals (tc,ex,ac,dl)

> **双精度浮点数对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是期望的浮点数值。
>
> `ac` 是实际的浮点数值，即被测试函数的返回值。
>
> `dl` 是允许的误差范围（delta），因为浮点数运算可能会有精度问题，所以这个参数允许在一定误差范围内认为两个浮点数是相等的。

函数使用举例：

```C
CuAssertDblEquals(tc, 3.33, 10.0 / 3.0, 0.01);
```

#### CuAssertDblEquals_Msg (tc,ms,ex,ac,dl)

> 在 `CuAssertDblEquals()` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertDblEquals(tc, "massage", 3.33, 10.0 / 3.0, 0.01);
```

#### CuAssertPtrEquals (tc,ex,ac)

> **指针对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是期望的指针。
>
> `ac` 是实际的指针，即被测试函数的返回值或某个变量的值。
>
> 如果 `ex` 和 `ac` 两个指针相等，测试通过；如果不相等，测试失败，并记录错误信息，包括文件名、行号以及预期值和实际值。

函数使用举例：

```C
CuAssertPtrEquals(tc, NULL, NULL);
```

#### CuAssertPtrEquals_Msg (tc,ms,ex,ac)

> 在 `CuAssertPtrEquals()` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertPtrEquals_Msg(tc, "massage", NULL, NULL);
```

#### CuAssertPtrNotNull (tc,p)

> **指针对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `p` 是需要检查的指针。
>
> 如果 `p` 不是 `NULL`，测试通过；如果是 `NULL`，测试失败，并记录错误信息，包括文件名、行号以及一个默认的错误消息或自定义的错误消息。

函数使用举例：

```C
CuAssertPtrNotNull(tc, !NULL);
```

#### CuAssertPtrNotNull_Msg (tc,msg,p)

> 在 `CuAssertPtrNotNull()` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertPtrNotNull_Msg(tc, "massage", !NULL);
```

### setjmp.h 相关

> **实例**
>
> ```C
> #include <stdio.h>
> #include <setjmp.h>
>  
> static jmp_buf buf;
>  
> void second(void) {
>     printf("second\n");         // 打印
>     longjmp(buf,1);             // 跳回setjmp的调用处 - 使得setjmp返回值为1
> }
>  
> void first(void) {
>     second();
>     printf("first\n");          // 不可能执行到此行
> }
>  
> int main() {   
>     if ( ! setjmp(buf) ) {
>         first();                // 进入此行前，setjmp返回0
>     } else {                    // 当longjmp跳转回，setjmp返回1，因此进入此行
>         printf("main\n");       // 打印
>     }
>  
>     return 0;
> }
> ```

#### jmp_buf 类型

> `jmp_buf` 是一个数据类型，用于保存调用环境，包括栈指针、指令指针和寄存器等。在执行 `setjmp()` 时，这些环境信息会被保存到 `jmp_buf` 类型的变量中。

#### int setjmp (jmp_buf environment)

> 这个宏把当前环境保存在变量 `environment` 中，以便函数 `longjmp()` 后续使用。如果这个宏直接从宏调用中返回，则它会返回零，但是如果它从 `longjmp()` 函数调用中返回，则它会返回一个非零值。

#### void longjmp (jmp_buf environment, int value)

> 该函数恢复最近一次调用 `setjmp()` 宏时保存的环境，`jmp_buf` 参数的设置是由之前调用 `setjmp()` 生成的。

### Cutest 例程解析

#### AllTest.c

```C
#include <stdio.h>   // 包含标准输入输出库
#include "CuTest.h"  // 包含 CuTest 测试框架的头文件

// 函数声明：获取测试套件
CuSuite* CuGetSuite(void);
CuSuite* CuStringGetSuite(void);

// 运行所有测试的函数
int RunAllTests(void) {
    CuString* output = CuStringNew();  // 创建一个新的 CuString 用于存储测试输出
    CuSuite* suite = CuSuiteNew();     // 创建一个新的测试套件

    // 将测试套件添加到当前测试套件中
    CuSuiteAddSuite(suite, CuGetSuite());
    CuSuiteAddSuite(suite, CuStringGetSuite());

    CuSuiteRun(suite);               // 运行测试套件
    CuSuiteSummary(suite, output);   // 获取测试总结并存储在 output 中
    CuSuiteDetails(suite, output);   // 获取测试详细信息并存储在 output 中
    printf("%s\n", output->buffer);  // 打印测试输出
    return suite->failCount;         // 返回失败的测试数量
}

// 主函数，程序入口
int main(void) {
    return RunAllTests();  // 调用 RunAllTest s并返回结果
}
```

#### CuTestTest.c

#### 字符串篇

1. 创建新字符串

```C
void TestCuStringNew(CuTest* tc) {
    CuString* str = CuStringNew();             // 创建一个新的 CuString 对象
    CuAssertTrue(tc, 0 == str->length);        // 验证新字符串的长度应该为 0
    CuAssertTrue(tc, 0 != str->size);          // 验证新字符串的容量应该大于 0
    CuAssertStrEquals(tc, "", str->buffer);    // 验证新字符串的内容应该为空字符串
}
```

1. 字符串拼接

```C
void TestCuStringAppend(CuTest* tc) {
    CuString* str = CuStringNew();                      // 创建一个新的 CuString 对象
    CuStringAppend(str, "hello");                       // 将字符串 "hello" 添加到 CuString
    CuAssertIntEquals(tc, 5, (int)str->length);         // 验证字符串长度应为 5
    CuAssertStrEquals(tc, "hello", str->buffer);        // 验证字符串内容应为 "hello"
    CuStringAppend(str, " world");                      // 将字符串 " world" 添加到 CuString
    CuAssertIntEquals(tc, 11, (int)str->length);        // 验证字符串长度应为 11
    CuAssertStrEquals(tc, "hello world", str->buffer);  // 验证字符串内容应为 "hello world"
}
```

> 注意此处的字符串长度并不包含字符串结束时的 `/0`。

1. 测试框架对 NULL 的处理

```C
void TestCuStringAppendNULL(CuTest* tc) {
    CuString* str = CuStringNew();                // 创建一个新的 CuString 对象
    CuStringAppend(str, NULL);                    // 尝试将 NULL 添加到 CuString
    CuAssertIntEquals(tc, 4, (int)str->length);   // 验证字符串长度应为 4 (处理 NULL 时的特定长度)
    CuAssertStrEquals(tc, "NULL", str->buffer);   // 验证字符串内容应为 "NULL"
}
```

1. 字符拼接

```C
void TestCuStringAppendChar(CuTest* tc) {
    CuString* str = CuStringNew();                // 创建一个新的 CuString 对象
    CuStringAppendChar(str, 'a');                 // 将字符 'a' 添加到 CuString
    CuStringAppendChar(str, 'b');                 // 将字符 'b' 添加到 CuString
    CuStringAppendChar(str, 'c');                 // 将字符 'c' 添加到 CuString
    CuStringAppendChar(str, 'd');                 // 将字符 'd' 添加到 CuString
    CuAssertIntEquals(tc, 4, (int)str->length);   // 验证字符串长度应为 4
    CuAssertStrEquals(tc, "abcd", str->buffer);   // 验证字符串内容应为 "abcd"
}
```

1. 在不同的位置插入字符串

```C
void TestCuStringInserts(CuTest* tc) {
    CuString* str = CuStringNew();                      // 创建新的 CuString 对象
    CuStringAppend(str, "world");                       // 向字符串追加 "world"
    CuAssertIntEquals(tc, 5, (int)str->length);         // 验证字符串长度为 5
    CuAssertStrEquals(tc, "world", str->buffer);        // 验证字符串内容为 "world"
    CuStringInsert(str, "hell", 0);                     // 在索引 0 插入 "hell"
    CuAssertIntEquals(tc, 9, (int)str->length);         // 验证字符串长度为 9
    CuAssertStrEquals(tc, "hellworld", str->buffer);    // 验证字符串内容为 "hellworld"
    CuStringInsert(str, "o ", 4);                       // 在索引 4 插入 "o "
    CuAssertIntEquals(tc, 11, (int)str->length);        // 验证字符串长度为 11
    CuAssertStrEquals(tc, "hello world", str->buffer);  // 验证字符串内容为 "hello world"
    CuStringInsert(str, "!", 11);                       // 在索引 11 插入 "!"
    CuAssertIntEquals(tc, 12, (int)str->length);        // 验证字符串长度为 12
    CuAssertStrEquals(tc, "hello world!", str->buffer); // 验证字符串内容为 "hello world!"
}
```

1. 缓存区大小检查

```C
void TestCuStringResizes(CuTest* tc) {
    CuString* str = CuStringNew();                      // 初始化一个新的 CuString 对象
    int i;
    for (i = 0; i < STRING_MAX; ++i) {                  // 循环向字符串追加内容，直到达到预定的最大长度
        CuStringAppend(str, "aa");
    }
    CuAssertTrue(tc, STRING_MAX * 2 == str->length);    // 验证最终字符串的长度是否为 STRING_MAX * 2
    CuAssertTrue(tc, STRING_MAX * 2 <= str->size);      // 检查缓冲区大小是否足够
```

> `str->size` 为 `CuStringNew()` 创建出对象的字符串缓存区大小。
>
> 当目前开辟的缓存区大小不够但是系统的空间充足时 cutest 会调用 `CuStringResize` 额外增加缓存区。

1. 创建并返回一个测试套件，用于测试 CuString 的功能

```C
CuSuite* CuStringGetSuite(void) {
    CuSuite* suite = CuSuiteNew();  // 初始化一个新的测试套件

    SUITE_ADD_TEST(suite, TestCuStringNew);         // 添加测试函数：测试 CuString 的创建
    SUITE_ADD_TEST(suite, TestCuStringAppend);      // 添加测试函数：测试向 CuString 中追加字符串
    SUITE_ADD_TEST(suite, TestCuStringAppendNULL);  // 添加测试函数：测试向 CuString 中追加 NULL
    SUITE_ADD_TEST(suite, TestCuStringAppendChar);  // 添加测试函数：测试向 CuString 中追加单个字符
    SUITE_ADD_TEST(suite, TestCuStringInserts);     // 添加测试函数：测试在 CuString 中插入字符串
    SUITE_ADD_TEST(suite, TestCuStringResizes);     // 添加测试函数：测试 CuString 的自动扩展功能

    return suite;  // 返回创建的测试套件
}
```

#### 其他

> `CuGetSuite` 函数中均为测试框架函数中的一些运用。

```C
void TestCuStringAppendFormat(CuTest* tc) {  // 测试 CuStringAppendFormat 函数的正确性
    int i;
    char* text = CuStrAlloc(301);  // 为长字符串分配内存
    CuString* str = CuStringNew();  // 创建新的 CuString 对象
    for (i = 0; i < 300; ++i)
        text[i] = 'a';  // 填充字符串内容为 'a'
    text[300] = '\0';  // 以 NULL 结尾

    CuStringAppendFormat(str, "%s", text);  // 使用格式化字符串追加内容到 CuString

    /* buffer limit raised to HUGE_STRING_LEN so no overflow */

    CuAssert(tc, "length of str->buffer is 300", 300 == strlen(str->buffer));  // 验证结果字符串的长度是否为 300
}
```

... ...

## 代码生成

> 查找源文件：脚本会检查当前目录中的所有 `.c` 文件，或者使用命令行参数指定的文件。
>
> 生成头文件：在生成的文件开头插入自动生成的代码提示、标准库包含和 CuTest 头文件的引入。
>
> 提取测试函数：脚本会查找所有以 `void Test` 开头的函数，提取其函数名并生成相应的 `extern` 声明。这是为了使测试函数在其他文件中可见。
>
> 构建测试套件：为每个测试函数生成 `SUITE_ADD_TEST` 调用，以将这些测试添加到测试套件中。
>
> 运行所有测试：在 `RunAllTests` 函数中，创建一个输出字符串和一个测试套件，然后运行所有添加的测试，输出测试结果，并清理资源。
>
> 主函数：脚本在生成的文件中包含一个 `main` 函数，调用 `RunAllTests`，从而执行所有测试。

```Bash
# 指定多个源文件位置
./make-tests.sh file1.c file2.c file3.c
```

# 中文 README 文档 (原项目)

**如何使用**

您可以使用 CuTest 创建单元测试，以推动您的开发，采用极限编程的风格。您还可以为现有代码添加单元测试，以确保其按预期工作。

您的单元测试是一次投资。它们让您能够自信地更改代码和添加新功能，而不必担心意外破坏早期功能。

**许可**

有关许可的详细信息，请参见 license.txt。

**入门**

要将单元测试添加到您的 C 代码中，您只需 CuTest.c 和 CuTest.h 文件。

CuTestTest.c 和 AllTests.c 已包含在内，以提供如何编写单元测试的示例，以及如何将它们聚合到套件中并合并到单个 AllTests.c 文件中。套件允许您将组测试放入逻辑集合中。AllTests.c 组合所有套件并运行它们。

您不需要查看 CuTest.c。查看 CuTestTest.c 和 AllTests.c（以获取示例用法）应该就足够了。

下载源代码后，运行编译器以创建名为 AllTests.exe 的可执行文件。例如，如果您使用的是带有 cl.exe 编译器的 Windows，您可以输入：

```Plain
cl.exe AllTests.c CuTest.c CuTestTest.c
AllTests.exe
```

这将运行与 CuTest 相关的所有单元测试并在控制台上打印输出。您可以在上面的命令中将 cl.exe 替换为 gcc 或您喜欢的其他编译器。

**详细示例**

这是一个更详细的示例。我们将首先进行一个简单的测试练习。目标是创建一个字符串工具库。首先，让我们编写一个将以 null 结尾的字符串转换为全大写的函数。

确保 CuTest.c 和 CuTest.h 可以在您的 C 项目中访问。接下来，创建一个名为 StrUtil.c 的文件，内容如下：

```C
#include "CuTest.h"

char* StrToUpper(char* str) {
    return str;
}

void TestStrToUpper(CuTest *tc) {
    char* input = strdup("hello world");
    char* actual = StrToUpper(input);
    char* expected = "HELLO WORLD";
    CuAssertStrEquals(tc, expected, actual);
}

CuSuite* StrUtilGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestStrToUpper);
    return suite;
}
```

创建另一个名为 AllTests.c 的文件，内容如下：

```C
#include "CuTest.h"

CuSuite* StrUtilGetSuite();

void RunAllTests(void) {
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();
    
    CuSuiteAddSuite(suite, StrUtilGetSuite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(void) {
    RunAllTests();
}
```

然后在命令行输入：

```Plain
gcc AllTests.c CuTest.c StrUtil.c
```

以进行编译。您可以用您喜欢的编译器替换 gcc。CuTest 应该足够可移植，可以处理所有 Windows 和 Unix 编译器。然后要运行测试，输入：

```Plain
a.out
```

这将打印一个错误，因为我们尚未正确实现 StrToUpper 函数。我们只是返回字符串，而没有将其转换为大写。

```C
char* StrToUpper(char* str) {
    return str;
}
```

将其重写如下：

```C
char* StrToUpper(char* str) {
    char* p;
    for (p = str ; *p ; ++p) *p = toupper(*p);
    return str;
}
```

重新编译并再次运行测试。这次测试应该通过。

**接下来该做什么**

此时，您可能想为 StrToUpper 函数编写更多测试。以下是一些想法：

- TestStrToUpper_EmptyString：传入 ""
- TestStrToUpper_UpperCase：传入 "HELLO WORLD"
- TestStrToUpper_MixedCase：传入 "HELLO world"
- TestStrToUpper_Numbers：传入 "1234 hello"

在编写每个测试时，将其添加到 StrUtilGetSuite 函数中。如果不这样做，测试将不会运行。稍后在编写其他函数和测试时，请务必也将它们包含在 StrUtilGetSuite 中。StrUtilGetSuite 函数应包含 StrUtil.c 中的所有测试。

随着时间的推移，您将创建另一个名为 FunkyStuff.c 的文件，其中包含与 StrUtil 无关的其他函数。遵循相同的模式。在 FunkyStuff.c 中创建 FunkyStuffGetSuite 函数，并将 FunkyStuffGetSuite 添加到 AllTests.c 中。

该框架的设计方式使得组织大量测试变得容易。

**大局**

每个单独的测试对应于一个 CuTest。这些被分组形成 CuSuite。CuSuites 可以包含 CuTests 或其他 CuSuites。AllTests.c 将程序中的所有 CuSuites 收集到一个单一的 CuSuite 中，然后作为一个 CuSuite 运行。

该项目是开源的，因此可以随意查看 CuTest.c 文件的底层实现，以了解其工作原理。CuTestTest.c 包含对 CuTest.c 的测试。因此，CuTest 自己进行测试。

由于 AllTests.c 中有一个 main()，您在构建产品时需要排除它。如果您希望避免处理多个构建，这里有一个更好的方法。删除 AllTests.c 中的 main()。请注意，它只是调用 RunAllTests()。相反，我们将直接从主程序中调用此函数。

现在在实际程序的 main() 中检查是否传入了命令行选项 "--test"。如果是，则从 AllTests.c 调用 RunAllTests()。否则，运行实际程序。

将测试与代码一起发布是有用的。如果您的客户抱怨问题，可以让他们运行单元测试并将输出发送给您。这可以帮助您快速定位客户环境中出现故障的系统部分。

CuTest 提供了一系列丰富的 CuAssert 函数。以下是列表：

```C
void CuAssert(CuTest* tc, char* message, int condition);
void CuAssertTrue(CuTest* tc, int condition);
void CuAssertStrEquals(CuTest* tc, char* expected, char* actual);
void CuAssertIntEquals(CuTest* tc, int expected, int actual);
void CuAssertPtrEquals(CuTest* tc, void* expected, void* actual);
void CuAssertPtrNotNull(CuTest* tc, void* pointer);
```

该项目是开源的，因此您可以添加其他更强大的断言，以使测试更易于编写且更简洁。请随时向我发送您所做的更改，以便我可以将其纳入未来的版本中。

如果您发现本文档中的任何错误，请联系我：asimjalis@peakprogramming.com。

**自动化测试套件生成**

make-tests.sh 将在当前目录中的所有 .c 文件中进行 grep，并生成运行其中包含的所有测试的代码。使用此脚本，您无需担心编写 AllTests.c 或处理其他套件代码。

**致谢**

以下人员为 CuTest 项目贡献了有用的代码更改。感谢他们！

- [2003.02.23] Dave Glowacki <dglo@hyde.ssec.wisc.edu>
- [2009.04.17] Tobias Lippert <herrmarder@googlemail.com>
- [2009.11.13] Eli Bendersky <eliben@gmail.com>
- [2009.12.14] Andrew Brown <abrown@datasci.com>