"""Tests of the booklet generator's pure functions, on small hand-written inputs.

Nothing is written to the repository: every output goes to a tmp_path.
Run from the repository root:  uv run --with pytest pytest scripts/build_livrets_test.py
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

import build_livrets
from build_livrets import card_row, cost, index, md_text, sort_key
from editions import CONTROLS, ENERGY, GOLD, HEROES, POTION_SLOTS

IRONCLAD = HEROES[0]


def card(card_id: str, name: str, rarity: str = "Common", pool: str = "Ironclad", **extra: object) -> dict:
    return {"id": card_id, "name": name, "rarity": rarity, "pool": pool, "type": "Attack", "energy": 1,
            "imagePath": f"/images/cards/{card_id.lower()}.webp", "descBase": f"{name} base", **extra}


def fixture_cards() -> dict[str, dict]:
    """Just enough cards for the Ironclad booklet: its starter deck, its key cards, and a few commons."""
    cards = {cid: card(cid, cid.title(), "Basic") for cid, _ in IRONCLAD.starter}
    cards.update({cid: card(cid, cid.title(), "Rare") for cid in IRONCLAD.key_cards})
    cards["ZEBRA"] = card("ZEBRA", "Zèbre")
    cards["ELAN"] = card("ELAN", "Élan")
    cards["ABEILLE"] = card("ABEILLE", "Abeille")
    cards["OTHER"] = card("OTHER", "Voleur", pool="Silent")
    return cards


RELICS = {IRONCLAD.relic_id: {"id": IRONCLAD.relic_id, "name": "Sang ardent", "imagePath": "/images/relics/b.webp",
                              "descI18n": "Heal <g>6</g> HP."}}


def all_relics() -> dict[str, dict]:
    return {h.relic_id: {"id": h.relic_id, "name": f"Relique {h.slug}", "imagePath": "/r.webp", "descI18n": "d"}
            for h in HEROES}


# --- Markup -----------------------------------------------------------------------------------


def test_md_text_turns_the_dump_markup_into_markdown() -> None:
    assert md_text("<g>Block</g>") == "**Block**"
    assert md_text("<s>2</s>") == "2 ★"
    assert md_text("<e>1</e>") == "1 ⚡"
    assert md_text("Gain [E] and <e/>") == "Gain ⚡ and ⚡"


def test_md_text_flattens_the_text_for_a_table_cell() -> None:
    assert md_text("line one\nline two") == "line one line two"
    assert md_text("a|b") == "a/b"  # a pipe would end the table cell
    assert md_text("  <i>x</i>  ") == "x"


def test_md_text_of_nothing_is_empty() -> None:
    assert md_text("") == "" and md_text(None) == ""  # type: ignore[arg-type]


# --- Sorting ----------------------------------------------------------------------------------


def test_sort_key_ignores_accents_and_case() -> None:
    assert sort_key("Élan") == "elan"
    assert sort_key("Âme") == "ame"
    names = ["Zèbre", "Élan", "abeille", "Éclair"]
    assert sorted(names, key=sort_key) == ["abeille", "Éclair", "Élan", "Zèbre"]
    assert sorted(names) != sorted(names, key=sort_key)  # a plain sort would put the accents last


@pytest.mark.xfail(strict=True, reason="sort_key drops the œ ligature instead of expanding it to 'oe'")
def test_sort_key_expands_the_oe_ligature() -> None:
    assert sort_key("Vœu") == "voeu"


# --- Cards ------------------------------------------------------------------------------------


@pytest.mark.parametrize(("energy", "expected"), [(0, "0"), (1, "1"), (3, "3"), (-1, "X"), (None, "X")])
def test_cost_is_the_energy_or_x(energy: int | None, expected: str) -> None:
    assert cost({"energy": energy}) == expected


def test_cost_of_a_card_without_energy_is_x() -> None:
    assert cost({}) == "X"


def test_card_row_has_five_cells_with_both_descriptions() -> None:
    row = card_row({"name": "Heurt", "imagePath": "/images/b.webp", "energy": 2, "type": "Attack",
                    "descBase": "Deal <g>8</g>.", "descUpgraded": "Deal <g>10</g>."})
    assert row == "| ![Heurt](../images/b.webp) **Heurt** | 2 ⚡ | Attaque | Deal **8**. | Deal **10**. |"


def test_card_row_falls_back_to_the_base_text_when_there_is_no_upgrade() -> None:
    row = card_row({"name": "Heurt", "imagePath": "/b.webp", "energy": 0, "type": "Skill", "descriptionClean": "Plain.",
                    "descUpgraded": ""})
    assert row.endswith("| Compétence | Plain. | Plain. |")


def test_card_row_keeps_an_unknown_type_as_is() -> None:
    assert "| Curse |" in card_row({"name": "n", "imagePath": "/b.webp", "energy": 1, "type": "Curse", "descBase": "d"})


# --- Documents --------------------------------------------------------------------------------


def test_booklet_lists_the_hero_starter_deck_and_only_its_own_commons_sorted() -> None:
    text = build_livrets.booklet(IRONCLAD, fixture_cards(), RELICS)
    assert text.startswith(f"# Livret d'instructions — Slay the GBA : {IRONCLAD.edition}")
    assert f"{IRONCLAD.rom}" in text and f"{IRONCLAD.ref}" in text
    assert "Relique de départ : ![Sang ardent](../images/relics/b.webp) Sang ardent" in text
    assert "> Heal **6** HP." in text
    assert "* **5×** Strike_Ironclad" in text
    assert "## 8. Catalogue des 3 cartes communes" in text  # Zèbre, Élan, Abeille: not the Silent's Voleur
    assert "Voleur" not in text
    assert text.index("Abeille") < text.index("Élan") < text.index("Zèbre")


def test_booklet_counts_the_cards_of_each_rarity() -> None:
    text = build_livrets.booklet(IRONCLAD, fixture_cards(), RELICS)
    assert "**9 cartes** au total : 2 de base, 3 communes, 0 peu communes, 4 rares et 0 anciennes" in text


def test_index_has_one_row_and_one_link_per_edition() -> None:
    text = index(all_relics())
    for hero in HEROES:
        assert f"| `{hero.ref}` | **{hero.edition}**" in text
        assert f"[Livret](livret_{hero.slug}.md)" in text
        assert f"Relique {hero.slug}" in text
        assert f"* **{hero.name}** — *{hero.tagline}*" in text
    assert text.count("[Livret](") == len(HEROES)


def test_index_states_the_shared_rules_and_controls() -> None:
    text = index(all_relics())
    assert f"{GOLD} pièces d'or" in text and f"{ENERGY} ⚡ par tour" in text and f"{POTION_SLOTS} emplacements" in text
    assert all(f"| {screen} | **{button}** | {action} |" in text for screen, button, action in CONTROLS)


def test_index_shows_difficulty_as_filled_and_empty_stars() -> None:
    text = index(all_relics())
    assert f"| {'★' * IRONCLAD.difficulty}{'☆' * (3 - IRONCLAD.difficulty)} |" in text


# --- Files ------------------------------------------------------------------------------------


def write_data(folder: Path, name: str, key: str, items: list[dict]) -> None:
    (folder / f"{name}_v0.111.0.json").write_text(json.dumps({key: items}))


def test_load_indexes_the_items_by_id(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    write_data(tmp_path, "card", "cards", [{"id": "A", "name": "a"}, {"id": "B", "name": "b"}])
    monkeypatch.setattr(build_livrets, "DATA", tmp_path)
    assert build_livrets.load("card", "cards") == {"A": {"id": "A", "name": "a"}, "B": {"id": "B", "name": "b"}}


def test_main_writes_one_booklet_per_edition_and_an_index(tmp_path: Path, monkeypatch: pytest.MonkeyPatch,
                                                          capsys: pytest.CaptureFixture[str]) -> None:
    data, out = tmp_path / "data", tmp_path / "livrets"
    data.mkdir()
    out.mkdir()
    cards = fixture_cards()
    for hero in HEROES:  # every edition needs its starter deck and key cards
        for cid, _ in hero.starter:
            cards.setdefault(cid, card(cid, cid.title(), "Basic", pool=hero.pool))
        for cid in hero.key_cards:
            cards.setdefault(cid, card(cid, cid.title(), "Rare", pool=hero.pool))
    write_data(data, "card", "cards", list(cards.values()))
    write_data(data, "relic", "relics", list(all_relics().values()))
    monkeypatch.setattr(build_livrets, "DATA", data)
    monkeypatch.setattr(build_livrets, "OUT", out)
    build_livrets.main()
    assert sorted(path.name for path in out.iterdir()) == sorted(
        ["README.md", *(f"livret_{hero.slug}.md" for hero in HEROES)])
    assert "livret_silent.md" in (out / "README.md").read_text()
    assert str(out) in capsys.readouterr().out
