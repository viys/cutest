# CuTest 测试移植手册

本文面向将本仓库 CuTest 引入其他 C 项目的开发者，覆盖从项目评估、文件引入、测试路径选择、自动注册、构建接线到覆盖率报告的完整流程。

本文中的目录名是推荐约定，不是强制模板。目标项目已有测试布局或构建入口时，应复用现有结构，只保留职责边界。所有示例均为通用写法，不包含任何具体产品、芯片、SDK、私有路径或业务宏。

如果只想先跑通最小示例，请先阅读 [CuTest 快速移植指南](porting-guide.md)。

## 1. 移植完成的标准

一次完整移植至少应满足：

- CuTest 核心与目标项目业务适配相互独立。
- 开发者能通过一条稳定命令构建并运行测试。
- 测试失败能通过非零退出码或明确的板级状态被自动识别。
- 新增、删除测试后，测试注册入口不会遗漏或残留。
- 主机测试不访问真实硬件，板级测试不伪造关键硬件行为。
- 覆盖率只统计目标项目选定的生产源码。

## 2. 移植前先盘点目标项目

不要先复制文件。先确认目标项目当前状态：

1. 搜索是否已有 `CuTest.h`、`CuTest.c`、`SUITE_ADD_TEST`、`CuSuite`、`RunAllTests` 或 `*GetSuite`。
2. 找到生产源码、现有测试、测试入口和构建文件。
3. 确认目标项目使用 CMake、Make、IDE 工程还是自定义构建脚本。
4. 确认主机编译器和目标工具链是否都需要支持。
5. 列出被测模块对中断、寄存器、定时器、存储、网络和动态内存的依赖。
6. 确认 CI 或本地脚本如何判断测试成功与失败。

如果目标项目已经集成了 CuTest，应优先补齐缺失接线，不要建立第二套平行框架。

## 3. 选择 unit test、board test 或两者并存

### 主机侧 unit test

优先用于：

- 参数校验和边界处理
- 纯算法、编解码和格式化
- 状态机和连续操作
- 缓存、配置和数据转换
- 能通过少量平台替身隔离的模块

主机测试反馈快，适合每次提交执行，但不能证明真实中断时序、寄存器副作用或外设链路正确。

### 目标硬件 board test

优先用于：

- 中断与并发访问
- 精确时序、定时推进和超时
- 寄存器、DMA、总线或 GPIO 行为
- 启动、复位、睡眠和唤醒路径
- 真实外设、功耗或工具链运行时行为

板级测试应使用目标项目原有固件构建和启动链路，不应为了测试重写另一套产品初始化流程。

### 决策原则

先尝试让逻辑模块直接参与主机编译。只有少量平台符号缺失时，增加最小测试替身；如果必须模拟完整平台语义才能运行，就停止扩大 stub，将该行为放回 board test。两条路径并存时，应维护独立测试文件、生成配置和构建目标。

## 4. 引入本仓库文件

### 推荐方式

建议在目标项目根目录的 `cutest/` 下保留本仓库的 `src/` 层级：

```text
your-project/
├── cutest/
│   └── src/
│       ├── CuTest.c
│       ├── CuTest.h
│       ├── CMakeLists.txt
│       ├── memory/
│       │   ├── CuMemory.c
│       │   └── CUMemory.h
│       └── scripts/
│           ├── make-tests.py
│           └── generate_coverage_report.py
├── tests/
│   ├── config/
│   ├── unit/
│   └── board/
├── cmake/
└── build script
```

固定版本复制和 Git submodule 都可以。目标项目应记录所用版本，升级时整体更新，不要把产品适配反向写入 CuTest 核心。

### 文件选择

始终需要：

- `src/CuTest.c`
- `src/CuTest.h`

需要自动生成测试注册入口时增加：

- `src/scripts/make-tests.py`

需要受控静态 heap 时增加：

- `src/memory/CuMemory.c`
- `src/memory/CUMemory.h`

需要主机覆盖率报告时增加：

- `src/scripts/generate_coverage_report.py`

不要移植本仓库的 `test/`、`test.ps1`、`docs/` 或构建产物。目标项目应建立自己的测试源、生成配置和顶层命令。

## 5. 推荐的目标项目布局

