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

> cutest 官网无法下载到源码, 遂作出更改并整理。
>
> 当前版本号由 `src/CuTest.h` 中的 `CUTEST_VERSION` 定义，目前为 `CuTest 1.7.0`。

```Bash
# 运行本仓库自测
./test.ps1
```

```Bash
# 生成覆盖率数据（GCC / Clang）
cmake -S test -B build-coverage -DCUTEST_ENABLE_COVERAGE=ON
cmake --build build-coverage
./build-coverage/cutest

# GCC 可继续使用 gcov 查看函数 / 分支 / 条件覆盖情况
gcov test/CuTestTest.c src/CuTest.c
```

```Bash
# 刷新 AllTests 聚合入口
./test.ps1 update

# 只构建默认测试工程
./test.ps1 cmake
./test.ps1 make
./test.ps1 run
```

> `CUTEST_ENABLE_COVERAGE` 目前仅对 GCC / Clang 生效；MSVC 不支持这套 `gcov` 覆盖率流程。

## Memory Middleware

CuTest 现在提供一套可选的 Memory Middleware，用固定静态 heap 模拟 `malloc`、`calloc`、`realloc`、`free`，实现思路参考 FreeRTOS `heap_4`。

默认情况下它是关闭的，现有项目仍然继续使用标准库分配函数。启用时可以在编译选项中定义：

```C
CUTEST_USE_MEMORY_MIDDLEWARE=1
```

也可以按需覆盖 heap 大小和对齐方式：

```C
CUTEST_MEMORY_HEAP_SIZE=16384
CUTEST_MEMORY_ALIGNMENT=8
```

在多线程、RTOS 或中断相关环境中，还可以按需覆盖临界区钩子：

```C
CUTEST_MEMORY_LOCK()
CUTEST_MEMORY_UNLOCK()
```

如果你在 CMake 工程中使用 CuTest，可以给目标增加这些定义：

```CMake
target_compile_definitions(my_test_target PRIVATE
    CUTEST_USE_MEMORY_MIDDLEWARE=1
    CUTEST_MEMORY_HEAP_SIZE=16384
    CUTEST_MEMORY_ALIGNMENT=8
)
```

中间件的内部声明位于 `src/memory/CUMemory.h`，供 CuTest 内部和仓库内测试使用。当前提供以下接口：

```C
void* CuMemoryMalloc(size_t size);
void* CuMemoryCalloc(size_t count, size_t size);
void* CuMemoryRealloc(void* ptr, size_t size);
void CuMemoryFree(void* ptr);
void CuMemoryReset(void);
size_t CuMemoryGetFreeSize(void);
size_t CuMemoryGetMinimumEverFreeSize(void);
```

`CuMemoryReset()` 仅适合在系统启动阶段或独立测试准备阶段使用。调用它会重新初始化整个 middleware heap，并使当前所有未释放的 middleware 分配失效。

## CuTest 库函数解析

> 此库的使用重点为 `Assert 公共断言相关` 的 API 使用，故重点介绍这些 API。

### CuArray 相关

> 在嵌入式调试中我们经常需要对 `unsigned char*` 也就是 `unit8_t` 类型的输入和返回进行测试，例如串口、I2C、、SPI、蓝牙等等。
>
> 因此我额外添加了 `CuArray` 对象以及相关的 API。

```C
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
```

#### CuArray 对象

> 由 `CuArrayNew()` 函数创建或使用 `CuArrayInit()` 手动初始化。
>
> 在使用时需要注意一个概念，例如, `arr[num]` 其中 `num` 的含义为偏移量（offset）而非索引（index）。C语言每个版本的公开资料中并无索引的概念，若将其理解成索引在程序开发时可能会出现逻辑不清晰而引起的边界 `+1` 问题。

```C
typedef struct {
    size_t length;           // 数组的长度
    size_t size;             // 开辟的数组空间大小
    unsigned char* array;    // 指向数组的指针
} CuArray;
```

#### unsigned char* CuArrAlloc (size_t size)

> 创建 `size` 大小的数组空间。

```C
unsigned char* arr = CuArrAlloc(256);    // 创建大小为 256 个字节的数组空间
```

#### unsigned char* CuArrCopy (unsigned char* old, size_t len)

> 从指定数组中复制并创建指定大小的空间。

```C
unsigned char arrTemp[6] = {1, 2, 3, 4, 5, 6};
unsigned char* arr = CuArrCopy(arrTemp, 4);    // 创建大小为 4 个字节的数组空间，其元素为 {1, 2, 3, 4}
```

#### void CuArrayInit (CuArray* arr)

