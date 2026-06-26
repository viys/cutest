# CuTest 移植指南

本文档聚焦 `#19 / #21 / #23` 的移植与接入场景，帮助使用者在不阅读整个仓库的前提下判断最小接入范围。

## 最小移植文件集

普通接入只需要以下两个文件：

- `src/CuTest.c`
- `src/CuTest.h`

如果需要启用 Memory Middleware，再额外带上：

- `src/memory/CuMemory.c`
- `src/memory/CUMemory.h`

对应开关如下：

```c
CUTEST_USE_MEMORY_MIDDLEWARE=1
```

按需还可以覆盖：

```c
CUTEST_MEMORY_HEAP_SIZE=16384
CUTEST_MEMORY_ALIGNMENT=8
```

## 仓库目录职责边界

### 业务项目应直接引入的内容

- `src/CuTest.c`
- `src/CuTest.h`
- `src/memory/` 下的 middleware 文件（仅在启用 middleware 时）

这些文件构成 CuTest 的核心库实现，也是外部项目最小可移植的源码集合。

### 仅用于本仓库自测的内容

- `test/CuTestTest.c`
- `test/AllTests.c`
- `test/CMakeLists.txt`

其中：

- `test/CuTestTest.c` 是本仓库对 CuTest 自身行为的回归测试。
- `test/AllTests.c` 是聚合入口文件，由脚本生成，不建议手工维护。
- `test/CMakeLists.txt` 只服务于本仓库的自测工程，不是业务项目的必选模板。

### 仅用于辅助生成的内容

- `scripts/make-tests.py`
- `scripts/make-tests.json`
- `test.ps1`

其中：

- `scripts/make-tests.py` 负责扫描 `TestXxx` 函数并生成 `test/AllTests.c`。
- `scripts/make-tests.json` 是默认生成配置，定义包含内容、扫描规则和输出模板。
- `test.ps1 update` 会在仓库根目录统一调用生成脚本，给自测工程刷新聚合入口。

## 测试入口与聚合入口关系

本仓库的测试流转关系如下：

1. `test/CuTestTest.c` 定义 `void Test...` 测试函数。
2. `scripts/make-tests.py` 扫描这些函数并生成 `test/AllTests.c`。
3. `test/AllTests.c` 提供 `RunAllTests()` 和默认 `main()`。
4. `test/CMakeLists.txt` 将 `CuTestTest.c + AllTests.c + src/CuTest.c` 组装成测试可执行文件。
5. `test.ps1` 负责统一执行 `update -> cmake -> make -> run`。

这条链路是仓库内自测流程，不是业务项目必须照搬的结构。外部项目完全可以自己手写一个更小的 `AllTests.c`，只要最终调用 `CuSuiteRun()`、`CuSuiteSummary()`、`CuSuiteDetails()` 即可。

## 业务项目接入建议

如果你的项目已经有自己的构建系统，建议优先采用以下方式：

1. 把 `src/` 中所需文件复制到项目的第三方目录。
2. 在项目的测试目标里编译 `CuTest.c`。
3. 编写自己的测试源文件和聚合入口。
4. 如果是 CMake 项目，直接把 `src/CuTest.c` 加入测试目标，并确保头文件搜索路径包含 `src/`。

最小手写聚合入口示例：

```c
#include <stdio.h>
#include "CuTest.h"

extern void TestSomething(CuTest*);

void RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestSomething);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    CuStringDelete(output);
    CuSuiteDelete(suite);
}
```

## 本仓库命令

刷新并验证本仓库自测工程：

```powershell
./test.ps1
```

只更新聚合入口：

```powershell
./test.ps1 update
```

直接调用生成脚本：

```powershell
python scripts/make-tests.py --config scripts/make-tests.json --output test/AllTests.c
```

## CMake 接入说明

如果你的项目使用 CMake，建议保持接入方式尽量直接：

1. 把 `src/CuTest.c` 编进你的测试目标。
2. 把 `src/` 加入该目标的头文件搜索路径。
3. 自己维护一个测试聚合入口，或在项目内复用 `scripts/make-tests.py` 的扫描思路。
4. 如果启用 middleware，再把 `src/memory/CuMemory.c` 一并加入目标，并打开 `CUTEST_USE_MEMORY_MIDDLEWARE=1`。

一个最小接入关系通常就是：

```cmake
add_executable(my_tests
    tests/AllTests.c
    tests/MyModuleTest.c
    third_party/cutest/src/CuTest.c
)

target_include_directories(my_tests PRIVATE
    third_party/cutest/src
)
```

这里的重点是接入关系本身，而不是额外维护一个仓库内示例工程。
