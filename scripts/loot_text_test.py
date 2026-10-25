"""Checks that relics and potions fit the reward screen and the merchant, measured with the widths of the GBA font.

Both screens (src/gba/scene_rewards.cpp, src/gba/scene_shop.cpp) show a relic or a potion as one row with its
name, and its text wrapped on at most TEXT_LINES lines of TEXT_WIDTH pixels. Relic texts come from
src/gba/loot_text.cpp, names from src/engine/relic.cpp, potions from src/engine/potion.cpp. Every relic a run can
obtain (active_relics in src/engine/run.cpp) must have a text, or the screen would offer it without saying what
it does.

Run from the repository root:  uv run --with pytest pytest scripts/loot_text_test.py
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

from card_text_test import FONT, text_width
from potion_belt_text_test import POTIONS, sanitize, wrap

ROOT = Path(__file__).resolve().parent.parent
LOOT_TEXT_SOURCE = ROOT / "src" / "gba" / "loot_text.cpp"
RELIC_SOURCE = ROOT / "src" / "engine" / "relic.cpp"
RUN_SOURCE = ROOT / "src" / "engine" / "run.cpp"
REWARDS_SCENE = ROOT / "src" / "gba" / "scene_rewards.cpp"
SHOP_SCENE = ROOT / "src" / "gba" / "scene_shop.cpp"

TEXT_WIDTH = 224
TEXT_LINES = 4
LINE_BYTES = 63  # a TextLine is a bn::string<64>
ROW_GAP = 8  # pixels kept between a name and its price

# Rows of the reward screen, and of the merchant with the widest price next to the name.
REWARD_RELIC_ROW = "► Relique : "
REWARD_POTION_ROW = "► Potion : "
SHOP_ROW = "► "
SHOP_PRICE = "315 Or"  # a Rare relic at its highest price

# Texts shown side by side on the same line: the left one starts at -116 or -112, the right one ends at 116.
SIDE_BY_SIDE = [
    (REWARDS_SCENE, "A Prendre", "START Continuer"),
    (REWARDS_SCENE, "Jeter quelle potion ?", "B Retour"),
]
# Texts alone on their line.
ALONE = [
    (SHOP_SCENE, "A Acheter   B Retour"),
    (SHOP_SCENE, "◄ ► Choisir   A Acheter   B Retour"),
    (SHOP_SCENE, "A Choisir   B Quitter"),
    (SHOP_SCENE, "A Remplacer   B Retour"),
    (SHOP_SCENE, "Jeter quelle potion ?"),
    (SHOP_SCENE, "Retire définitivement une carte du deck."),
    (LOOT_TEXT_SOURCE, "Relique du marchand"),
    (LOOT_TEXT_SOURCE, "Relique peu commune"),
]

RELIC_TEXT_ROW = re.compile(r'^    \{ "(\w+)", "([^"]*)" \},$', re.MULTILINE)
RELIC_ROW = re.compile(r'\{ "(\w+)", "([^"]*)", RelicRarity::(\w+), "(\w+)", ')

RELIC_TEXTS = dict(RELIC_TEXT_ROW.findall(LOOT_TEXT_SOURCE.read_text("utf-8")))
RELIC_NAMES = {m.group(1): m.group(2) for m in RELIC_ROW.finditer(RELIC_SOURCE.read_text("utf-8"))}


def active_relics() -> list[str]:
    source = RUN_SOURCE.read_text("utf-8")
    block = source[source.index("ACTIVE_RELICS[] = {") : source.index("return ACTIVE_RELICS;")]
    return re.findall(r'"(\w+)"', block)


ACTIVE_RELICS = active_relics()


def width(text: str) -> int:
    return text_width(sanitize(text), FONT)


def assert_wraps(owner: str, text: str) -> None:
    lines = wrap(text, TEXT_WIDTH)
    assert len(lines) <= TEXT_LINES, f"{owner}: {len(lines)} lines: {lines}"
    for line in lines:
        assert width(line) <= TEXT_WIDTH, f"{owner}: line too wide: {line!r}"
        assert len(line.encode("utf-8")) <= LINE_BYTES - 4, f"{owner}: line too long for a TextLine: {line!r}"


def assert_row_fits(owner: str, row: str, right: str = "") -> None:
    gap = ROW_GAP if right else 0
    assert width(row) + gap + width(right) <= TEXT_WIDTH, f"{owner}: row too wide: {row!r}"
    assert len(row.encode("utf-8")) <= LINE_BYTES, f"{owner}: row too long for a TextLine: {row!r}"


def test_readers_find_the_sources() -> None:
    assert len(ACTIVE_RELICS) >= 80
    assert "ANCHOR" in ACTIVE_RELICS
    assert len(RELIC_NAMES) >= 290
    assert len(POTIONS) >= 60


def test_every_obtainable_relic_has_a_text() -> None:
    missing = sorted(set(ACTIVE_RELICS) - set(RELIC_TEXTS))
    assert not missing, f"relics offered without a text in loot_text.cpp: {missing}"
    unknown = sorted(set(RELIC_TEXTS) - set(RELIC_NAMES))
    assert not unknown, f"texts for relics the engine does not know: {unknown}"


@pytest.mark.parametrize("relic_id", sorted(RELIC_TEXTS))
def test_relic_fits_both_screens(relic_id: str) -> None:
    assert RELIC_TEXTS[relic_id], f"{relic_id}: empty text"
    assert_wraps(relic_id, RELIC_TEXTS[relic_id])
    name = RELIC_NAMES[relic_id]
    assert_row_fits(relic_id, REWARD_RELIC_ROW + name)
    assert_row_fits(relic_id, SHOP_ROW + name, SHOP_PRICE)


@pytest.mark.parametrize(("potion_id", "name", "description"), POTIONS, ids=[p[0] for p in POTIONS])
def test_potion_fits_both_screens(potion_id: str, name: str, description: str) -> None:
    assert_wraps(potion_id, description)
    assert_row_fits(potion_id, REWARD_POTION_ROW + name)
    assert_row_fits(potion_id, SHOP_ROW + name, SHOP_PRICE)


@pytest.mark.parametrize(("source", "left", "right"), SIDE_BY_SIDE, ids=[pair[1] for pair in SIDE_BY_SIDE])
def test_side_by_side_texts_do_not_meet(source: Path, left: str, right: str) -> None:
    text = source.read_text("utf-8")
    assert f'"{left}"' in text and f'"{right}"' in text, "text changed in the scene: update this test"
    assert width(left) + ROW_GAP + width(right) <= TEXT_WIDTH


@pytest.mark.parametrize(("source", "line"), ALONE, ids=[pair[1] for pair in ALONE])
def test_single_texts_fit_a_line(source: Path, line: str) -> None:
    assert f'"{line}"' in source.read_text("utf-8"), "text changed in the scene: update this test"
    assert width(line) <= TEXT_WIDTH
    assert len(line.encode("utf-8")) <= LINE_BYTES
