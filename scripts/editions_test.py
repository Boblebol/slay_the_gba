"""Tests of the cartridge editions: every hero is fully described and no key is used twice.

Run from the repository root:  uv run --with pytest pytest scripts/editions_test.py
"""

from __future__ import annotations

import re
from collections import Counter

import pytest

import editions
from editions import CONTROLS, DIFFICULTY, HEROES, Hero, Palette

HEX_COLOR = re.compile(r"#[0-9a-f]{6}")
ROM_NAME = re.compile(r"slay_[a-z]+\.gba")


def duplicates(values: list[str]) -> list[str]:
    return [value for value, count in Counter(values).items() if count > 1]


def test_there_are_five_editions() -> None:
    assert [hero.slug for hero in HEROES] == ["ironclad", "silent", "defect", "regent", "necrobinder"]


@pytest.mark.parametrize("hero", HEROES, ids=lambda hero: hero.slug)
def test_every_edition_has_a_hero_a_rom_and_colours(hero: Hero) -> None:
    assert hero.name and hero.english and hero.edition and hero.relic_id
    assert ROM_NAME.fullmatch(hero.rom) and hero.rom == f"slay_{hero.slug}.gba"
    assert hero.ref.isupper() and 0 < len(hero.ref) <= 12  # GBA header title: 12 characters at most
    assert hero.color_name
    assert HEX_COLOR.fullmatch(hero.color) and HEX_COLOR.fullmatch(hero.accent)
    assert hero.palettes and all(HEX_COLOR.fullmatch(palette.hex) for palette in hero.palettes)


@pytest.mark.parametrize("hero", HEROES, ids=lambda hero: hero.slug)
def test_every_hero_is_playable(hero: Hero) -> None:
    assert hero.hp > 0
    assert hero.difficulty in DIFFICULTY
    assert hero.starter and all(copies > 0 for _, copies in hero.starter)
    assert hero.key_cards and hero.tips


@pytest.mark.parametrize("hero", HEROES, ids=lambda hero: hero.slug)
def test_key_cards_are_distinct(hero: Hero) -> None:
    assert not duplicates(list(hero.key_cards))


@pytest.mark.parametrize("hero", HEROES, ids=lambda hero: hero.slug)
def test_starter_deck_has_no_card_listed_twice(hero: Hero) -> None:
    assert not duplicates([card_id for card_id, _ in hero.starter])


@pytest.mark.parametrize("hero", HEROES, ids=lambda hero: hero.slug)
def test_palette_names_and_colours_are_distinct_within_an_edition(hero: Hero) -> None:
    assert not duplicates([palette.name for palette in hero.palettes])
    assert not duplicates([palette.hex for palette in hero.palettes])


@pytest.mark.parametrize("field", ["slug", "pool", "name", "english", "edition", "ref", "rom", "color_name", "color",
                                   "relic_id", "zone", "tome", "screenshot"])
def test_no_key_is_shared_between_editions(field: str) -> None:
    assert not duplicates([getattr(hero, field) for hero in HEROES])


def test_no_hero_is_declared_twice() -> None:
    assert len(HEROES) == len(set(HEROES))


def test_palette_hex_unpacks_bgr555_into_rgb() -> None:
    # BGR555 stores red in the low 5 bits, green in the next 5 and blue in the top 5.
    assert Palette("p", "d", 0x0000).hex == "#000000"
    assert Palette("p", "d", 0x001F).hex == "#f80000"
    assert Palette("p", "d", 0x03E0).hex == "#00f800"
    assert Palette("p", "d", 0x7C00).hex == "#0000f8"
    assert Palette("p", "d", 0x1084).hex == "#202020"


def test_screenshot_name_of_the_default_edition_has_no_suffix() -> None:
    by_slug = {hero.slug: hero for hero in HEROES}
    assert by_slug["ironclad"].screenshot == "combat"
    assert by_slug["silent"].screenshot == "combat_silent"


def test_french_labels_have_no_empty_values() -> None:
    for table in (editions.TYPE_FR, editions.RARITY_FR, editions.ENCOUNTER_FR, DIFFICULTY):
        assert table and all(label for label in table.values())


def test_controls_are_screen_button_action_triples() -> None:
    assert CONTROLS and all(len(row) == 3 and all(row) for row in CONTROLS)
    assert not duplicates([f"{screen}|{button}" for screen, button, _ in CONTROLS])
