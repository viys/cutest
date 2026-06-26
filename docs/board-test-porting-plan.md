# board-test 移植规划

本文档记录将当前 CuTest 版本移植到目标固件项目的规划。目标是让目标项目后续只支持板级 `board-test`，不再维护 Windows/PC 侧 unit test 流程，并尽量避免为了目标项目修改本仓库的 CuTest 核心代码。

## 目标

- 目标项目只保留 `board-test` 测试路径。
- CuTest 作为独立源码组件放入目标项目的 `cutest/` 目录。
- 目标项目启用 `CuMemory`，不再通过重定义全局 `malloc`、`calloc`、`realloc`、`free` 来适配 CuTest。
- 测试注册入口由脚本生成，减少手工维护 registry 的遗漏风险。
- 适配文件和真实测试文件命名分离，避免脚本误收集。
- 本仓库保持通用 CuTest 实现，不为单一目标项目引入专用分支。

## 移植文件范围

目标项目可以直接复制本仓库的 `src/`，再将目录重命名为 `cutest/`。这样核心源码、Memory Middleware 和通用生成器会作为一个完整单元一起移植。

### 必须移植

- `src/CuTest.c`
- `src/CuTest.h`
- `src/memory/CuMemory.c`
- `src/memory/CUMemory.h`
- `src/scripts/make-tests.py`

目标项目需要自行维护一份 board-test 专用配置，例如 `cutest/scripts/board-tests.json`；本仓库的 `test/make-tests.json` 只用于框架自测，不随 `src/` 移植。

### 不需要移植

- `test/`
- `docs/`
- 本仓库顶层 `CMakeLists.txt`
- `test.ps1`
- 覆盖率构建目录或本仓库自测产物

## 目标项目建议结构

```text
target-project/
├── cutest/
│   ├── CuTest.c
│   ├── CuTest.h
│   ├── memory/
│   │   ├── CuMemory.c
│   │   └── CUMemory.h
│   └── scripts/
│       ├── make-tests.py
│       └── board-tests.json
├── tests/
│   └── board/
│       ├── board_port.c
│       ├── board_registry.c
│       ├── board_runner.c
│       ├── board_runtime.c
│       ├── board_test_config.h
│       └── board_test_*.c
├── cmake/
│   └── board_tests.cmake
└── build.ps1
```

其中：

- `cutest/` 只保存 CuTest 本体和生成脚本。
- `tests/board/board_test_*.c` 只用于真实测试用例文件。
- `board_port.c`、`board_runner.c`、`board_runtime.c` 等板级适配文件不要使用 `board_test_` 前缀，避免和真实测试文件混淆。
- `board_registry.c` 建议由脚本生成，不建议手工维护。
- `board_test_config.h` 用于收敛 SDK 头文件、`CuTest.h` 引入顺序和目标板专用测试配置。

## 文件命名约束

板级适配层的 `.c` 文件不应与单元测试文件使用相同前缀。`board_test_` 前缀只保留给会被生成器扫描和注册的真实测试文件：

- 真实测试文件：`board_test_temperature_humidity.c`、`board_test_storage.c`
- 板级适配文件：`board_port.c`、`board_runner.c`、`board_runtime.c`
- 生成文件：`board_registry.c`
- 禁止命名：`board_test_port.c`、`board_test_runner.c`、`board_test_runtime.c`

这样 `tests/board/board_test_*.c` 可以作为稳定且明确的扫描规则，不会把端口层、运行入口或兼容层误识别为测试源。`board_test_config.h` 是不参与 `.c` 文件扫描的公共配置头文件，可以保留现有名称。

## CuMemory 接入方式

目标芯片需要使用 `CuMemory`。目标项目应通过编译宏让 CuTest 内部分配全部走 middleware：

```c
CUTEST_USE_MEMORY_MIDDLEWARE=1
```

构建时同时编译：

- `cutest/CuTest.c`
- `cutest/memory/CuMemory.c`

建议在目标项目 `board_tests.cmake` 中设置以下宏：

```cmake
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CUTEST_USE_MEMORY_MIDDLEWARE=1
    CUTEST_MEMORY_HEAP_SIZE=8192
    CUTEST_MEMORY_ALIGNMENT=4
    MAX_TEST_CASES=64
    SUITE_INLINE_CAPACITY=8
    SUITE_INC=8
    STRING_MAX=128
    STRING_INC=128
    ARRAY_MAX=128
    ARRAY_INC=128
    HUGE_STRING_LEN=512
)
```

