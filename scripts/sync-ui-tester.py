#!/usr/bin/env python3
"""Generate browser UI tester metadata from the firmware renderer.

The tester cannot execute the embedded C++ renderer directly, but it can share
the small set of UI constants that tend to drift: page order, page titles,
subtitles, palette colors, metric labels, units, ranges, and accents.
"""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UI_RENDERER_H = ROOT / "include" / "UiRenderer.h"
UI_RENDERER_CPP = ROOT / "src" / "UiRenderer.cpp"
CONFIG_H = ROOT / "include" / "Config.h"
OUTPUT = ROOT / "ui-tester" / "generated" / "firmware-ui.js"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_number(raw: str) -> float | int:
    value = raw.strip().rstrip("fFuUlL")
    if value.lower().startswith("0x") or value.lower().startswith("-0x"):
        return int(value, 16)
    parsed = float(value)
    return int(parsed) if parsed.is_integer() else parsed


def cpp_string(raw: str) -> str:
    return bytes(raw, "utf-8").decode("unicode_escape")


def rgb_to_hex(r: str, g: str, b: str) -> str:
    return f"#{int(r):02x}{int(g):02x}{int(b):02x}"


def lower_camel(name: str) -> str:
    return name[:1].lower() + name[1:]


def enum_key(name: str) -> str:
    return lower_camel(name)