```text
tests/
├── config/
│   ├── unit-tests.json
│   └── board-tests.json
├── unit/
│   ├── unit_port.c
│   └── unit_test_*.c
└── board/
    ├── board_test_config.h
    ├── board_runner.c
    ├── board_port.c
    ├── board_runtime.c
    └── board_test_*.c
```

职责必须清晰：

- `unit_test_*.c` 和 `board_test_*.c` 只存放真实测试。
- `unit_port.c` 只存放主机替身。
- `board_runner.c` 负责执行 suite 和发布结果。
- `board_port.c` 负责最小输出通道适配。
- `board_runtime.c` 只补目标工具链缺失的最小运行时能力。
- registry 生成到构建目录，不提交到源码目录，也不手工修改。

## 6. 写测试用例

生成器默认适合扫描以下签名：

```c
#include "CuTest.h"

/* 验证被测接口的一个独立行为。 */
void TestExample(CuTest *tc)
{
    CuAssertIntEquals(tc, 1, 1);
}
```

建议每个测试只验证一个清晰行为，并优先覆盖正常路径、边界、错误路径和状态转换。测试必须可重复执行，不依赖执行顺序；共享替身状态需要在每个测试开始前显式重置。

## 7. 自动生成 registry

### 为什么分开配置

`unit test` 和 `board test` 必须使用独立 JSON 配置：

- 主机入口通常需要 `main()`，并用退出码报告失败。
- 板级入口通常只生成 suite，由固件 runner 调用。
- 两类测试的 include、扫描范围和运行环境不同。

配置路径可以自定，但建议保存在 `tests/config/`。配置中的 `files` 和 `globs` 相对配置文件所在目录解析；构建系统已持有准确源码列表时，优先通过 `--files` 显式传入。

### unit test 配置要点

以本仓库 `test/make-tests.json` 的字段结构为基础，在目标项目创建 `unit-tests.json`。下面的模板假设配置位于 `tests/config/`，并保证测试失败能传递给 CI：

```json
{
  "globs": [
    "../unit/unit_test_*.c"
  ],
  "include_lines": [
    "/* This file is generated. Do not edit it manually. */",
    "#include <stdio.h>",
    "#include \"CuTest.h\""
  ],
  "test_regex": "^void\\s+(?P<test_name>Test[A-Za-z0-9_]*)\\s*\\(\\s*CuTest\\s*\\*\\s*tc\\s*\\)",
  "extern_template": "extern void {test_name}(CuTest *tc);",
  "run_function_name": "UnitTestGetSuite",
  "suite_add_template": "    SUITE_ADD_TEST(suite, {test_name});",
  "run_function_prefix_lines": [
    "/* Create the host-side test suite. */",
    "CuSuite *{run_function_name}(void)",
    "{",
    "    CuSuite *suite = CuSuiteNew();",
    "",
    "    if (suite == NULL) {",
    "        return NULL;",
    "    }",
    ""
  ],
  "run_function_suffix_lines": [
    "",
    "    return suite;",
    "}"
  ],
  "emit_main": true,
  "main_lines": [
    "/* Run host-side tests and expose failures through the process status. */",
    "int main(void)",
    "{",
    "    CuString *output = CuStringNew();",
    "    CuSuite *suite = {run_function_name}();",
    "    int result = 0;",
    "",
    "    if ((output == NULL) || (suite == NULL)) {",
    "        CuStringDelete(output);",
    "        CuSuiteDelete(suite);",
    "        return 1;",
    "    }",
    "",
    "    CuSuiteRun(suite);",
    "    CuSuiteSummary(suite, output);",
    "    CuSuiteDetails(suite, output);",
    "    printf(\"%s\", output->buffer);",
    "    result = (suite->failCount == 0) ? 0 : 1;",
    "    CuSuiteDelete(suite);",
    "    CuStringDelete(output);",
    "    return result;",
    "}"
  ]
}
```

模板至少保证：

- `globs` 只匹配 `unit_test_*.c`。
- `test_regex` 只匹配 `void TestXxx(CuTest *tc)`。
- `emit_main` 为 `true`。
- `main()` 在 `suite->failCount != 0` 时返回非零值。
- 分配失败时也返回非零值。
- 退出前释放 `CuSuite` 和 `CuString`。

