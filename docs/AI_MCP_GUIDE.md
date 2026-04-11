# CuTest AI / MCP 使用指南

本文档面向两类场景:

1. 让 AI Agent 更快理解当前仓库.
2. 通过 MCP 服务把项目能力暴露给支持 MCP 的客户端.

## 项目适合 AI 参与的原因

`CuTest` 是一个体量小、边界清晰的 C 单元测试框架, 很适合让 AI 做下面这些事情:

- 阅读和解释断言宏与测试失败信息.
- 为现有 API 自动补测试用例.
- 定位 `CuArray` / `CuString` / `CuSuite` 相关行为.
- 生成新的 `AllTests.c` 聚合文件或测试模板.
- 在修改 `CuTest.c` 后回归执行测试并总结结果.

## 仓库结构速览

| 路径 | 作用 |
| --- | --- |
| `CuTest.h` | 对外头文件, 包含核心类型、断言宏、套件宏 |
| `CuTest.c` | 框架实现, 包括字符串、数组、断言与测试运行逻辑 |
| `test/CuTestTest.c` | 框架自测 |
| `test/AllTests.c` | 测试入口 |
| `test/CMakeLists.txt` | 测试工程构建配置 |
| `make-tests.sh/.ps1` | 自动生成测试聚合代码的脚本 |
| `README` | 原版英文说明 |
| `readme.md` | 中文整理版说明 |

## AI 读取项目时建议重点关注

### 1. 三层核心抽象

- `CuTest`: 单个测试用例, 保存名称、函数指针、失败状态和消息.
- `CuSuite`: 测试集合, 负责聚合与执行.
- `CuString` / `CuArray`: 框架内部的辅助容器.

### 2. 关键断言入口

最值得让 AI 熟悉的是这些宏:

- `CuAssert`
- `CuAssertTrue`
- `CuAssertStrEquals`
- `CuAssertIntEquals`
- `CuAssertDblEquals`
- `CuAssertArrEquals`
- `CuAssertPtrEquals`
- `CuAssertPtrNotNull`
- `SUITE_ADD_TEST`

### 3. 重要行为约定

- 失败信息最终由 `CuFailInternal` 统一写入 `tc->message`.
- `CuTestRun` 使用 `setjmp/longjmp` 中断失败测试.
- `CuSuiteRun` 只累积失败数, 不做复杂调度.
- `CuStringAppend(NULL)` 会写入字符串 `"NULL"`.
- `CuArrayInsert` 和 `CuStringInsert` 超界时会自动夹到末尾.

## 推荐给 AI 的任务提示词

### 分析框架实现

```text
请先阅读 CuTest.h、CuTest.c 和 test/CuTestTest.c，
总结 CuTest 的核心抽象、断言流程、失败信息格式和测试执行路径。
```

### 生成新测试

```text
请为 CuStringInsert 补充边界测试，包括:
1. pos 大于 length
2. 插入空字符串
3. 原字符串为空
4. 连续多次插入
并保持当前 CuTest 测试风格。
```

### 做回归分析

```text
请修改完成后执行测试，并说明:
1. 变更影响了哪些函数
2. 是否新增或改变失败输出格式
3. 是否存在内存释放或边界风险
```

## MCP 服务能力设计

仓库里附带了一个轻量级 MCP 服务:

- 路径: `mcp/cutest_mcp_server.py`
- 传输: `stdio`
- 依赖: Python 3.13+ 标准库

服务提供的能力适合给 AI 做只读分析和回归执行:

- `project_overview`: 返回项目摘要和核心文件说明
- `list_asserts`: 列出断言宏
- `list_test_cases`: 枚举测试套件和测试用例
- `describe_symbol`: 读取某个符号的声明、实现与测试位置
- `search_repo`: 在仓库里搜索代码或文档文本
- `run_tests`: 配置、构建并执行 `test/` 下的测试

## 推荐工作流

### 方案 1: 先文档, 后代码

适合第一次接入 AI:

1. 先让 AI 读取本文件和 `readme.md`.
2. 通过 `describe_symbol` 理解目标 API.
3. 通过 `list_test_cases` 看现有覆盖.
4. 修改代码后用 `run_tests` 做回归.

### 方案 2: 直接走 MCP

适合已经在 Cursor、Claude Desktop、Codex 等客户端中配置 MCP:

1. 启动 `mcp/cutest_mcp_server.py`
2. 让 Agent 调用 `project_overview`
3. 再根据任务调用 `describe_symbol` / `search_repo`
4. 需要验证时调用 `run_tests`

## AI 接入时的上下文建议

给模型提供下面这些上下文通常就够了:

- `CuTest.h`
- `CuTest.c`
- `test/CuTestTest.c`
- `test/AllTests.c`
- 本文档

如果任务只涉及使用方式而不改框架内部, 还可以只给:

- `CuTest.h`
- `README` 或 `readme.md`

## 注意事项

- `run_tests` 会在仓库下创建独立构建目录, 不会改动源码.
- 当前 MCP 服务更偏向“代码分析助手”, 不是远程托管服务.
- 如果你后续希望把它扩展成真正的 SDK 文档服务器, 建议继续增加 `resources/list` 与 `resources/read` 的结构化内容.
