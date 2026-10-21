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
from build_gba_art import CARDS, DIGITS, FRAME_COLOR, HEROES, NO_COST, SHARED_OWNERS, TYPE_ICONS, CardArt

CARD_HEADER = build_gba_art.ROOT / "include" / "engine" / "card.h"
CARD_META_HEADER = build_gba_art.ROOT / "include" / "engine" / "card_meta.h"
CARD_SOURCE = build_gba_art.ROOT / "src" / "engine" / "card.cpp"


def engine_card_ids() -> list[str]:
    """Enumerator names of `enum class CardId`, in declaration order (COUNT excluded)."""
    source = CARD_HEADER.read_text()
    body = re.search(r"enum class CardId[^{]*\{(.*?)\n\};", source, re.S)
    assert body, "enum class CardId not found in card.h"
    names = [entry.split("=")[0].strip() for entry in re.sub(r"//[^\n]*", "", body.group(1)).split(",")]
    names = [name for name in names if name]
    assert names[-1] == "COUNT"
    return names[:-1]


def engine_card_meta() -> dict[str, tuple[str, str]]:
    """(rarity, pool) of every card of card_meta.h, by lower-case CardId name."""
    rows = re.findall(r"\{ CardRarity::(\w+), CardPool::(\w+) \}, // (\w+)", CARD_META_HEADER.read_text())
    return {name.lower(): (rarity, pool) for rarity, pool, name in rows}


STRING = r'"(?:[^"\\]|\\.)*"'
CARD_ROW = re.compile(
    r"\{\s*CardId::(\w+),\s*" + STRING + r",\s*(\w+),\s*CardType::(\w+),\s*TargetType::\w+,\s*\w+,\s*\w+,\s*"
    + STRING + r",\s*" + STRING + r",\s*(\w+),\s*\w+,\s*\w+,\s*" + STRING + r"\s*(.*?)\}\s*,?\s*$", re.M)


def engine_card_costs() -> dict[str, dict[str, object]]:
    """Energy costs, Star costs and type of every CardDef row of card.cpp, in the CardArt conventions:
    -1 for X, NO_COST for an Unplayable card."""

    def value(token: str) -> int:
        return -1 if token == "COST_X" else int(token)

    cards: dict[str, dict[str, object]] = {}
    for name, cost, kind, upgraded_cost, rest in CARD_ROW.findall(CARD_SOURCE.read_text()):
        extra = [token.strip() for token in rest.split(",") if token.strip()]
        extra += ["0"] * (4 - len(extra))
        cards[name.lower()] = {
            "costs": (NO_COST if "UNPLAYABLE" in extra[0] else value(cost),
                      NO_COST if "UNPLAYABLE" in extra[1] else value(upgraded_cost)),
            "stars": (value(extra[2]), value(extra[3])),
            "kind": "status" if kind in ("STATUS", "CURSE", "QUEST") else kind.lower(),
        }
    return cards


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


def test_costs_stars_and_kinds_are_the_ones_of_the_engine() -> None:
    engine = engine_card_costs()
    assert sorted(engine) == sorted(card.enum_name for card in CARDS)
    wrong = [(card.enum_name, field, getattr(card, field), engine[card.enum_name][field])
             for card in CARDS[1:] for field in ("costs", "stars", "kind")
             if getattr(card, field) != engine[card.enum_name][field]]
    assert not wrong, f"(card, field, art, engine): {wrong}"


def test_volley_costs_x() -> None:
    volley = next(card for card in CARDS if card.enum_name == "volley")
    assert volley.costs == (-1, -1)


def test_colorless_cards_are_the_ones_of_the_colorless_pool() -> None:
    pool = sorted(name for name, (_, pool) in engine_card_meta().items() if pool == "COLORLESS")
    assert pool and sorted(card.enum_name for card in CARDS if card.owner == "colorless") == pool
    assert not [card.enum_name for card in CARDS if card.folder == "colorless"]  # the owner names the folder
    assert FRAME_COLOR["colorless"] == FRAME_COLOR["status"] == "grey"
    assert build_gba_art.HERO_RGB["colorless"] == build_gba_art.HERO_RGB["status"]