不要直接照搬本仓库的自测扫描路径；路径必须指向目标项目自己的测试源。

### board test 配置要点

下面的 `board-tests.json` 模板只生成 suite，不生成进程入口：

```json
{
  "globs": [
    "../board/board_test_*.c"
  ],
  "include_lines": [
    "/* This file is generated. Do not edit it manually. */",
    "#include \"board_test_config.h\""
  ],
  "test_regex": "^void\\s+(?P<test_name>Test[A-Za-z0-9_]*)\\s*\\(\\s*CuTest\\s*\\*\\s*tc\\s*\\)",
  "extern_template": "extern void {test_name}(CuTest *tc);",
  "run_function_name": "BoardTestGetSuite",
  "suite_add_template": "    SUITE_ADD_TEST(suite, {test_name});",
  "run_function_prefix_lines": [
    "/* Create the target-board test suite. */",
    "CuSuite *{run_function_name}(void)",
    "{",
    "    CuSuite *suite = CuSuiteNew();",
    "",
    "    if (suite == NULL) {",
    "        return NULL;",
    "    }",
    ""
  ],
  "run_function_suffix_lines": [
    "",
    "    return suite;",
    "}"
  ],
  "emit_main": false,
  "main_lines": []
}
```

该模板应满足：

- `globs` 只匹配 `board_test_*.c`。
- `include_lines` 引入板级公共测试头。
- `run_function_name` 使用项目约定的 suite 入口名。
- `emit_main` 为 `false`。
- 生成函数返回 `CuSuite *`，创建失败时返回 `NULL`。

runner、port、runtime、stub 和生成结果都不能落入扫描范围。

### 直接调用生成器

```powershell
python cutest/src/scripts/make-tests.py `
    --config tests/config/unit-tests.json `
    --files tests/unit/unit_test_example.c `
    --output build/unit-tests/unit_tests_generated.c
```

生成器会按文件路径排序，拒绝重复测试名，并在没有匹配测试时失败。`--emit-main true|false` 可以覆盖配置值，但稳定工程应把该选择写入各自配置。

## 8. CMake 接入

### 引入 CuTest 库目标

保留 `src/CMakeLists.txt` 时，可以直接复用本仓库定义的 `CuTest` 目标：

```cmake
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/cutest/src
    ${CMAKE_BINARY_DIR}/cutest
)
```

如果目标项目不希望引入子目录，也可以把 `CuTest.c` 直接加入测试目标。

### 主机测试目标

测试源码列表应成为唯一事实来源：同一列表既传给生成器，也加入测试目标。

```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter)

file(GLOB UNIT_TEST_CASE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/tests/unit/unit_test_*.c"
)
if(NOT UNIT_TEST_CASE_SOURCES)
    message(FATAL_ERROR "No unit test sources found")
endif()

set(CUTEST_GENERATOR
    "${CMAKE_SOURCE_DIR}/cutest/src/scripts/make-tests.py"
)
set(UNIT_TEST_CONFIG
    "${CMAKE_SOURCE_DIR}/tests/config/unit-tests.json"
)
set(UNIT_TEST_REGISTRY
    "${CMAKE_CURRENT_BINARY_DIR}/unit_tests_generated.c"
)

add_custom_command(
    OUTPUT "${UNIT_TEST_REGISTRY}"
    COMMAND "${Python3_EXECUTABLE}" "${CUTEST_GENERATOR}"
            --config "${UNIT_TEST_CONFIG}"
            --files ${UNIT_TEST_CASE_SOURCES}
            --output "${UNIT_TEST_REGISTRY}"
    DEPENDS "${CUTEST_GENERATOR}" "${UNIT_TEST_CONFIG}"
            ${UNIT_TEST_CASE_SOURCES}
    COMMENT "Generating CuTest unit-test registry"
    VERBATIM
)

add_executable(app_unit_tests
    ${UNIT_TEST_CASE_SOURCES}
    ${UNIT_TEST_REGISTRY}
    tests/unit/unit_port.c
    src/module_under_test.c
)
target_include_directories(app_unit_tests PRIVATE src tests/unit)
target_link_libraries(app_unit_tests PRIVATE CuTest)

