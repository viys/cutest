# CuTest MCP 服务

这是一个为当前仓库准备的轻量级 `MCP` 服务, 用来帮助 AI 客户端理解并操作 `CuTest` 项目。

## 特点

- 零第三方依赖
- 使用 `stdio` 传输
- 可以读取项目摘要、断言宏、测试用例
- 可以搜索符号并执行测试

## 启动方式

在仓库根目录执行:

```powershell
python .\mcp\cutest_mcp_server.py
```

## 可用工具

- `project_overview`
- `list_asserts`
- `list_test_cases`
- `describe_symbol`
- `search_repo`
- `run_tests`

## MCP 配置示例

### Claude Desktop

```json
{
  "mcpServers": {
    "cutest": {
      "command": "python",
      "args": [
        "C:/Users/jiyon/workspace/github/cutest/mcp/cutest_mcp_server.py"
      ]
    }
  }
}
```

### 通用 stdio MCP 客户端

```json
{
  "servers": {
    "cutest": {
      "transport": "stdio",
      "command": "python",
      "args": [
        "C:/Users/jiyon/workspace/github/cutest/mcp/cutest_mcp_server.py"
      ]
    }
  }
}
```

## 推荐调用顺序

```text
project_overview
list_test_cases
describe_symbol(symbol="CuStringInsert")
run_tests
```

## 适合的任务

- 解释某个断言宏的用途
- 查某个 API 是否已有测试
- 修改后自动回归测试
- 让 AI 先理解项目再开始补代码

## 注意

`run_tests` 依赖本机已安装可用的 C 编译器和 `cmake`。如果当前机器没有 `MSVC`、`MinGW` 或 `clang`，它会明确返回环境错误。
