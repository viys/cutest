#!/usr/bin/env python3
"""Build tests, run them, and generate an HTML coverage report with gcovr."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def run(command: list[str], *, cwd: Path | None = None, verbose: bool = False) -> None:
    """Run a command and stop immediately when it fails."""
    if verbose:
        print(f"+ {' '.join(command)}", flush=True)
    result = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        capture_output=not verbose,
        text=True,
    )
    if result.returncode != 0:
        if not verbose:
            print(result.stdout, end="")
            print(result.stderr, end="", file=sys.stderr)
        raise subprocess.CalledProcessError(result.returncode, command)


def command_path(name: str) -> str:
    """Return an executable path or raise a useful configuration error."""
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"Required command was not found: {name}")
    return path


def parse_args() -> argparse.Namespace:
    """Parse project-specific coverage settings from the command line."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--cmake-source-dir", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--report-dir", type=Path, required=True)
    parser.add_argument("--filter", dest="filters", action="append", required=True)
    parser.add_argument("--cmake-arg", action="append", default=[])
    parser.add_argument("--verbose", action="store_true")
    return parser.parse_args()


def main() -> int:
    """Execute the generic configure, build, test, and report workflow."""
    args = parse_args()
    source_root = args.source_root.resolve()
    cmake_source_dir = (source_root / args.cmake_source_dir).resolve()
    build_dir = (source_root / args.build_dir).resolve()
    report_dir = (source_root / args.report_dir).resolve()
    report_path = report_dir / "coverage.html"

    cmake = command_path("cmake")
    ctest = command_path("ctest")

    cmake_args = [
        cmake,
        "-S", str(cmake_source_dir),
        "-B", str(build_dir),
        "--fresh",
        "-DCMAKE_C_FLAGS=--coverage -O0 -g",
        "-DCMAKE_EXE_LINKER_FLAGS=--coverage",
    ]
    ninja = shutil.which("ninja")
    gcc = shutil.which("gcc")
    if ninja and gcc:
        cmake_args.extend(["-G", "Ninja", f"-DCMAKE_C_COMPILER={gcc}"])
    cmake_args.extend(args.cmake_arg)

    print("=== Building Coverage Tests ===")
    run(cmake_args, verbose=args.verbose)
    run([cmake, "--build", str(build_dir), "--target", "clean"], verbose=args.verbose)
    run([cmake, "--build", str(build_dir), "--parallel"], verbose=args.verbose)

    # 覆盖率仍需运行测试以生成 gcda 数据，但默认不输出测试执行日志。
    run([ctest, "--test-dir", str(build_dir)], verbose=False)

    report_dir.mkdir(parents=True, exist_ok=True)
    gcovr_args = [
        sys.executable, "-m", "gcovr", str(build_dir),
        "--root", str(source_root),
        "--html-details", str(report_path),
        "--html-title", "Unit Test Coverage Report",
    ]
    for coverage_filter in args.filters:
        gcovr_args.extend(["--filter", coverage_filter])

    print("=== Generating Coverage Report ===")
    run(gcovr_args, verbose=args.verbose)
    print(f"Coverage report generated: {report_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode) from error
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