enable_testing()
add_test(NAME app_unit_tests COMMAND app_unit_tests)
```

这样修改生成器、配置或任一测试源都会重新生成 registry；新增或删除匹配文件会触发 CMake 重新配置。

### 板级测试目标

板级目标采用相同的生成依赖模式，但有三点不同：

1. 使用 `board-tests.json` 和 `board_test_*.c`。
2. 将生成 registry、`board_runner.c`、`board_port.c` 和必要的 `board_runtime.c` 加入测试固件目标。
3. 通过项目已有构建开关把板级测试源与正式固件源隔离。

不要在通用文档或 CuTest 核心中硬编码芯片库、链接脚本、SDK include、固件目标名或产品宏。这些都应留在目标项目自己的板级 CMake 文件中。

## 9. unit test 的平台替身

先尝试直接编译生产头文件和生产源码。只有实际缺失的平台符号才放入 `unit_port.c` 或 `unit_stub.c`：

- 只实现被测源码直接需要的符号。
- 用可重置状态控制成功、失败、忙碌和超时分支。
- 不访问真实寄存器、文件、网络或外设。
- 定时器和异步回调由测试显式推进，不伪造真实时间流逝。
- ISR 相关替身只验证调用协议和状态，不宣称覆盖真实中断安全。
- 每个替身函数说明其简化行为和不覆盖的语义。

如果不同模块需要冲突的符号或编译宏，应拆成多个测试可执行目标，不要把大量条件编译堆进同一个替身文件。

## 10. board test 的 runner 与 port

board runner 至少负责：

1. 初始化输出端口。
2. 按需重置 CuTest 专用内存区。
3. 创建输出对象和 suite，并处理分配失败。
4. 执行 suite，打印 summary 和 details。
5. 释放对象，发布可被自动化识别的最终状态。

板测结束后是复位、保持循环还是交还主循环，必须由目标项目明确决定。保持循环时要遵守看门狗策略；使用中断、DMA 或环形缓冲输出时，要确认失败路径不会破坏并发访问或阻塞日志。

`board_port` 只负责输出通道的最小初始化，并尽量支持重复调用。不要让它顺带初始化业务状态，否则测试结果会依赖隐藏前置条件。

## 11. 内存策略

默认使用标准 `malloc`、`calloc`、`realloc` 和 `free`。目标环境运行库稳定且允许测试使用动态内存时，不需要中间件。

需要隔离测试分配、限制 heap 或目标环境缺少稳定分配器时，启用：

如果通过本仓库 `src/CMakeLists.txt` 引入 CuTest，应在 `add_subdirectory()` 之前设置变量：

```cmake
set(CUTEST_USE_MEMORY_MIDDLEWARE ON)
set(CUTEST_MEMORY_HEAP_SIZE 16384)
set(CUTEST_MEMORY_ALIGNMENT 8)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/cutest/src
    ${CMAKE_BINARY_DIR}/cutest
)
```

该接法会同时编译 `CuMemory.c` 并向 CuTest 使用者传播必要定义。如果直接把源码加入测试目标，则必须自行加入 `CuMemory.c`，并在同一目标上定义 `CUTEST_USE_MEMORY_MIDDLEWARE=1`、heap 大小和对齐方式。

多线程、RTOS 或中断环境需要按平台实现 `CUTEST_MEMORY_LOCK()` 与 `CUTEST_MEMORY_UNLOCK()`。不要在仍有对象存活时调用 `CuMemoryReset()`；它会使中间件 heap 中现有分配全部失效。除非目标项目已经有明确约束，否则不要全局重定义业务分配器。

## 12. 覆盖率报告

### 脚本来源与位置

覆盖率报告脚本必须以本仓库为准：

```text
src/scripts/generate_coverage_report.py
```

移植时保持它与 `make-tests.py` 同处 CuTest 的 `src/scripts/` 下。不要采用参考业务工程中的其他脚本语言、其他工具目录、固定模块清单或固定报告路径。

### 本仓库的标准用法

本仓库通过以下命令生成报告：

```powershell
./test.ps1 coverage
```

它等价于按本仓库目录传入以下参数关系：

```powershell
python src/scripts/generate_coverage_report.py `
    --source-root . `
    --cmake-source-dir test `
    --build-dir build/test/coverage `
    --report-dir build/test/coverage-report `
    --filter ".*src.*\.c$" `
    --cmake-arg=-DCUTEST_ENABLE_COVERAGE=ON `
    --verbose
```

