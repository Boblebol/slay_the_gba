"""Tests of the GBA art builder: the card table against the engine and the dump, and its pure helpers.

Nothing is written to the repository: every output goes to a tmp_path.
Run from the repository root:  uv run --with pytest --with pillow --with numpy pytest scripts/build_gba_art_test.py
"""

from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import build_gba_art
from build_gba_art import CARDS, DIGITS, FRAME_COLOR, HEROES, NO_COST, TYPE_ICONS, CardArt

CARD_HEADER = build_gba_art.ROOT / "include" / "engine" / "card.h"


def engine_card_ids() -> list[str]:
    """Enumerator names of `enum class CardId`, in declaration order (COUNT excluded)."""
    source = CARD_HEADER.read_text()
    body = re.search(r"enum class CardId[^{]*\{(.*?)\n\};", source, re.S)
    assert body, "enum class CardId not found in card.h"
    names = [entry.split("=")[0].strip() for entry in re.sub(r"//[^\n]*", "", body.group(1)).split(",")]
    names = [name for name in names if name]
    assert names[-1] == "COUNT"
    return names[:-1]


def card_art_files(card: CardArt) -> list[Path]:
    """Every art file a card needs: one per cartridge for the hero's own cards, a single one otherwise."""
    if not card.art:
        return []
    return [build_gba_art.card_art_path(card, hero) for hero in (HEROES if card.owner == "hero" else HEROES[:1])]


# --- The card table against the engine --------------------------------------------------------


def test_cards_follow_the_order_of_the_engine_card_id_enum() -> None:
    assert [card.enum_name for card in CARDS] == [name.lower() for name in engine_card_ids()]


def test_the_first_card_is_the_blank_one_and_names_are_unique() -> None:
    assert CARDS[0].enum_name == "none" and not CARDS[0].art
    assert not [name for name, count in Counter(card.enum_name for card in CARDS).items() if count > 1]


@pytest.mark.parametrize("card", [card for card in CARDS if card.art], ids=lambda card: card.enum_name)
def test_every_card_art_points_to_an_image_that_exists(card: CardArt) -> None:
    for path in card_art_files(card):
        assert path.is_file(), f"missing {path.relative_to(build_gba_art.ROOT)}"
        assert path.suffix == ".webp" and "{" not in path.name


def test_only_the_blank_card_has_no_art() -> None:
    assert [card.enum_name for card in CARDS if not card.art] == ["none"]


@pytest.mark.parametrize("card", CARDS, ids=lambda card: card.enum_name)
def test_every_card_has_a_known_owner_kind_and_frame(card: CardArt) -> None:
    assert card.owner == "hero" or card.owner in FRAME_COLOR
    assert card.kind in TYPE_ICONS


def test_the_hero_art_template_is_only_used_by_cards_every_hero_owns() -> None:
    for card in CARDS:
        assert ("{hero}" in card.art) <= (card.owner == "hero")


# --- Digits -----------------------------------------------------------------------------------


@pytest.mark.parametrize("digit", sorted(DIGITS))
def test_every_digit_is_three_columns_by_five_rows(digit: str) -> None:
    rows = DIGITS[digit]
    assert len(rows) == 5 and all(len(row) == 3 for row in rows)
    assert all(set(row) <= {"#", "."} for row in rows)


def test_digits_are_the_ten_numbers_and_x() -> None:
    assert sorted(DIGITS) == sorted("0123456789X")
    assert len({tuple(rows) for rows in DIGITS.values()}) == len(DIGITS)  # no two glyphs look the same


def test_every_cost_and_star_of_the_card_table_can_be_drawn() -> None:
    for card in CARDS:
        for value in card.costs:
            if value != NO_COST:
                assert set("X" if value < 0 else str(value)) <= set(DIGITS), card.enum_name
        for value in card.stars:
            if value:
                assert ("X" if value < 0 else str(value)) in DIGITS, card.enum_name


# --- Cartridge sheets -------------------------------------------------------------------------


@pytest.mark.parametrize("hero", HEROES)
def test_a_cartridge_sheet_starts_with_the_blank_card_and_skips_other_heroes(hero: str) -> None:
    sheet = build_gba_art.sheet_cards(hero)
    assert sheet[0].enum_name == "none"
    assert {card.owner for card in sheet} <= {"hero", "status", "curse", hero}
    assert len(sheet) == len({card.enum_name for card in sheet})