def test_ancient_cards_are_the_event_cards_of_ancient_rarity() -> None:
    ancient = sorted(name for name, entry in engine_card_meta().items() if entry == ("ANCIENT", "EVENT"))
    assert ancient and sorted(card.enum_name for card in CARDS if card.owner == "ancient") == ancient
    assert all(card.folder == "event" for card in CARDS if card.owner == "ancient")


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
    assert {card.owner for card in sheet} <= {"hero", hero, *SHARED_OWNERS}
    assert len(sheet) == len({card.enum_name for card in sheet})


def test_on_cartridge_keeps_shared_and_own_cards_only() -> None:
    own = CardArt("a", "silent", "a", "attack", (1, 1))
    assert build_gba_art.on_cartridge(own, "silent") and not build_gba_art.on_cartridge(own, "defect")
    assert build_gba_art.on_cartridge(CardArt("b", "hero", "b", "skill", (1, 1)), "defect")
    assert build_gba_art.on_cartridge(CardArt("c", "status", "c", "status", (NO_COST, NO_COST)), "regent")
    for owner in SHARED_OWNERS:
        assert all(build_gba_art.on_cartridge(CardArt("d", owner, "d", "skill", (1, 1)), hero) for hero in HEROES)


def test_shared_cards_have_a_single_sprite_item_on_every_cartridge() -> None:
    header = build_gba_art.CARD_ART_HEADER.read_text()
    for card in CARDS:
        if card.owner in SHARED_OWNERS:
            assert header.count(f'#include "bn_sprite_items_card_{card.enum_name}.h"') == len(HEROES), card.enum_name
            assert f"card_{card.enum_name}_ironclad" not in header


def test_every_card_sprite_of_the_repository_is_used_and_fits_one_obj_palette() -> None:
    wanted = {f"card_{card.enum_name}_{hero}" if card.owner == "hero" else f"card_{card.enum_name}"
              for card in CARDS[1:] for hero in HEROES}
    found = {path.stem for path in build_gba_art.OUT.glob("card_*.bmp")}
    assert found == wanted, f"unused: {sorted(found - wanted)}, missing: {sorted(wanted - found)}"
    for card in CARDS:
        if card.owner in ("colorless", "ancient"):
            with Image.open(build_gba_art.OUT / f"card_{card.enum_name}.bmp") as bmp:
                assert bmp.size == (64, 64) and int(np.asarray(bmp).max()) <= 15, card.enum_name


@pytest.mark.parametrize("hero", HEROES)
def test_a_mini_card_sheet_is_one_palette_with_white_and_two_frames_per_card(hero: str) -> None:
    with Image.open(build_gba_art.OUT / f"hand_cards_{hero}.bmp") as bmp:
        assert bmp.size == (32, 32 * 2 * len(build_gba_art.sheet_cards(hero)))
        assert int(np.asarray(bmp).max()) <= 15
        palette = bmp.getpalette()[:48]
    assert build_gba_art.SHEET_WHITE in [tuple(palette[i:i + 3]) for i in range(0, 48, 3)]


# --- Hero poses -------------------------------------------------------------------------------


@pytest.mark.parametrize("hero", HEROES)
def test_a_hero_sheet_is_one_palette_with_an_idle_an_attack_and_a_hurt_pose(hero: str) -> None:
    with Image.open(build_gba_art.OUT / f"hero_{hero}.bmp") as bmp:
        assert bmp.size == (64, 64 * len(build_gba_art.HERO_POSES))
        sheet = np.asarray(bmp)
        palette = bmp.getpalette()[:48]
    assert build_gba_art.HERO_POSES == ("idle", "attack", "hurt")  # frame order of include/engine/hero_pose.h
    assert int(sheet.max()) <= 15
    colors = [tuple(palette[i:i + 3]) for i in range(0, 48, 3)]
    for accent in (build_gba_art.FX_WHITE, *build_gba_art.HERO_FX[hero]):
        assert build_gba_art.to_gba(accent) in colors
    idle, attack, hurt = (sheet[i * 64:(i + 1) * 64] for i in range(3))
    assert (idle != attack).sum() > 100 and (idle != hurt).sum() > 100