脚本会依次执行全新 CMake 配置、清理、编译、CTest 和 `gcovr --html-details`。本仓库报告入口为：

```text
build/test/coverage-report/coverage.html
```

### 在目标项目中调用

目标项目应在自己的顶层构建脚本中调用同一个 Python 脚本，只修改参数，不修改通用脚本：

```powershell
python cutest/src/scripts/generate_coverage_report.py `
    --source-root . `
    --cmake-source-dir . `
    --build-dir build/coverage `
    --report-dir build/coverage-report `
    --filter ".*src[/\\].*\.c$" `
    --cmake-arg=-DBUILD_UNIT_TESTS=ON `
    --verbose
```

参数含义：

- `--source-root`：目标项目根目录。
- `--cmake-source-dir`：能够配置主机测试的 CMake 源目录。
- `--build-dir`：专用覆盖率构建目录。
- `--report-dir`：HTML 报告输出目录。
- `--filter`：目标生产源码过滤规则，可重复传入。
- `--cmake-arg`：目标项目启用测试所需的 CMake 参数，可重复传入。

目标测试必须通过 `add_test()` 注册到 CTest。过滤规则只应覆盖待测生产源码，并排除 CuTest、本身测试源、生成 registry、port 和 stub。运行环境需要 CMake、CTest、Python、GCC 或兼容 gcov 的 Clang，以及 Python 包 `gcovr`。

## 13. 推荐移植顺序

1. 盘点目标项目已有测试和构建入口。
2. 选择 unit test、board test 或先接其中一条。
3. 固定 CuTest 版本并引入最小文件集。
4. 编写一个最小测试，先用手写入口跑通。
5. 建立对应 JSON 配置并接入 registry 生成。
6. 将生成步骤加入构建依赖。
7. 按需增加最小 port、stub 或 runtime。
8. 决定是否启用内存中间件。
9. 用目标项目的稳定命令完成构建和执行。
10. 主机测试稳定后，再按需接入本仓库覆盖率脚本。

## 14. 最终验收清单

- [ ] CuTest 核心文件未包含目标项目私有适配。
- [ ] unit 与 board 测试的扫描规则互不混用。
- [ ] registry 位于构建目录并由构建系统自动更新。
- [ ] 测试函数新增、删除、重命名后注册结果正确。
- [ ] 重复测试名会导致生成失败。
- [ ] 主机断言失败时进程和 CTest 都返回失败。
- [ ] board runner 的完成与失败状态可被稳定识别。
- [ ] stub 状态可以在测试间复位，没有顺序依赖。
- [ ] 中断、时序、并发和超时行为没有被主机替身过度承诺。
- [ ] 标准内存与 middleware 选择符合目标环境。
- [ ] 覆盖率调用的是 `src/scripts/generate_coverage_report.py` 的目标项目副本。
- [ ] 覆盖率过滤范围只包含目标生产源码。
- [ ] 文档和构建文件不含个人绝对路径、私有项目名、芯片名或产品宏。

## 15. 常见失败

### 新测试没有执行

检查测试函数签名、文件命名、`--files` 列表和 custom command 的 `DEPENDS`。确认运行的是最新生成的 registry，而不是源码目录中的旧文件。

### 本地显示断言失败，但 CI 仍通过

检查生成的主机 `main()` 是否根据 `suite->failCount` 返回非零值，并确认测试已通过 `add_test()` 注册。

### 主机链接出现大量平台符号缺失

先判断被测模块是否适合主机测试。只补直接依赖的最小替身；如果缺失符号不断扩散，说明应缩小主机测试范围或改用 board test。

### 新增测试文件后 registry 没有更新

让测试 glob 使用 `CONFIGURE_DEPENDS`，或显式维护源码清单；同时把生成器、JSON 配置和测试源全部加入生成命令依赖。

### 覆盖率包含测试框架或 stub

收紧 `--filter`，必要时传入多个过滤规则。不要用宽泛目录规则把 `cutest/`、`tests/` 或构建目录纳入生产覆盖率。