这些数值是初始建议，最终应根据目标固件 RAM 余量、测试数量和失败输出长度调整。

## 不再重定义全局内存函数

旧接入中如果存在类似做法：

```c
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
```

迁移后应删除这类全局替换。原因是它会影响整个固件链接范围，容易让业务代码、SDK 或库函数误用测试分配器。

新版结构中，CuTest 自己通过 `CU_MALLOC`、`CU_CALLOC`、`CU_REALLOC`、`CU_FREE` 路由到 `CuMemory`。目标项目只需要配置 `CUTEST_USE_MEMORY_MIDDLEWARE=1`，不需要接管 C 标准库分配函数。

## board-test 入口

推荐在目标固件公共入口中提供弱函数钩子：

```c
CU_WEAK void boardTestMain(bool isRetention)
{
    (void)isRetention;
}
```

普通固件可以无条件调用 `boardTestMain(isRetention)`。非板测构建会链接弱空实现，板测构建则由 `board_runner.c` 提供强实现覆盖它。因此不需要再引入 `BOARD_TEST_FIRMWARE_ENABLE` 这类开关宏。

## board-test 端口层

建议把目标项目板级适配收敛到 `tests/board/board_port.c`：

```c
void boardTestPortInit(void);
void boardTestPortWrite(const char *text);
```

`boardTestPortInit()` 负责串口或日志通道初始化。`boardTestPortWrite()` 可选，如果目标项目直接使用 `printf` 输出，也可以先不引入该函数。

端口层不应包含 CuTest 内存分配逻辑。内存统一由 `CuMemory` 负责。

## 运行时兼容层

如果目标工具链缺少标准 `setjmp`、`longjmp` 或部分软浮点 helper，可在 `tests/board/board_runtime.c` 中集中放置最小兼容实现，CMake 中也可以补充目标 SDK 提供的软浮点库。

注意：CuTest 原本依赖 `setjmp`、`longjmp` 在断言失败时跳出当前测试。若目标板只能提供最小 stub，失败断言可能无法真正中断当前测试，只适合先打通链接和基础用例。后续如果目标 SDK 或工具链能提供真实实现，应优先替换为真实实现。

## 测试注册方式

目标项目的测试文件继续使用 CuTest 的测试函数风格。涉及 SDK 头文件或目标板类型时，建议测试文件包含目标项目自己的 `board_test_config.h`，不要直接包含 `CuTest.h`：

```c
#include "board_test_config.h"

void TestTHBoard_StatusRoundTrip(CuTest *tc)
{
    CuAssertIntEquals(tc, 0, 0);
}
```

建议脚本只扫描 `tests/board/board_test_*.c`，自动生成 `tests/board/board_registry.c`。生成文件提供一个统一 suite：

```c
CuSuite *BoardTestGetSuite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestTHBoard_StatusRoundTrip);

    return suite;
}
```

`board_runner.c` 只负责运行该 suite：

```c
CuMemoryReset();
suite = BoardTestGetSuite();
CuSuiteRun(suite);
CuSuiteSummary(suite, output);
CuSuiteDetails(suite, output);
CuSuiteDelete(suite);
```

这样新增测试时只需要新增或修改 `board_test_*.c`，不需要手工维护数组、计数和逐条运行逻辑。

## 构建流程调整

目标项目的 `board-test` action 建议执行以下步骤：

1. 扫描 `tests/board/board_test_*.c`。
2. 生成 `tests/board/board_registry.c`。
3. 配置 CMake，打开 `BUILD_BOARD_TESTS=ON`。
4. 编译目标板测试固件。
5. 生成可烧录的 board-test 固件产物。

目标项目后续不再需要：

- `BUILD_TESTS`
- `cmake/tests.cmake`
- `tests/unit/`
- `unit_tests.exe`
- `build.ps1 -Action test`
- `BOARD_TEST_FIRMWARE_ENABLE`

## CMake 接入示例

目标项目的 `cmake/board_tests.cmake` 可以按如下思路组织：

