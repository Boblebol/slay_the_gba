"""Checks that every potion fits the belt of the combat screen, measured with the widths of the GBA font.

The belt (src/gba/scene_combat.cpp) shows the name of the potion on one line, next to its place in the belt, and
its description wrapped on at most BELT_DESCRIPTION_LINES lines of BELT_TEXT_WIDTH pixels. Names and descriptions
come from src/engine/potion.cpp.

Run from the repository root:  uv run --with pytest pytest scripts/potion_belt_text_test.py
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

from card_text_test import FONT, text_width

ROOT = Path(__file__).resolve().parent.parent
POTION_SOURCE = ROOT / "src" / "engine" / "potion.cpp"

BELT_TEXT_WIDTH = 224
BELT_DESCRIPTION_LINES = 3
BELT_PLACE = "◄ 3/3 ►"  # widest place shown on the right of the name
LINE_BYTES = 63  # a TextLine is a bn::string<64>

POTION_ROW = re.compile(r'\{ "(\w+)", "([^"]*)", "(\w+)", "(\w+)", "([^"]*)" \}')


def sanitize(text: str) -> str:
    """Non-breaking spaces are drawn as spaces, as gba::sanitize does."""
    return text.replace(" ", " ").replace(" ", " ")


def wrap(text: str, max_width: int) -> list[str]:
    """Greedy word wrap on spaces, as gba::wrap_text does."""
    lines: list[str] = []
    current = ""
    for word in text.split(" "):
        candidate = f"{current} {word}" if current else word
        if current and text_width(sanitize(candidate), FONT) > max_width:
            lines.append(current)
            current = word
        else:
            current = candidate
    if current:
        lines.append(current)
    return lines


POTIONS = [(m.group(1), m.group(2), m.group(5)) for m in POTION_ROW.finditer(POTION_SOURCE.read_text("utf-8"))]


def test_reader_finds_the_potions() -> None:
    assert len(POTIONS) >= 60
    assert any(potion_id == "FIRE_POTION" for potion_id, _, _ in POTIONS)


@pytest.mark.parametrize(("potion_id", "name", "description"), POTIONS, ids=[p[0] for p in POTIONS])
def test_potion_fits_the_belt(potion_id: str, name: str, description: str) -> None:
    name_width = text_width(sanitize(name), FONT)
    assert name_width + text_width(BELT_PLACE, FONT) + 4 <= BELT_TEXT_WIDTH, f"{potion_id}: name too wide"

    lines = wrap(description, BELT_TEXT_WIDTH)
    assert len(lines) <= BELT_DESCRIPTION_LINES, f"{potion_id}: {len(lines)} lines: {lines}"
    for line in lines:
        assert text_width(sanitize(line), FONT) <= BELT_TEXT_WIDTH, f"{potion_id}: line too wide: {line!r}"
        assert len(line.encode("utf-8")) <= LINE_BYTES - 4, f"{potion_id}: line too long for a TextLine: {line!r}"