def test_on_cartridge_keeps_shared_and_own_cards_only() -> None:
    own = CardArt("a", "silent", "a", "attack", (1, 1))
    assert build_gba_art.on_cartridge(own, "silent") and not build_gba_art.on_cartridge(own, "defect")
    assert build_gba_art.on_cartridge(CardArt("b", "hero", "b", "skill", (1, 1)), "defect")
    assert build_gba_art.on_cartridge(CardArt("c", "status", "c", "status", (NO_COST, NO_COST)), "regent")


def test_card_art_path_substitutes_the_hero_and_honours_the_token_folder() -> None:
    images = build_gba_art.BIBLE / "cards"
    assert build_gba_art.card_art_path(CardArt("s", "hero", "strike_{hero}", "attack", (1, 1)), "silent") == (
        images / "silent" / "strike_silent.webp")
    assert build_gba_art.card_art_path(CardArt("s", "silent", "shiv", "attack", (0, 0), folder="token"), "defect") == (
        images / "token" / "shiv.webp")
    assert build_gba_art.card_art_path(CardArt("b", "ironclad", "bash", "attack", (2, 2)), "defect") == (
        images / "ironclad" / "bash.webp")


# --- Pure helpers -----------------------------------------------------------------------------


def test_to_gba_keeps_the_five_high_bits_of_each_channel() -> None:
    assert build_gba_art.to_gba((255, 0, 7)) == (248, 0, 0)
    assert build_gba_art.to_gba((8, 100, 200)) == (8, 96, 200)


def test_lerp_truncates_towards_the_start_colour() -> None:
    assert build_gba_art.lerp((0, 0, 0), (100, 50, 10), 0.5) == (50, 25, 5)
    assert build_gba_art.lerp((10, 10, 10), (20, 20, 20), 0.0) == (10, 10, 10)
    assert build_gba_art.lerp((10, 10, 10), (20, 20, 20), 1.0) == (20, 20, 20)
    assert build_gba_art.lerp((0, 0, 0), (255, 255, 255), 0.25) == (63, 63, 63)


def test_pixel_art_paints_only_the_mapped_characters() -> None:
    img = build_gba_art.pixel_art(["#.", ".o"], {"#": (255, 0, 0), "o": (0, 0, 255)})
    assert img.size == (2, 2)
    assert img.getpixel((0, 0)) == (255, 0, 0, 255) and img.getpixel((1, 1)) == (0, 0, 255, 255)
    assert img.getpixel((1, 0))[3] == 0


def test_quantize_opaque_of_no_pixels_is_a_single_black_entry() -> None:
    assert build_gba_art.quantize_opaque(np.zeros((0, 3), dtype=np.uint8), 15) == [(0, 0, 0)]


def test_nearest_indices_picks_the_closest_palette_colour() -> None:
    rgb = np.array([[[250, 5, 5], [5, 5, 250], [0, 0, 0]]], dtype=np.uint8)
    palette = [(255, 0, 0), (0, 0, 255), (0, 0, 0)]
    assert build_gba_art.nearest_indices(rgb, palette).tolist() == [[0, 1, 2]]


def test_rgba_to_sprite_reserves_index_zero_for_transparency() -> None:
    img = Image.new("RGBA", (2, 1), (255, 0, 0, 255))
    img.putpixel((1, 0), (0, 255, 0, 0))
    indices, palette = build_gba_art.rgba_to_sprite(img, 4)
    assert indices.tolist() == [[1, 0]]
    assert palette[0] == build_gba_art.TRANSPARENT and palette[1] == (255, 0, 0)


def test_fit_art_fits_the_box_and_bottom_aligns() -> None:
    img = Image.new("RGBA", (40, 20), (255, 255, 255, 255))
    out = build_gba_art.fit_art(img, (10, 10))
    assert out.size == (10, 10)
    assert out.getchannel("A").getbbox() == (0, 5, 10, 10)  # a 10x5 picture sitting on the bottom edge


def test_write_json_and_indexed_bmp_go_where_they_are_told(tmp_path: Path) -> None:
    build_gba_art.write_json(tmp_path / "x.json", {"a": 1})
    assert json.loads((tmp_path / "x.json").read_text()) == {"a": 1}
    build_gba_art.write_indexed_bmp(tmp_path / "x.bmp", np.array([[0, 1]]), [(0, 0, 0), (255, 255, 255)])
    with Image.open(tmp_path / "x.bmp") as bmp:
        assert bmp.mode == "P" and bmp.size == (2, 1)
        assert [bmp.getpixel((x, 0)) for x in range(2)] == [0, 1]