> 初始化数组对象并分配 `ARRAY_MAX` 字节大小的的初始空间。

```C
CuArray arr；
CuArrayInit(&arr);    // 初始化数组对象
```

#### CuArray* CuArrayNew (void)

> 创建并初始化数组对象，在初始化时分配 `ARRAY_MAX` 字节大小的的初始空间。

```C
CuArray* arr = CuArrayNew();    // 创建数组对象
```

#### void CuArrayAppend (CuArray* arr, unsigned char* array, size_t len)

> 数组对象拼接其他数组。

```C
unsigned char testArry1[3] = {1, 2, 3};
unsigned char testArry2[3] = {4, 5, 6};

CuArray* arr = CuArrayNew();         // 创建数组对象
CuArrayAppend(arr, testArry1, 3);    // 给数组对象拼接数组 testArry1
CuArrayAppend(arr, testArry2, 3);    // 给数组对象拼接数组 testArry2
```

#### void CuArrayAppendSingle (CuArray* arr, unsigned char single)

> 数组对象拼接单字节。

```C
CuArray* arr = CuArrayNew();    // 创建数组对象
CuArrayAppendSingle(arr, 1);    // 给数组对象拼接 1
CuArrayAppendSingle(arr, 2);    // 给数组对象拼接 2
```

#### void CuArrayInsert (CuArray* arr, unsigned char* array, size_t pos, size_t len)

> 数组对象插入其他数组。

```C
unsigned char testArry1[3] = {1, 3, 6};
unsigned char testArry2[1] = {2};
unsigned char testArry3[2] = {4, 5};

CuArray* arr = CuArrayNew();            // 创建数组对象
CuArrayAppend(arr, testArry1, 3);       // 给数组对象拼接 testArry1
CuArrayInsert(arr, testArry2, 1, 1);    // 向数组对象偏移量为 1 处拼接数组 testArry2
CuArrayInsert(arr, testArry3, 3, 2);    // // 向数组对象偏移量为 3 处拼接数组 testArry3
```

#### void CuArrayResize (CuArray* arr, size_t newSize)

> 重分配数组对象的内存。

```C
CuArray* arr= CuArrayNew();
CuArrayResize(arr, ARRAY_INC * 2);    // 重新分配 CuArray 对象的内存空间
```

#### void CuArrayDelete (CuArray* arr)

> 删除数组对象并释放分配给它的空间。

```C
CuArray* str = CuArrayNew();
CuArrayDelete(str);    // 删除 CuArray对象
```

### CuString 相关

> `strlen()` 函数计算字符串长度时，它会返回字符串中字符的数量，不包括终止符。

```C
/* CuString */

char* CuStrAlloc(size_t size);
char* CuStrCopy(const char* old);

#define CU_MALLOC(SIZE)          malloc((SIZE))
#define CU_CALLOC(COUNT, SIZE)   calloc((COUNT), (SIZE))
#define CU_REALLOC(PTR, SIZE)    realloc((PTR), (SIZE))
#define CU_FREE(PTR)             free((PTR))
#define CU_ALLOC(TYPE)           ((TYPE*)CU_MALLOC(sizeof(TYPE)))

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
#define SUITE_INLINE_CAPACITY 8
#define SUITE_INC 8

#define SUITE_ADD_TEST(SUITE, TEST) \
    do { \
        CuTest* testCase = CuTestNew(#TEST, TEST); \
        if (!CuSuiteAdd((SUITE), testCase)) { \
            CuTestDelete(testCase); \
        } \
    } while (0)

typedef struct
{
    int count;
    int failCount;
    int capacity;
    CuTest** list;
    CuTest* inlineList[SUITE_INLINE_CAPACITY];
} CuSuite;

void CuSuiteInit(CuSuite* testSuite);
CuSuite* CuSuiteNew(void);
void CuSuiteCleanup(CuSuite* testSuite);
void CuSuiteDelete(CuSuite *testSuite);
int CuSuiteAdd(CuSuite* testSuite, CuTest *testCase);
void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2);
void CuSuiteRun(CuSuite* testSuite);
void CuSuiteSummary(CuSuite* testSuite, CuString* summary);
void CuSuiteDetails(CuSuite* testSuite, CuString* details);
```

#### CuSuite 对象

> 测试套件。
>
> 当前 `CuSuite` 会先使用 `inlineList` 中的小容量内联空间保存测试用例。当测试数量超过 `SUITE_INLINE_CAPACITY` 后，再按 `SUITE_INC` 扩容到堆内存。这样少量测试时不需要额外申请列表内存，大量测试时也可以继续增长。

