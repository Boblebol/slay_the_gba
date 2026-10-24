"""Tests of the English texts of the website (site_en.py): names from class names and filled templates."""

from __future__ import annotations

import pytest

from site_en import describe, english_name, id_name, markup


@pytest.mark.parametrize(("class_name", "strip", "name"), [
    ("BeetleJuice", False, "Beetle Juice"), ("StrikeIronclad", True, "Strike"), ("StrikeIronclad", False, "Strike Ironclad"),
    ("ArtOfWar", False, "Art of War"), ("PaelsTooth", False, "Pael's Tooth"), ("Ironclad", True, "Ironclad"),
])
def test_english_name_splits_class_names(class_name: str, strip: bool, name: str) -> None:
    assert english_name(class_name, strip) == name


def test_id_name_keeps_small_words_lower_case_after_the_first() -> None:
    assert id_name("BATTLEWORN_DUMMY") == "Battleworn Dummy" and id_name("THE_LOST_AND_FOUND") == "The Lost and Found"


def test_describe_fills_values_plurals_and_energy_icons() -> None:
    entry = {"description": "Gain {Energy:energyIcons()}. Draw {Cards} {Cards:plural:card|cards}.",
             "vars": {"Energy": 2, "Cards": 1}}
    assert describe(entry) == "Gain 2 ●. Draw 1 card."


def test_describe_applies_upgrades_and_keywords() -> None:
    entry = {"description": "Deal {Damage:diff()} damage.", "vars": {"damage": 6}, "upgrade": {"damage": "+3"},
             "keywords": ["Innate", "Exhaust"]}
    assert describe(entry, upgraded=True) == "[gold]Innate[/gold].\nDeal 9 damage.\n[gold]Exhaust[/gold]."


def test_describe_names_enchantments_and_takes_the_generic_branch_of_run_values() -> None:
    claw = {"description": "Enchant with [purple]{EnchantmentName}[/purple].", "enchantmentVars": {"EnchantmentName": "GOOPY"}}
    assert describe(claw) == "Enchant with [purple]Goopy[/purple]."
    tome = {"description": "Obtain {AncientCard.StringValue:cond:[gold]{}[/gold]|an [gold]Ancient Card[/gold]}."}
    assert describe(tome) == "Obtain an [gold]Ancient Card[/gold]."


def test_describe_of_a_keyword_only_curse_lists_its_keywords() -> None:
    curse = {"description": "", "keywords": ["Unplayable", "Ethereal"]}
    assert describe(curse, french="<g>Injouable</g>.\n<g>Éthéré</g>.") == "[gold]Unplayable[/gold].\n[gold]Ethereal[/gold]."
    assert describe({"description": "", "keywords": ["Unplayable"]}, french="Injouable.\nPerdez 1 PV.") is None


def test_describe_gives_up_on_values_the_data_does_not_have() -> None:
    assert describe({"description": "Deal {Damage} damage.", "vars": {"block": 3}}) is None


def test_markup_turns_game_tags_into_html() -> None:
    assert markup("[gold]Block[/gold] [blue]3[/blue] 1 ●\nx") == '<b class="kw">Block</b> 3 1 <span class="nrg">●</span><br>x'