```cmake
file(GLOB_RECURSE BOARD_TEST_CASE_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/board/board_test_*.c"
)

set(BOARD_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/cutest/CuTest.c"
    "${CMAKE_SOURCE_DIR}/cutest/memory/CuMemory.c"
    "${CMAKE_SOURCE_DIR}/tests/board/board_runner.c"
    "${CMAKE_SOURCE_DIR}/tests/board/board_port.c"
    "${CMAKE_SOURCE_DIR}/tests/board/board_runtime.c"
    "${CMAKE_SOURCE_DIR}/tests/board/board_registry.c"
    ${BOARD_TEST_CASE_SOURCES}
)

set(BOARD_TEST_INCLUDES
    "${CMAKE_SOURCE_DIR}/cutest"
    "${CMAKE_SOURCE_DIR}/tests/board"
)

set(BOARD_TEST_COMPILE_DEFINITIONS
    CUTEST_USE_MEMORY_MIDDLEWARE=1
    CUTEST_MEMORY_HEAP_SIZE=8192
    CUTEST_MEMORY_ALIGNMENT=4
    MAX_TEST_CASES=64
    SUITE_INLINE_CAPACITY=8
    SUITE_INC=8
    STRING_MAX=128
    STRING_INC=128
    HUGE_STRING_LEN=512
)

set(BOARD_TEST_LINK_LIBS
    c
    m
)
```

如果目标芯片需要额外运行时库，例如 Telink TC32 软浮点库，应在目标项目的 `board_tests.cmake` 中补充到链接列表。具体变量名应贴合目标项目现有 CMake 结构，不要求照搬。

## 风险点

### `setjmp` 和 `longjmp`

CuTest 依赖 `setjmp`、`longjmp` 在断言失败时跳出当前测试。如果目标工具链或目标 SDK 不支持标准实现，需要单独确认替代方案。

最小 stub 可以帮助先完成链接，但会改变失败断言后的控制流。迁移初期可以接受这个折中，后续如果需要覆盖失败路径，应补真实实现或改造目标板断言退出机制。

### `printf` 与格式化缓冲

目标板 RAM 较小，`HUGE_STRING_LEN` 不宜直接使用较大的桌面默认值。建议从 `512` 起步，实际再根据失败输出长度调整。

### CuMemory 堆大小

`CUTEST_MEMORY_HEAP_SIZE` 过小会导致 suite、字符串或失败消息分配失败。建议迁移初期保留 `CuMemoryGetFreeSize()` 和 `CuMemoryGetMinimumEverFreeSize()` 的调试输出，用于确认真实峰值。

### 测试数量上限

`MAX_TEST_CASES` 应覆盖所有 board-test 用例数量。生成 registry 后可以在构建阶段统计测试数量，超过上限时直接失败，而不是等固件运行时静默丢测试。

## 推荐迁移步骤

1. 将本仓库完整 `src/` 复制到目标项目并重命名为 `cutest/`。
2. 在目标项目 `cutest/scripts/` 下新增 board-test 专用生成配置。
3. 新建 `board_runner.c`、`board_port.c`、`board_runtime.c`、`board_test_config.h` 等板级适配文件。
4. 改造 `board_tests.cmake`，加入 `CuMemory.c` 和 board-test 编译宏。
5. 删除旧的全局 `malloc/free` 适配。
6. 将手工 registry 改为脚本生成 `board_registry.c`。
7. 调整 `build.ps1 -Action board-test`，在 CMake 配置前生成 registry。
8. 废弃 PC 侧 unit test 构建入口。
9. 删除 `BOARD_TEST_FIRMWARE_ENABLE`，改用弱 `boardTestMain()` 钩子。
10. 编译 board-test 固件并根据内存水位调整 `CUTEST_MEMORY_HEAP_SIZE`。

## 本仓库保持事项

为避免目标项目需求反向污染通用 CuTest，本仓库后续应保持以下边界：

- `src/` 继续提供通用 CuTest、Memory Middleware 和聚合生成器。
- 目标项目的 board-test registry 生成模板放在目标项目内维护。
- 目标项目的串口、日志、芯片启动和烧录流程不进入本仓库。
- 如果发现 `CuMemory` 或核心 CuTest 存在通用缺陷，再以通用修复方式回到本仓库处理。