def test_a_hero_sheet_rebuilds_to_the_committed_one(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    for hero in HEROES:
        (tmp_path / f"player_{hero}.bmp").write_bytes((build_gba_art.OUT / f"player_{hero}.bmp").read_bytes())
    monkeypatch.setattr(build_gba_art, "OUT", tmp_path)
    build_gba_art.build_heroes()
    for hero in HEROES:
        with Image.open(tmp_path / f"hero_{hero}.bmp") as built, \
                Image.open(build_gba_art.ROOT / "graphics" / f"hero_{hero}.bmp") as committed:
            assert np.array_equal(np.asarray(built), np.asarray(committed)), hero


def test_lean_keeps_the_feet_and_moves_the_head() -> None:
    body = np.zeros((8, 8), dtype=np.uint8)
    body[2:8, 3] = 5
    leaned = build_gba_art.lean(body, 2)
    assert leaned[7, 3] == 5 and leaned[2, 5] == 5 and leaned[2, 3] == 0
    assert build_gba_art.lean(body, 0, drop=1)[3, 3] == 5


# --- Frames -----------------------------------------------------------------------------------


@pytest.mark.parametrize("kind", ["attack", "skill", "power"])
def test_the_grey_frame_keeps_the_shape_of_its_kind_without_its_colour(kind: str) -> None:
    grey = build_gba_art.load_frame(kind, "grey")
    red = build_gba_art.load_frame(kind, "red")
    assert grey.getchannel("A").tobytes() == red.getchannel("A").tobytes()
    opaque = np.asarray(grey.getchannel("A")) > 128
    # The border and its shaded parts lose their colour; the dark text panel was hardly coloured to begin with.
    saturation = np.asarray(grey.convert("RGB").convert("HSV"))[..., 1][opaque].astype(int)
    assert saturation.max() <= 96
    assert saturation.mean() < np.asarray(red.convert("RGB").convert("HSV"))[..., 1][opaque].mean() / 2


def test_the_grey_frame_of_a_status_card_is_the_skill_one() -> None:
    assert build_gba_art.load_frame("status", "grey").tobytes() == build_gba_art.load_frame("skill", "grey").tobytes()


def test_the_energy_gem_of_a_shared_card_has_no_colour() -> None:
    for owner in SHARED_OWNERS:
        gem = np.asarray(build_gba_art.energy_gem(owner, 16).convert("RGBA")).astype(int)
        opaque = gem[..., 3] > 128
        assert (gem[..., :3].max(axis=2) - gem[..., :3].min(axis=2))[opaque].max() <= 2, owner


@pytest.mark.parametrize("name", ["maul", "wish"])
def test_an_ancient_card_is_full_art_inside_a_closed_white_line(name: str) -> None:
    card = next(card for card in CARDS if card.enum_name == name)
    body = np.asarray(build_gba_art.compose_card(card, "ironclad", (46, 62)))
    assert body.shape == (62, 46, 4)
    line = np.all(body[..., :3] == np.array(build_gba_art.ANCIENT_LINE), axis=2) & (body[..., 3] == 255)
    # The line runs along the four sides: found in every row and every column away from the rounded corners.
    assert line[4:-4, :3].any(axis=1).all() and line[4:-4, -3:].any(axis=1).all()
    assert line[:3, 4:-4].any(axis=0).all() and line[-3:, 4:-4].any(axis=0).all()
    assert (body[4:-4, 4:-4, 3] == 255).all()  # the art fills the card: no portrait window


def test_an_ancient_mini_card_has_the_white_line_and_no_hero_colour() -> None:
    card = next(card for card in CARDS if card.enum_name == "maul")
    mini = build_gba_art.compose_mini(card, "silent")
    assert mini.size == (23, 32)
    assert mini.getpixel((1, 10))[:3] == build_gba_art.ANCIENT_LINE
    assert mini.getpixel((21, 10))[:3] == build_gba_art.ANCIENT_LINE
    assert mini.getpixel((3, 24))[:3] == build_gba_art.ANCIENT_PANEL
    assert mini.tobytes() == build_gba_art.compose_mini(card, "defect").tobytes()


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
