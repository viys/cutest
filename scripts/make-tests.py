#!/usr/bin/env python3
"""Generate a CuTest aggregation source file from configured test sources."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Iterable


REQUIRED_CONFIG_KEYS = {
    "include_lines",
    "test_regex",
    "extern_template",
    "run_function_name",
    "suite_add_template",
    "run_function_prefix_lines",
    "run_function_suffix_lines",
    "emit_main",
    "main_lines",
}

SUPPORTED_FIELDS = {"test_name", "run_function_name"}
PLACEHOLDER_PATTERN = re.compile(r"\{([a-z_]+)\}")


class GeneratorError(Exception):
    """Raised when generator inputs or templates are invalid."""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate an AllTests-style aggregation source file."
    )
    parser.add_argument(
        "--config",
        required=True,
        help="Path to the JSON generator configuration file.",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Path to the generated output source file.",
    )
    parser.add_argument(
        "--files",
        nargs="*",
        help="Optional explicit test source files. Overrides config files/globs.",
    )
    parser.add_argument(
        "--emit-main",
        choices=("true", "false"),
        help="Override the config emit_main flag.",
    )
    return parser.parse_args()


def load_config(config_path: Path) -> dict:
    try:
        with config_path.open("r", encoding="utf-8") as config_file:
            config = json.load(config_file)
    except FileNotFoundError as exc:
        raise GeneratorError(f"Missing config file: {config_path}") from exc
    except json.JSONDecodeError as exc:
        raise GeneratorError(
            f"Invalid JSON in config file {config_path}: {exc}"
        ) from exc

    missing_keys = sorted(REQUIRED_CONFIG_KEYS.difference(config))
    if missing_keys:
        raise GeneratorError(
            "Config is missing required keys: " + ", ".join(missing_keys)
        )

    if "files" not in config and "globs" not in config:
        raise GeneratorError("Config must define at least one of: files, globs")

    return config


def validate_template(template: str, template_name: str) -> None:
    for field_name in PLACEHOLDER_PATTERN.findall(template):
        if field_name not in SUPPORTED_FIELDS:
            raise GeneratorError(
                f"Template '{template_name}' uses unsupported placeholder "
                f"'{field_name}'"
            )


def validate_templates(config: dict) -> None:
    validate_template(config["extern_template"], "extern_template")
    validate_template(config["suite_add_template"], "suite_add_template")

    for index, line in enumerate(config["run_function_prefix_lines"]):
        validate_template(line, f"run_function_prefix_lines[{index}]")

    for index, line in enumerate(config["run_function_suffix_lines"]):
        validate_template(line, f"run_function_suffix_lines[{index}]")

    for index, line in enumerate(config["main_lines"]):
        validate_template(line, f"main_lines[{index}]")


def resolve_sources(
    config: dict, config_path: Path, cli_files: list[str] | None
) -> list[Path]:
    base_dir = config_path.parent
    raw_paths: list[Path] = []
    excluded_paths: set[Path] = set()

    if cli_files:
        raw_paths.extend(Path(file_path) for file_path in cli_files)
    else:
        for file_path in config.get("files", []):
            raw_paths.append(base_dir / file_path)
        for pattern in config.get("globs", []):
            raw_paths.extend(base_dir.glob(pattern))
        for file_path in config.get("exclude_files", []):
            excluded_paths.add((base_dir / file_path).resolve())
        for pattern in config.get("exclude_globs", []):
            excluded_paths.update(path.resolve() for path in base_dir.glob(pattern))

    resolved_paths: list[Path] = []
    seen_paths: set[Path] = set()

    for path in raw_paths:
        resolved_path = path.resolve()
        if resolved_path in excluded_paths:
            continue
        if not resolved_path.exists():
            raise GeneratorError(f"Input file does not exist: {path}")
        if resolved_path.is_dir():
            raise GeneratorError(f"Input path must be a file, not a directory: {path}")
        if resolved_path.suffix.lower() != ".c":
            raise GeneratorError(f"Input file must be a .c source file: {path}")
        if resolved_path not in seen_paths:
            seen_paths.add(resolved_path)
            resolved_paths.append(resolved_path)

    resolved_paths.sort(key=lambda path: str(path).lower())

    if not resolved_paths:
        raise GeneratorError("No input test source files were resolved")

    return resolved_paths


def collect_tests(source_files: Iterable[Path], pattern: re.Pattern[str]) -> list[str]:
    test_names: list[str] = []
    seen_tests: set[str] = set()

    for source_file in source_files:
        try:
            content = source_file.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            content = source_file.read_text(encoding="utf-8-sig")

        for match in pattern.finditer(content):
            test_name = match.group("test_name")
            if test_name in seen_tests:
                raise GeneratorError(f"Duplicate test name detected: {test_name}")
            seen_tests.add(test_name)
            test_names.append(test_name)

    if not test_names:
        raise GeneratorError("No test functions matched the configured test_regex")

    return test_names


def render_line(template: str, *, test_name: str | None, run_function_name: str) -> str:
    values = {
        "test_name": test_name,
        "run_function_name": run_function_name,
    }
    rendered = template

    for field_name in PLACEHOLDER_PATTERN.findall(template):
        value = values.get(field_name)
        if value is None:
            raise GeneratorError(
                f"Template requires '{field_name}' but no value was provided"
            )
        rendered = rendered.replace("{" + field_name + "}", value)

    return rendered


def render_lines(
    templates: Iterable[str], *, test_name: str | None, run_function_name: str
) -> list[str]:
    return [
        render_line(
            template, test_name=test_name, run_function_name=run_function_name
        )
        for template in templates
    ]


def build_output(config: dict, test_names: list[str], emit_main: bool) -> str:
    run_function_name = config["run_function_name"]
    lines: list[str] = []

    lines.extend(config["include_lines"])
    lines.append("")

    for test_name in test_names:
        lines.append(
            render_line(
                config["extern_template"],
                test_name=test_name,
                run_function_name=run_function_name,
            )
        )

    lines.append("")
    lines.extend(
        render_lines(
            config["run_function_prefix_lines"],
            test_name=None,
            run_function_name=run_function_name,
        )
    )

    for test_name in test_names:
        lines.append(
            render_line(
                config["suite_add_template"],
                test_name=test_name,
                run_function_name=run_function_name,
            )
        )

    lines.extend(
        render_lines(
            config["run_function_suffix_lines"],
            test_name=None,
            run_function_name=run_function_name,
        )
    )

    if emit_main:
        lines.append("")
        lines.extend(
            render_lines(
                config["main_lines"],
                test_name=None,
                run_function_name=run_function_name,
            )
        )

    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    config_path = Path(args.config).resolve()
    output_path = Path(args.output).resolve()

    try:
        config = load_config(config_path)
        validate_templates(config)
        source_files = resolve_sources(config, config_path, args.files)
        test_pattern = re.compile(config["test_regex"], re.MULTILINE)
        test_names = collect_tests(source_files, test_pattern)
        emit_main = config["emit_main"]
        if args.emit_main is not None:
            emit_main = args.emit_main == "true"
        output = build_output(config, test_names, emit_main)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(output, encoding="utf-8", newline="\n")
    except GeneratorError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