def extract_braced_block(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise ValueError(f"Could not find function signature fragment: {signature}")

    open_brace = text.find("{", start)
    if open_brace < 0:
        raise ValueError(f"Could not find function body for: {signature}")

    depth = 0
    for index in range(open_brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace + 1 : index]

    raise ValueError(f"Unclosed function body for: {signature}")


def parse_config_constants(config_h: str) -> dict[str, float | int]:
    constants: dict[str, float | int] = {}
    pattern = re.compile(
        r"constexpr\s+(?:float|double|int|uint8_t|uint16_t|uint32_t|int16_t|int32_t)"
        r"\s+(\w+)\s*=\s*([-+]?(?:0x[0-9A-Fa-f]+|\d+(?:\.\d+)?)(?:[fFuUlL]*))\s*;"
    )
    for name, value in pattern.findall(config_h):
        constants[name] = parse_number(value)
    return constants


def parse_page_enum(ui_renderer_h: str) -> list[dict[str, int | str]]:
    match = re.search(r"enum\s+class\s+PageKind\s*:\s*uint8_t\s*\{(?P<body>.*?)\};", ui_renderer_h, re.S)
    if not match:
        raise ValueError("Could not find PageKind enum in UiRenderer.h")

    pages: list[dict[str, int | str]] = []
    next_value = 0
    for raw_entry in match.group("body").split(","):
        entry = raw_entry.strip()
        if not entry:
            continue
        entry_match = re.match(r"(\w+)(?:\s*=\s*(\d+))?", entry)
        if not entry_match:
            continue
        name, explicit_value = entry_match.groups()
        value = int(explicit_value) if explicit_value is not None else next_value
        pages.append({"enumName": name, "key": enum_key(name), "value": value})
        next_value = value + 1

    return sorted(pages, key=lambda page: int(page["value"]))


def parse_page_text(ui_renderer_cpp: str, function_name: str) -> dict[str, str]:
    body = extract_braced_block(ui_renderer_cpp, f"const char* UiRenderer::{function_name}_")
    page_text: dict[str, str] = {}
    for name, text in re.findall(r"case\s+PageKind::(\w+):\s*return\s+\"((?:\\.|[^\"])*)\";", body):
        page_text[name] = cpp_string(text)
    return page_text


def parse_page_count(ui_renderer_cpp: str, fallback: int) -> int:
    match = re.search(r"constexpr\s+uint8_t\s+kPageCount\s*=\s*(\d+)\s*;", ui_renderer_cpp)
    return int(match.group(1)) if match else fallback


def parse_palette(ui_renderer_cpp: str) -> dict[str, str]:
    palette: dict[str, str] = {}
    pattern = re.compile(
        r"uint16_t\s+(color\w+)\s*\(\)\s*\{\s*return\s+rgb565\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*;\s*\}"
    )
    for function_name, red, green, blue in pattern.findall(ui_renderer_cpp):
        palette[lower_camel(function_name.removeprefix("color"))] = rgb_to_hex(red, green, blue)
    return palette


def parse_metric_meta(ui_renderer_cpp: str, constants: dict[str, float | int]) -> dict[str, dict[str, object]]:
    body = extract_braced_block(ui_renderer_cpp, "void UiRenderer::metricMeta_")
    metrics: dict[str, dict[str, object]] = {}
    case_pattern = re.compile(
        r"case\s+MetricKind::(\w+):(?P<body>.*?)(?=case\s+MetricKind::|default:)",
        re.S,
    )
    for name, case_body in case_pattern.findall(body):
        title_match = re.search(r'title\s*=\s*"((?:\\.|[^"])*)";', case_body)
        unit_match = re.search(r'unit\s*=\s*"((?:\\.|[^"])*)";', case_body)
        min_match = re.search(r"minValue\s*=\s*cfg::(\w+)\s*;", case_body)
        max_match = re.search(r"maxValue\s*=\s*cfg::(\w+)\s*;", case_body)
        accent_match = re.search(r"accentColor\s*=\s*rgb565\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*;", case_body)

        metric_key = lower_camel(name)
        min_value = constants.get(min_match.group(1), 0) if min_match else 0
        max_value = constants.get(max_match.group(1), 1) if max_match else 1
        metrics[metric_key] = {
            "title": cpp_string(title_match.group(1)) if title_match else name,
            "unit": cpp_string(unit_match.group(1)) if unit_match else "",
            "min": min_value,
            "max": max_value,
            "decimals": 0 if metric_key == "distance" else 1,
            "accent": rgb_to_hex(*accent_match.groups()) if accent_match else "#ffffff",
        }
    return metrics


def parse_copy(ui_renderer_cpp: str) -> dict[str, str]:
    copy = {
        "brand": "Capture Healing",
        "logoTitleLine1": "Capture",
        "logoTitleLine2": "Healing",
        "logoSubtitleLine1": "Cold Atmospheric",
        "logoSubtitleLine2": "Plasma Research",
        "trendsHeading": "Temperature + Humidity + Distance",
        "loggingHeading": "SD Logging Status",
        "footerOnline": "Tap left for previous | right for next",
    }

    header_match = re.search(r'drawString\("([^"]+)",\s*margin\s*\+\s*10,\s*margin\s*\+\s*8\)', ui_renderer_cpp)
    if header_match:
        copy["brand"] = cpp_string(header_match.group(1))

    logo_strings = re.findall(
        r'tft_\.drawString\("((?:\\.|[^"])*)"',
        extract_braced_block(ui_renderer_cpp, "void UiRenderer::drawLogoPage_"),
    )
    if len(logo_strings) >= 4:
        copy["logoTitleLine1"] = cpp_string(logo_strings[0])
        copy["logoTitleLine2"] = cpp_string(logo_strings[1])
        copy["logoSubtitleLine1"] = cpp_string(logo_strings[2])
        copy["logoSubtitleLine2"] = cpp_string(logo_strings[3])

    trends_strings = re.findall(
        r'tft_\.drawString\("((?:\\.|[^"])*)"',
        extract_braced_block(ui_renderer_cpp, "void UiRenderer::drawTrendsFrame_"),
    )
    if trends_strings:
        copy["trendsHeading"] = cpp_string(trends_strings[0])

    logging_strings = re.findall(
        r'tft_\.drawString\("((?:\\.|[^"])*)"',
        extract_braced_block(ui_renderer_cpp, "void UiRenderer::drawLoggingFrame_"),
    )
    if logging_strings:
        copy["loggingHeading"] = cpp_string(logging_strings[0])

    footer_match = re.search(r'snprintf\(line,\s*sizeof\(line\),\s*"([^"]+)"\);', ui_renderer_cpp)
    if footer_match:
        copy["footerOnline"] = cpp_string(footer_match.group(1))

    return copy


def build_config() -> dict[str, object]:
    ui_renderer_h = read(UI_RENDERER_H)
    ui_renderer_cpp = read(UI_RENDERER_CPP)
    config_h = read(CONFIG_H)

    constants = parse_config_constants(config_h)
    pages = parse_page_enum(ui_renderer_h)
    titles = parse_page_text(ui_renderer_cpp, "pageTitle")
    subtitles = parse_page_text(ui_renderer_cpp, "pageSubtitle")
    for page in pages:
        enum_name = str(page["enumName"])
        page["title"] = titles.get(enum_name, enum_name)
        page["subtitle"] = subtitles.get(enum_name, "")

    page_count = parse_page_count(ui_renderer_cpp, len(pages))
    if page_count != len(pages):
        raise ValueError(f"kPageCount is {page_count}, but PageKind has {len(pages)} entries")

    return {
        "schemaVersion": 1,
        "sourceFiles": [
            "include/UiRenderer.h",
            "src/UiRenderer.cpp",
            "include/Config.h",
        ],
        "pageCount": page_count,
        "pages": pages,
        "palette": parse_palette(ui_renderer_cpp),
        "metrics": parse_metric_meta(ui_renderer_cpp, constants),
        "copy": parse_copy(ui_renderer_cpp),
    }


def write_output(config: dict[str, object]) -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    rendered = (
        "// Generated by scripts/sync-ui-tester.py; do not edit by hand.\n"
        "window.FIRMWARE_UI = "
        + json.dumps(config, indent=2, sort_keys=True)
        + ";\n"
    )
    if OUTPUT.exists() and OUTPUT.read_text(encoding="utf-8") == rendered:
        return
    OUTPUT.write_text(rendered, encoding="utf-8")


def main() -> None:
    config = build_config()
    write_output(config)
    print(f"Synced UI tester metadata: {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