```C
#define MAX_TEST_CASES  1024        // 测试套件中可以包含的最大测试用例数量
#define SUITE_INLINE_CAPACITY 8     // 测试套件内联保存的测试用例数量
#define SUITE_INC 8                 // 动态列表扩容步长

typedef struct
{
    int count;                       // 测试套件中当前注册的测试用例数量
    int failCount;                   // 测试套件中失败的测试用例数量
    int capacity;                    // 当前 list 可容纳的测试用例数量
    CuTest** list;                   // 当前使用的测试用例指针列表
    CuTest* inlineList[SUITE_INLINE_CAPACITY]; // 小容量内联列表
} CuSuite;
```

#### void CuSuiteInit (CuSuite* testSuite)

> 初始化 `CuSuite` 对象。

```C
CuSuite suite;
CuSuiteInit(&suite);    // 初始化 CuSuite 对象
```

#### CuSuite* CuSuiteNew (void)

> 创建并初始化 `CuSuite` 对象。

```C
CuSuite* suite = CuSuiteNew();    // 创建并初始化 CuSuite 对象
```

#### void CuSuiteCleanup (CuSuite* testSuite)

> 清理 `CuSuite` 对象内部动态列表，并将对象恢复到空套件状态。
>
> 该函数不会删除已经加入套件的 `CuTest` 测试样例，适合手动管理测试样例生命周期的栈上 `CuSuite`。

```C
CuSuite suite;
CuSuiteInit(&suite);

/* 添加并运行测试 */

CuSuiteCleanup(&suite);    // 清理套件内部动态列表
```

#### void CuSuiteDelete (CuSuite *testSuite)

> 删除堆上创建的 `CuSuite` 对象，并释放套件拥有的测试样例。

```C
CuSuite* suite = CuSuiteNew();    // 创建并初始化 CuSuite 对象
CuSuiteDelete(suite);             // 删除 CuSuite 对象
```

#### int CuSuiteAdd (CuSuite* testSuite, CuTest *testCase)

> 给测试套件添加测试样例。
>
> 返回非零表示添加成功，返回 0 表示参数无效或内部扩容失败。

```C
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc1 = CuTestNew("MyTest1", TestFunction1);    // 创建样例 1
CuTest* tc2 = CuTestNew("MyTest2", TestFunction2);    // 创建样例 2

CuSuiteAdd(suite, tc1);                               // 给测试套件添加测试样例 1
CuSuiteAdd(suite, tc2);                               // 给测试套件添加测试样例 2
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
>
> 成功拼接的测试样例会从 `testSuite2` 移动到 `testSuite` 中。

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

CuSuiteAdd(suite, tc);                                // 给测试套件添加测试样例
CuSuiteRun(suite);                                    // 运行测试套件
```

#### void CuSuiteSummary (CuSuite* testSuite, CuString* summary)

> 获取测试套间的测试总结（返回值）。

```C
CuString* summary = CuStringNew();                    // 创建初始化后的 CuString 对象
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc = CuTestNew("MyTest", TestFunction);       // 创建样例

CuSuiteAdd(suite, tc);                                // 给测试套件添加测试样例
CuSuiteRun(suite);                                    // 运行测试套件
CuSuiteSummary(suite, summary);                       // 获取测试套间的测试总结
```

#### void CuSuiteDetails (CuSuite* testSuite, CuString* details)

> 获取测试套件详细的测试结果。

```C
CuString* details = CuStringNew();                    // 创建初始化后的 CuString 对象
CuSuite* suite = CuSuiteNew();                        // 创建并初始化 CuSuite 对象
CuTest* tc = CuTestNew("MyTest", TestFunction);       // 创建样例

CuSuiteAdd(suite, tc);                                // 给测试套件添加测试样例
CuSuiteRun(suite);                                    // 运行测试套件
CuSuiteDetails(suite, details);                       // 获取测试套件详细的测试结果
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
CuFail(tc, "massage");
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

#### CuAssertMacroEquals (tc, ex, ac)

> **宏对比断言函数：**
>
> 主要用于将预期宏的值与所测试函数的返回值进行对比，不同时打印断言。
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是一个预期宏。
>
> `ac` 实际的值。

函数使用举例：

```C
#define EX_MACRO_VAL    1
CuAssertMacroEquals(tc, EX_MACRO_VAL, 1);
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
CuAssertStrEquals_Msg(tc, "massage", "MyTest", "MyTest");
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
> 如果 `ex` 和 `ac` 两个整数相等，测试通过；如果不相等，测试失败，并记录错误信息，包括文件名、行号以及预期值和实际值。

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
CuAssertIntEquals_Msg(tc, "massage", 1, 1);
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
CuAssertDblEquals_Msg(tc, "massage", 3.33, 10.0 / 3.0, 0.01);
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
int value = 1;
CuAssertPtrNotNull(tc, &value);
```

#### CuAssertPtrNotNull_Msg (tc,msg,p)

> 在 `CuAssertPtrNotNull()` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
int value = 1;
CuAssertPtrNotNull_Msg(tc, "massage", &value);
```

