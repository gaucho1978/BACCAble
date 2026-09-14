#!/usr/bin/env python3
"""Regression test for BACCAble Setup/Actions menu labels.

Run from repository root:
    python3 tests/test_menu_labels.py
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SETUP = ROOT / "firmware/baccable/settings/setup_entries.c"
MENU = ROOT / "firmware/baccable/features/menu.c"

MAX_LABEL = 16  # "> " + 16 chars = 18-char IPC field.

EXPECTED_SETUP = (
    "ESC/TC control",
    "Dyno action",
    "Brake action",
    "AWD disable",
    "BCM DTC clear",
    "BCM fault read",
    "Virtual HAS",
    "QV exhaust",
)

EXPECTED_ACTIONS = (
    "QV exhaust req",
    "Press HAS button",
    "Toggle ESC/TC",
    "Toggle Dyno mode",
    "Brake override",
    "Disable launch",
    "AWD off request",
    "Read BCM faults",
    "Clear BCM DTCs",
    "Clear time data",
    "Peak hold",
    "IBS SOC override",
)

RETIRED_SETUP = (
    "Allow ESC/TC",
    "Allow Dyno",
    "Allow brake",
    "Allow 4WD",
    "Allow clear",
    "Allow read",
    "Allow HAS",
    "Allow exhaust",
)


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    setup_text = SETUP.read_text(encoding="utf-8")
    menu_text = MENU.read_text(encoding="utf-8")

    for label in EXPECTED_SETUP + EXPECTED_ACTIONS:
        if len(label) > MAX_LABEL:
            fail(f"{label!r}: {len(label)} chars, maximum is {MAX_LABEL}")

    for label in EXPECTED_SETUP:
        if f'"{label}"' not in setup_text:
            fail(f"missing Setup label: {label!r}")

    for label in RETIRED_SETUP:
        if f'"{label}"' in setup_text:
            fail(f"old Setup label still present: {label!r}")

    match = re.search(
        r"static const ActionEntry actions\[\]\s*=\s*\{(?P<body>.*?)\n\};",
        menu_text,
        re.S,
    )
    if not match:
        fail("ActionEntry actions[] not found")

    body = match.group("body")
    action_labels = re.findall(
        r'\{ACTION_[A-Z_]+,\s*\d+,\s*"([^"]+)"\s*,\s*UI_ENTRY_[A-Z_]+\}',
        body,
    )

    if len(action_labels) != len(EXPECTED_ACTIONS):
        fail(
            f"expected {len(EXPECTED_ACTIONS)} actions, "
            f"found {len(action_labels)}: {action_labels}"
        )

    if tuple(action_labels) != EXPECTED_ACTIONS:
        fail(
            "Action labels/order differ.\n"
            f"Expected: {EXPECTED_ACTIONS}\n"
            f"Found:    {tuple(action_labels)}"
        )

    for label in action_labels:
        rendered = "> " + label
        if len(rendered) > 18:
            fail(f"18-char overflow: {rendered!r} ({len(rendered)} chars)")

    print("OK: Setup labels present and retired 'Allow ...' labels removed.")
    print("OK: all Action labels are <= 16 characters.")
    print("OK: every '> ' + Action label fits in exactly 18 characters or less.")


if __name__ == "__main__":
    main()
