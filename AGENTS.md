# Repository Guidelines

## 项目结构与模块组织

仓库根目录通过 `src/` 目录提供 CuTest 库本体，核心源码只有 [`CuTest.c`](/C:/Users/Sonoff_yzy/workspace/github/cutest/src/CuTest.c) 和 [`CuTest.h`](/C:/Users/Sonoff_yzy/workspace/github/cutest/src/CuTest.h)。`src/CMakeLists.txt` 负责核心库构建。`test/` 目录是独立测试工程，其中 `CuTestTest.c` 存放框架自测用例，`AllTests.c` 由脚本按 `void Test...` 函数自动聚合并提供 `main()`。`scripts/` 目录下的 `make-tests.sh` 与 `make-tests.ps1` 用于从测试源自动生成聚合代码。根目录 `test.ps1` 统一驱动测试工程的配置、编译、执行、清理和删除操作，默认使用仓库根目录下的 `build/` 作为构建输出目录。`build-coverage/`、`build-coverage-gcc/` 属于本地构建产物，不应手工维护。

## 构建、测试与开发命令

- `./test.ps1`：依次执行 `update -> cmake -> make -> run`，用于完整刷新聚合测试、配置、编译并运行测试。
- `./test.ps1 update`：调用 `scripts/make-tests.ps1` 扫描 `test/` 下的测试源并更新 `test/AllTests.c`。
- `./test.ps1 cmake`：在仓库根目录下生成 `build/` 测试工程。
- `./test.ps1 make`：编译 `build/` 中的 `cutest` 测试可执行文件；若 `build/` 不存在会先自动执行配置。
- `./test.ps1 run`：执行 `build/` 中的 `cutest` 测试程序；若可执行文件不存在则提示先执行 `make`。
- `./test.ps1 clean`：清理 `build/` 中的编译产物但保留 CMake 配置。
- `./test.ps1 delete`：删除仓库根目录下的 `build/`。
- `ctest --test-dir build --output-on-failure`：运行已注册测试；若未启用 CTest，可直接执行 `build/cutest`。
- `cmake -S test -B build-coverage -DCUTEST_ENABLE_COVERAGE=ON`：启用 GCC/Clang 覆盖率构建。
- `./scripts/make-tests.sh file1.c file2.c > AllTests.c` 或 `./scripts/make-tests.ps1 -Files "file1.c" "file2.c" > AllTests.c`：按测试函数生成聚合入口。

## 代码风格与命名约定

项目使用 C99，格式化规则见 `.clang-format`。保持现有 C 风格：4 空格缩进，左花括号单独成行，宏名全大写，公开类型与函数沿用 `CuString`、`CuSuiteAdd` 这类 `Cu` 前缀 PascalCase 命名。测试函数统一命名为 `TestXxx`，脚本会直接扫描这些函数生成 `AllTests.c`。修改时优先最小改动，不引入无关重构。

## 测试要求

新增功能或修复必须补充到 `test/CuTestTest.c`，并确保函数名符合 `TestXxx` 扫描规则；未被脚本收集到的测试不会执行。重点覆盖边界条件，例如容量上限、`NULL` 输入、长度为 0、扩容和失败路径。涉及断言、字符串或数组处理时，优先补回归测试。

## 提交与合并请求规范

最近提交同时存在 `fix:`、`feat:`、`docs:` 前缀，建议继续使用这种简洁风格，例如 `fix: handle NULL buffer in CuStringAppend`。每个提交只做一类变更。Pull Request 需说明改动目的、影响范围、测试命令和结果；若修改公共接口或生成脚本，补充示例命令或输出片段。

## GitHub Project 与 Issue 流程

仓库已接入 GitHub Project 自动入板流程。新增 issue 或拆分子任务时，不仅要创建 issue 本身，还要同步处理 Project 字段和 label，避免条目进入看板后缺少分类信息。

- GitHub Project 字段固定取值如下：
  - `Status`：`Todo` / `In Progress` / `Done`
  - `Priority`：`High` / `Medium` / `Low`
  - `Type`：`Bug` / `Feature` / `Test` / `Docs`
  - `Area`：`Core` / `Test` / `Build` / `Docs` / `Tooling`
- 创建 issue 后，确认其已自动加入 GitHub Project。
- 至少补齐 `Status`，其余字段按项目当前约定补 `Priority`、`Type`、`Area`。
- 创建 issue 时同步检查并设置对应 label，不要遗漏 label。
- 若仓库缺少所需 label，应先创建再关联到 issue。
- 拆分父 issue 时，在子 issue 正文中写明关联关系，例如 `Parent issue: #19`。
- 涉及流程自动化的工作流文件必须存在于仓库默认分支；仅推送到开发分支不会触发 `issues` 类事件。
- 填写建议：
  - 结构优化、构建流程、脚本整理类任务优先使用 `Area=Tooling`。
  - 核心库源码变更优先使用 `Area=Core`。
  - 测试补充、测试修复、回归覆盖类任务优先使用 `Type=Test`。
  - 文档、README、移植说明类任务优先使用 `Type=Docs` 或 `Area=Docs`。