#### CuAssertArrEquals (tc, ex, ac, len)

> **数组对比断言函数：**
>
> `tc` 是指向 `CuTest` 结构的指针，代表当前的测试用例。
>
> `ex` 是期望的数组。
>
> `ac` 是实际的数组，即被测试函数的返回值或某个变量的值。
>
> `len` 是要对比的长度。
>
> 如果 `ex` 和 `ac` 两个数组在 `len` 长度内完全相同，测试通过；如果不相等，测试失败，并记录错误信息，包括文件名、行号以及具体位置预期值和实际值的差异。

函数使用举例：

```C
CuAssertArrEquals(tc, testArry1, testArry2, 1);
```

#### CuAssertArrEquals_Msg (tc, ms, ex, ac, len)

> 在 `CuAssertArrEquals` 函数的基础上添加错误信息打印。
>
> `ms` 是一个字符串，描述了失败时的额外信息。

函数使用举例：

```C
CuAssertArrEquals_Msg(tc, "massage", testArry1, testArry2, 1);
```

### CREATE_ASSERTS

> 宏函数创建辅助函数。

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

extern void TestCuStringNew(CuTest*);
extern void TestCuStringAppend(CuTest*);

void RunAllTests(void) {
    CuString* output = CuStringNew();  // 创建一个新的 CuString 用于存储测试输出
    CuSuite* suite = CuSuiteNew();     // 创建一个新的测试套件

    SUITE_ADD_TEST(suite, TestCuStringNew);
    SUITE_ADD_TEST(suite, TestCuStringAppend);

    CuSuiteRun(suite);               // 运行测试套件
    CuSuiteSummary(suite, output);   // 获取测试总结并存储在 output 中
    CuSuiteDetails(suite, output);   // 获取测试详细信息并存储在 output 中
    printf("%s\n", output->buffer);  // 打印测试输出
}

