# Repository Guidelines

## 项目结构与模块组织

仓库根目录提供 CuTest 库本体，核心源码只有 [`CuTest.c`](/C:/Users/Sonoff_yzy/workspace/github/cutest/CuTest.c) 和 [`CuTest.h`](/C:/Users/Sonoff_yzy/workspace/github/cutest/CuTest.h)。`test/` 目录是独立测试工程，其中 `CuTestTest.c` 存放框架自测用例，`AllTests.c` 负责聚合 Suite 并提供 `main()`。根目录下的 `make-tests.sh` 与 `make-tests.ps1` 用于从测试源自动生成聚合代码。`build-coverage/`、`build-coverage-gcc/` 属于本地构建产物，不应手工维护。

## 构建、测试与开发命令

- `cmake -S test -B test/build`：生成测试工程。
- `cmake --build test/build`：编译 `cutest` 测试可执行文件。
- `ctest --test-dir test/build --output-on-failure`：运行已注册测试；若未启用 CTest，可直接执行 `test/build/cutest`。
- `cmake -S test -B build-coverage -DCUTEST_ENABLE_COVERAGE=ON`：启用 GCC/Clang 覆盖率构建。
- `./make-tests.sh file1.c file2.c > AllTests.c` 或 `./make-tests.ps1 -Files "file1.c" "file2.c" > AllTests.c`：按测试函数生成聚合入口。

## 代码风格与命名约定

项目使用 C99，格式化规则见 `.clang-format`。保持现有 C 风格：4 空格缩进，左花括号单独成行，宏名全大写，公开类型与函数沿用 `CuString`、`CuSuiteAdd` 这类 `Cu` 前缀 PascalCase 命名。测试函数统一命名为 `TestXxx`，测试套件入口使用 `XxxGetSuite`。修改时优先最小改动，不引入无关重构。

## 测试要求

新增功能或修复必须补充到 `test/CuTestTest.c`，并确保已加入对应 Suite；未注册的测试不会执行。重点覆盖边界条件，例如容量上限、`NULL` 输入、长度为 0、扩容和失败路径。涉及断言、字符串或数组处理时，优先补回归测试。

## 提交与合并请求规范

最近提交同时存在 `fix:`、`feat:`、`docs:` 前缀，建议继续使用这种简洁风格，例如 `fix: handle NULL buffer in CuStringAppend`。每个提交只做一类变更。Pull Request 需说明改动目的、影响范围、测试命令和结果；若修改公共接口或生成脚本，补充示例命令或输出片段。
