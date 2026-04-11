#!/usr/bin/env python3
"""Minimal MCP server for the CuTest repository.

This server uses JSON-RPC over stdio with Content-Length framing.
It exposes a few repository-aware tools without third-party packages.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import traceback
from pathlib import Path
from typing import Any


SERVER_NAME = "cutest-mcp"
SERVER_VERSION = "0.1.0"
PROTOCOL_VERSION = "2024-11-05"
ROOT = Path(__file__).resolve().parent.parent
TEST_DIR = ROOT / "test"
DEFAULT_BUILD_DIR = ROOT / "build" / "mcp-tests"

TEXT_FILES = [
    ROOT / "CuTest.h",
    ROOT / "CuTest.c",
    ROOT / "README",
    ROOT / "readme.md",
    ROOT / "CMakeLists.txt",
    TEST_DIR / "CMakeLists.txt",
    TEST_DIR / "AllTests.c",
    TEST_DIR / "CuTestTest.c",
]


def write_message(payload: dict[str, Any]) -> None:
    data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    header = f"Content-Length: {len(data)}\r\n\r\n".encode("ascii")
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()


def read_message() -> dict[str, Any] | None:
    headers: dict[str, str] = {}

    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None

        if line in (b"\r\n", b"\n"):
            break

        text = line.decode("ascii", errors="replace")
        name, _, value = text.partition(":")
        headers[name.strip().lower()] = value.strip()

    content_length = int(headers.get("content-length", "0"))
    if content_length <= 0:
        return None

    body = sys.stdin.buffer.read(content_length)
    if not body:
        return None
    return json.loads(body.decode("utf-8"))


def make_text_result(text: str) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": text}]}


def normalize_rel(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def tool_project_overview(_: dict[str, Any]) -> str:
    return "\n".join(
        [
            "CuTest is a compact C unit test framework.",
            "",
            "Core files:",
            "- CuTest.h: public API, types, assertion macros, suite macros",
            "- CuTest.c: implementation of CuArray, CuString, CuTest, CuSuite",
            "- test/CuTestTest.c: self-tests for the framework",
            "- test/AllTests.c: executable entry point for all tests",
            "",
            "Primary abstractions:",
            "- CuTest: single test case",
            "- CuSuite: collection of tests",
            "- CuString and CuArray: internal helper containers",
            "",
            "Notable behaviors:",
            "- Assertions eventually fail through CuFailInternal",
            "- CuTestRun uses setjmp/longjmp for failure control flow",
            '- CuStringAppend(NULL) stores the literal string "NULL"',
            "- Tests are built from test/CMakeLists.txt",
        ]
    )


def parse_assert_macros(include_internal: bool) -> list[str]:
    text = read_text(ROOT / "CuTest.h")
    matches = re.findall(r"^#define\s+([A-Za-z0-9_]+)\(", text, flags=re.MULTILINE)
    names = []
    for name in matches:
        if name.startswith("CuAssert") or name == "CuFail":
            names.append(name)
    if include_internal:
        internal = re.findall(
            r"^void\s+(Cu[A-Za-z0-9_]+_Line(?:Msg)?)\(",
            text,
            flags=re.MULTILINE,
        )
        names.extend(internal)
    return sorted(set(names))


def tool_list_asserts(arguments: dict[str, Any]) -> str:
    include_internal = bool(arguments.get("include_internal", False))
    macros = parse_assert_macros(include_internal)
    lines = ["Available assertion entries:"]
    lines.extend(f"- {name}" for name in macros)
    return "\n".join(lines)


def collect_suites() -> dict[str, list[str]]:
    suites: dict[str, list[str]] = {}
    for path in TEXT_FILES:
        if path.suffix != ".c":
            continue
        text = read_text(path)
        current_suite: str | None = None
        for line in text.splitlines():
            suite_match = re.search(r"CuSuite\*\s+([A-Za-z0-9_]+GetSuite)\s*\(", line)
            if suite_match:
                current_suite = suite_match.group(1)
                suites.setdefault(current_suite, [])
                continue
            test_match = re.search(r"SUITE_ADD_TEST\(\s*[^,]+,\s*([A-Za-z0-9_]+)\s*\)", line)
            if test_match and current_suite:
                suites[current_suite].append(test_match.group(1))
    return suites


def tool_list_test_cases(arguments: dict[str, Any]) -> str:
    requested_suite = arguments.get("suite")
    suites = collect_suites()
    if requested_suite:
        tests = suites.get(requested_suite)
        if tests is None:
            return f"Suite not found: {requested_suite}"
        return "\n".join([f"{requested_suite}:", *[f"- {test}" for test in tests]])

    lines = []
    for suite_name in sorted(suites):
        lines.append(f"{suite_name}:")
        lines.extend(f"- {test}" for test in suites[suite_name])
        lines.append("")
    return "\n".join(lines).strip()


def find_symbol_locations(symbol: str) -> list[tuple[Path, int, str]]:
    results: list[tuple[Path, int, str]] = []
    pattern = re.compile(rf"\b{re.escape(symbol)}\b")
    for path in TEXT_FILES:
        if not path.exists():
            continue
        for line_number, line in enumerate(read_text(path).splitlines(), start=1):
            if pattern.search(line):
                results.append((path, line_number, line.strip()))
    return results


def tool_describe_symbol(arguments: dict[str, Any]) -> str:
    symbol = str(arguments.get("symbol", "")).strip()
    if not symbol:
        raise ValueError("symbol is required")

    matches = find_symbol_locations(symbol)
    if not matches:
        return f"Symbol not found: {symbol}"

    lines = [f"Symbol: {symbol}", ""]
    for path, line_number, line in matches[:30]:
        lines.append(f"- {normalize_rel(path)}:{line_number}: {line}")
    if len(matches) > 30:
        lines.append(f"... {len(matches) - 30} more matches omitted")
    return "\n".join(lines)


def tool_search_repo(arguments: dict[str, Any]) -> str:
    query = str(arguments.get("query", "")).strip()
    if not query:
        raise ValueError("query is required")

    max_results = int(arguments.get("max_results", 20))
    query_lower = query.lower()
    matches: list[str] = []

    for path in TEXT_FILES:
        if not path.exists():
            continue
        for line_number, line in enumerate(read_text(path).splitlines(), start=1):
            if query_lower in line.lower():
                matches.append(f"- {normalize_rel(path)}:{line_number}: {line.strip()}")
                if len(matches) >= max_results:
                    return "\n".join(matches)

    if not matches:
        return f'No matches for "{query}"'
    return "\n".join(matches)


def run_command(command: list[str], cwd: Path) -> tuple[int, str]:
    completed = subprocess.run(
        command,
        cwd=str(cwd),
        capture_output=True,
        text=True,
        timeout=180,
        check=False,
    )
    output = completed.stdout
    if completed.stderr:
        output = output + ("\n" if output else "") + completed.stderr
    return completed.returncode, output.strip()


def locate_test_binary(build_dir: Path) -> Path:
    candidates = []
    for path in build_dir.rglob("*"):
        if not path.is_file():
            continue
        name = path.name.lower()
        if os.name == "nt":
            if name == "cutest.exe":
                candidates.append(path)
        else:
            if name == "cutest" and os.access(path, os.X_OK):
                candidates.append(path)

    if not candidates:
        raise FileNotFoundError("Unable to locate built cutest test executable")

    candidates.sort(key=lambda item: len(item.parts))
    return candidates[0]


def tool_run_tests(arguments: dict[str, Any]) -> str:
    build_dir_arg = str(arguments.get("build_dir", "")).strip()
    build_dir = Path(build_dir_arg) if build_dir_arg else DEFAULT_BUILD_DIR
    if not build_dir.is_absolute():
        build_dir = ROOT / build_dir

    build_dir.mkdir(parents=True, exist_ok=True)

    configure_code, configure_output = run_command(
        ["cmake", "-S", str(TEST_DIR), "-B", str(build_dir)],
        ROOT,
    )
    if configure_code != 0:
        hint = ""
        if "No CMAKE_C_COMPILER could be found" in configure_output:
            hint = (
                "\n\nHint: install a local C toolchain first, such as "
                "Visual Studio Build Tools, MSVC, MinGW, or clang."
            )
        raise RuntimeError(
            "\n".join(
                [
                    f"Configure failed with exit code {configure_code}.",
                    "",
                    configure_output,
                ]
            )
            + hint
        )

    build_code, build_output = run_command(
        ["cmake", "--build", str(build_dir)],
        ROOT,
    )
    if build_code != 0:
        raise RuntimeError(
            "\n".join(
                [
                    f"Build failed with exit code {build_code}.",
                    "",
                    build_output,
                ]
            )
        )

    binary = locate_test_binary(build_dir)
    test_code, test_output = run_command([str(binary)], ROOT)

    return "\n".join(
        [
            f"Build directory: {normalize_rel(build_dir)}",
            f"Test binary: {normalize_rel(binary)}",
            f"Exit code: {test_code}",
            "",
            "Configure output:",
            configure_output or "(empty)",
            "",
            "Build output:",
            build_output or "(empty)",
            "",
            "Test output:",
            test_output or "(empty)",
        ]
    )


TOOLS: dict[str, dict[str, Any]] = {
    "project_overview": {
        "description": "Summarize the CuTest repository structure and design.",
        "inputSchema": {
            "type": "object",
            "properties": {},
            "additionalProperties": False,
        },
        "handler": tool_project_overview,
    },
    "list_asserts": {
        "description": "List public assertion macros from CuTest.h.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "include_internal": {
                    "type": "boolean",
                    "description": "Include internal *_Line and *_LineMsg helpers.",
                }
            },
            "additionalProperties": False,
        },
        "handler": tool_list_asserts,
    },
    "list_test_cases": {
        "description": "List available test suites and test cases.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "suite": {
                    "type": "string",
                    "description": "Optional suite function name such as CuGetSuite.",
                }
            },
            "additionalProperties": False,
        },
        "handler": tool_list_test_cases,
    },
    "describe_symbol": {
        "description": "Find declarations, definitions, and tests for a symbol.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "symbol": {
                    "type": "string",
                    "description": "Symbol name to locate, for example CuStringInsert.",
                }
            },
            "required": ["symbol"],
            "additionalProperties": False,
        },
        "handler": tool_describe_symbol,
    },
    "search_repo": {
        "description": "Case-insensitive text search across source and docs.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "max_results": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 100,
                    "default": 20,
                },
            },
            "required": ["query"],
            "additionalProperties": False,
        },
        "handler": tool_search_repo,
    },
    "run_tests": {
        "description": "Configure, build, and run tests from the test directory.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "build_dir": {
                    "type": "string",
                    "description": "Optional custom build directory.",
                }
            },
            "additionalProperties": False,
        },
        "handler": tool_run_tests,
    },
}


def handle_request(message: dict[str, Any]) -> dict[str, Any] | None:
    method = message.get("method")
    params = message.get("params", {})
    message_id = message.get("id")

    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": message_id,
            "result": {
                "protocolVersion": PROTOCOL_VERSION,
                "capabilities": {"tools": {}, "resources": {}},
                "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
                "instructions": (
                    "Use the repository-aware tools to inspect CuTest, "
                    "find symbols, and run the self-tests."
                ),
            },
        }

    if method == "notifications/initialized":
        return None

    if method == "ping":
        return {"jsonrpc": "2.0", "id": message_id, "result": {}}

    if method == "tools/list":
        tools = [
            {
                "name": name,
                "description": info["description"],
                "inputSchema": info["inputSchema"],
            }
            for name, info in TOOLS.items()
        ]
        return {"jsonrpc": "2.0", "id": message_id, "result": {"tools": tools}}

    if method == "tools/call":
        tool_name = params.get("name")
        arguments = params.get("arguments", {})
        info = TOOLS.get(tool_name)
        if info is None:
            return {
                "jsonrpc": "2.0",
                "id": message_id,
                "error": {"code": -32601, "message": f"Unknown tool: {tool_name}"},
            }
        try:
            text = info["handler"](arguments)
            return {
                "jsonrpc": "2.0",
                "id": message_id,
                "result": make_text_result(text),
            }
        except Exception as exc:
            details = "".join(traceback.format_exception_only(type(exc), exc)).strip()
            return {
                "jsonrpc": "2.0",
                "id": message_id,
                "result": {
                    "isError": True,
                    "content": [{"type": "text", "text": details}],
                },
            }

    return {
        "jsonrpc": "2.0",
        "id": message_id,
        "error": {"code": -32601, "message": f"Method not found: {method}"},
    }


def main() -> int:
    while True:
        message = read_message()
        if message is None:
            return 0
        response = handle_request(message)
        if response is not None:
            write_message(response)


if __name__ == "__main__":
    raise SystemExit(main())