int main(void) {
    RunAllTests();
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

> 当前仓库不再使用 `CuGetSuite` 作为总入口；`AllTests.c` 由 Python 脚本直接扫描 `void Test...` 函数后生成，并可按配置决定是否生成 `main()`。

```C
/* 测试 CuStringAppendFormat 函数的正确性 */
void TestCuStringAppendFormat(CuTest* tc) {
    int i;
    char* text = CuStrAlloc(301);                                              // 为长字符串分配内存
    CuString* str = CuStringNew();                                             // 创建新的 CuString 对象
    for (i = 0; i < 300; ++i)
        text[i] = 'a';                                                         // 填充字符串内容为 'a'
    text[300] = '\0';                                                          // 以 NULL 结尾

    CuStringAppendFormat(str, "%s", text);                                     // 使用格式化字符串追加内容到 CuString

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

### Linux

```Bash
# 指定多个源文件位置
python src/scripts/make-tests.py --config test/make-tests.json --files file1.c file2.c file3.c --output AllTests.c
# MCU / 嵌入式场景可关闭 main() 生成
python src/scripts/make-tests.py --config test/make-tests.json --files file1.c file2.c file3.c --emit-main false --output AllTests.c
```

# 英文 README 文档 (原项目)

HOW TO USE

You can use CuTest to create unit tests to drive your development
in the style of Extreme Programming. You can also add unit tests to
existing code to ensure that it works as you suspect.

Your unit tests are an investment. They let you to change your
code and add new features confidently without worrying about
accidentally breaking earlier features.


LICENSING

For details on licensing see license.txt.


GETTING STARTED

To add unit testing to your C code the only files you need are
CuTest.c and CuTest.h. 

CuTestTest.c and AllTests.c have been included to provide an
example of how to write unit tests and then how to aggregate them
into suites and into a single AllTests.c file. Suites allow you
to put group tests into logical sets. AllTests.c combines all the
suites and runs them. 

You should not have to look inside CuTest.c. Looking in
CuTestTest.c and AllTests.c (for example usage) should be
sufficient. 

After downloading the sources, run your compiler to create an
executable called AllTests.exe. For example, if you are using
Windows with the cl.exe compiler you would type: 

    cl.exe AllTests.c CuTest.c CuTestTest.c
    AllTests.exe

This will run all the unit tests associated with CuTest and print
the output on the console. You can replace cl.exe with gcc or
your favorite compiler in the command above.


DETAILED EXAMPLE

Here is a more detailed example. We will work through a simple
test first exercise. The goal is to create a library of string
utilities. First, lets write a function that converts a
null-terminated string to all upper case.

Ensure that CuTest.c and CuTest.h are accessible from your C
project. Next, create a file called StrUtil.c with these
contents:

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

Create another file called AllTests.c with these contents:

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

Then type this on the command line:

    gcc AllTests.c CuTest.c StrUtil.c

to compile. You can replace gcc with your favorite compiler.
CuTest should be portable enough to handle all Windows and Unix
compilers. Then to run the tests type:

    a.out

This will print an error because we haven't implemented the
StrToUpper function correctly. We are just returning the string
without changing it to upper case. 

    char* StrToUpper(char* str) {
        return str;
    }

Rewrite this as follows:

    char* StrToUpper(char* str) {
        char* p;
        for (p = str ; *p ; ++p) *p = toupper(*p);
        return str;
    }

Recompile and run the tests again. The test should pass this
time.


WHAT TO DO NEXT

At this point you might want to write more tests for the
StrToUpper function. Here are some ideas:

TestStrToUpper_EmptyString :  pass in ""
TestStrToUpper_UpperCase   :  pass in "HELLO WORLD"
TestStrToUpper_MixedCase   :  pass in "HELLO world"
TestStrToUpper_Numbers     :  pass in "1234 hello"

As you write each one of these tests add it to StrUtilGetSuite
function. If you don't the tests won't be run. Later as you write
other functions and write tests for them be sure to include those
in StrUtilGetSuite also. The StrUtilGetSuite function should
include all the tests in StrUtil.c

Over time you will create another file called FunkyStuff.c
containing other functions unrelated to StrUtil. Follow the same
pattern. Create a FunkyStuffGetSuite function in FunkyStuff.c.
And add FunkyStuffGetSuite to AllTests.c.

The framework is designed in the way it is so that it is easy to
organize a lot of tests.

THE BIG PICTURE

Each individual test corresponds to a CuTest. These are grouped
to form a CuSuite. CuSuites can hold CuTests or other CuSuites.
AllTests.c collects all the CuSuites in the program into a single
CuSuite which it then runs as a single CuSuite.

The project is open source so feel free to take a peek under the
hood at the CuTest.c file to see how it works. CuTestTest.c
contains tests for CuTest.c. So CuTest tests itself.

Since AllTests.c has a main() you will need to exclude this when
you are building your product. Here is a nicer way to do this if
you want to avoid messing with multiple builds. Remove the main()
in AllTests.c. Note that it just calls RunAllTests(). Instead
we'll call this directly from the main program.

Now in the main() of the actual program check to see if the
command line option "--test" was passed. If it was then I call
RunAllTests() from AllTests.c. Otherwise run the real program.

Shipping the tests with the code can be useful. If you customers
complain about a problem you can ask them to run the unit tests
and send you the output. This can help you to quickly isolate the
piece of your system that is malfunctioning in the customer's
environment. 

CuTest offers a rich set of CuAssert functions. Here is a list:

void CuAssert(CuTest* tc, char* message, int condition);
void CuAssertTrue(CuTest* tc, int condition);
void CuAssertStrEquals(CuTest* tc, char* expected, char* actual);
void CuAssertIntEquals(CuTest* tc, int expected, int actual);
void CuAssertPtrEquals(CuTest* tc, void* expected, void* actual);
void CuAssertPtrNotNull(CuTest* tc, void* pointer);

The project is open source and so you can add other more powerful
asserts to make your tests easier to write and more concise.
Please feel free to send me changes you make so that I can
incorporate them into future releases.

If you see any errors in this document please contact me at
asimjalis@peakprogramming.com.


AUTOMATING TEST SUITE GENERATION

make-tests.sh will grep through all the .c files in the current
directory and generate the code to run all the tests contained in
them. Using this script you don't have to worry about writing
AllTests.c or dealing with any of the other suite code.


CREDITS

These people have contributed useful code changes to the CuTest project.
Thanks!

- [02.23.2003] Dave Glowacki <dglo@hyde.ssec.wisc.edu>
- [04.17.2009] Tobias Lippert <herrmarder@googlemail.com>
- [11.13.2009] Eli Bendersky <eliben@gmail.com>
- [12.14.2009] Andrew Brown <abrown@datasci.com>