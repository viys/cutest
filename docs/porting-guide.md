# CuTest 快速移植指南

本文面向准备在自己 C 项目中使用本仓库 CuTest 的开发者。目标是先用最少文件跑通一个测试，再按需要接入自动注册、板级测试、内存中间件和覆盖率报告。

更完整的工程化方案见 [CuTest 测试移植手册](test-porting-playbook.md)。

## 1. 选择接入方式

推荐保留本仓库的 `src/` 目录层级，并将 CuTest 放在目标项目根目录的 `cutest/` 下：

```text
your-project/
├── cutest/
│   └── src/
│       ├── CuTest.c
│       ├── CuTest.h
│       ├── CMakeLists.txt
│       ├── memory/
│       └── scripts/
├── tests/
└── CMakeLists.txt
```

可以复制固定版本，也可以用 Git submodule 等方式固定仓库版本。无论采用哪种方式，都不要在目标项目中修改 CuTest 核心文件来适配业务；项目差异应放在测试构建、runner、port 或 stub 中。

## 2. 复制所需文件

最小运行集：

- `src/CuTest.c`
- `src/CuTest.h`

需要自动扫描并注册 `Test...` 函数时，再引入：

- `src/scripts/make-tests.py`

目标环境不适合直接使用标准动态内存时，再引入：

- `src/memory/CuMemory.c`
- `src/memory/CUMemory.h`

需要生成主机测试覆盖率报告时，再引入：

- `src/scripts/generate_coverage_report.py`

不需要复制本仓库的 `test/`、`docs/`、`test.ps1` 或 `build/`。这些内容用于本仓库自测和说明，不是目标项目的运行依赖。

## 3. 先选择测试路径

| 被测行为 | 推荐路径 |
| --- | --- |
| 参数检查、纯逻辑、状态转换、编解码、格式化 | 主机侧 `unit test` |
| 中断、寄存器、真实时序、外设、总线、电平、功耗 | 目标硬件 `board test` |
| 逻辑可在主机验证，但最终行为依赖硬件 | 两条路径并存 |

第一次接入建议先选择一条路径跑通，不必同时建立两套测试工程。

## 4. 写第一个测试

```c
#include "CuTest.h"

void TestAddition(CuTest *tc)
{
    CuAssertIntEquals(tc, 4, 2 + 2);
}
```

测试函数应使用 `Test` 前缀，并保持 `void TestXxx(CuTest *tc)` 签名，便于生成器自动发现。

## 5. 接入 CMake

如果保留了 `src/CMakeLists.txt`，可以直接复用本仓库的库目标：

```cmake
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/cutest/src
    ${CMAKE_BINARY_DIR}/cutest
)

add_executable(app_unit_tests
    tests/unit/AllTests.c
    tests/unit/unit_test_example.c
    src/example.c
)

target_include_directories(app_unit_tests PRIVATE src)
target_link_libraries(app_unit_tests PRIVATE CuTest)

enable_testing()
add_test(NAME app_unit_tests COMMAND app_unit_tests)
```

如果只复制了 `CuTest.c` 和 `CuTest.h`，也可以直接把 `CuTest.c` 加入测试可执行目标，并将其目录加入 include path。

`AllTests.c` 可以先手写；用例增多后，再按照完整手册接入 `make-tests.py` 自动生成。主机测试的 `main()` 必须在测试失败时返回非零值，否则 CI 无法识别失败。

## 6. 构建并验收

目标项目至少应提供一条稳定命令完成配置、构建和执行。例如：

```powershell
cmake -S . -B build/unit-tests -DBUILD_UNIT_TESTS=ON
cmake --build build/unit-tests
ctest --test-dir build/unit-tests --output-on-failure
```

验收标准：

- 测试可执行文件成功生成。
- 至少一个测试确实被注册并执行。
- 通过时进程返回 `0`。
- 故意制造断言失败时进程返回非 `0`，CTest 同步失败。
- 新增或删除测试文件后，聚合入口能够正确更新。

## 7. 覆盖率脚本

覆盖率实现以本仓库的 Python 脚本为准：

```text
src/scripts/generate_coverage_report.py
```

本仓库自身通过以下命令调用它：

```powershell
./test.ps1 coverage
```

移植到目标项目后，保留 Python 脚本的参数式调用方式，只在目标项目的顶层脚本中传入自己的源码根目录、测试 CMake 目录、构建目录、报告目录和源码过滤规则。不要复制其他业务项目的覆盖率脚本路径或固定源码列表。完整命令模板见 [覆盖率报告](test-porting-playbook.md#覆盖率报告)。
