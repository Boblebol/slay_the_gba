"""Tests of the implementation tracker, against the real engine sources and reference data.

Run from the repository root:  uv run --with pytest pytest scripts/progress_test.py
"""

from __future__ import annotations

import pytest

import progress
from progress import DONE, EXCLUDED, PARTIAL, TODO, Catalogue, Category, Item, Tally


def find(section: progress.Section, category: str, key: str) -> Item:
    items = next(c for c in section.categories if c.title == category).items
    return next(item for item in items if item.key == key)


def test_partial_counts_for_half_and_excluded_is_not_counted() -> None:
    category = Category("x", (Item("a", "a", DONE), Item("b", "b", PARTIAL), Item("c", "c", TODO),
                              Item("d", "d", EXCLUDED)))
    assert category.tally == Tally(done=1, partial=1, todo=1, excluded=1)
    assert category.tally.total == 3
    assert category.tally.percent == pytest.approx(50.0)


def test_empty_tally_is_zero_percent() -> None:
    assert Tally().percent == 0.0


def test_shared_strike_and_defend_count_for_every_hero() -> None:
    cards = progress.implemented_cards()
    assert {"BASH", "STRIKE_IRONCLAD", "STRIKE_NECROBINDER", "DEFEND_REGENT"} <= cards
    assert "STRIKE" not in cards and "LANTERN_KEY" not in cards


def test_cards_created_in_combat_are_matched_by_engine_identifier() -> None:
    assert progress.engine_id("MinionDiveBomb") == "MINION_DIVE_BOMB" and progress.engine_id("Burn") == "BURN"
    tokens = {entry["id"] for entry in progress.token_entries(progress.load("card"))}
    assert {"BURN", "VOID", "SOVEREIGN_BLADE"} <= tokens
    # The engine calls the Void card VOID_CARD.
    assert "VOID" in progress.implemented_cards()


def test_engine_names_are_mapped_to_reference_identifiers() -> None:
    cards = progress.implemented_cards()
    assert "POISONED_STAB" in cards and "POISON_STAB" not in cards


def test_detection_fails_loudly_when_the_engine_changes() -> None:
    with pytest.raises(SystemExit):
        progress.detect(r"no_such_engine_construct (\w+)", progress.ENGINE / "combat.cpp")


def test_statuses_follow_the_engine_and_the_overrides() -> None:
    result = progress.compute()
    assert find(result.section("ironclad"), "Cartes", "BASH").status == DONE
    assert find(result.section("ironclad"), "Reliques", "BURNING_BLOOD").status == DONE
    assert find(result.section("necrobinder"), "Cartes", "UNLEASH").status == DONE
    assert find(result.section("bestiaire"), "Boss", "CEREMONIAL_BEAST").status == DONE
    assert find(result.section("silent"), "Cartes", "ABRASIVE").status == DONE
    assert find(result.section("cartes"), "Quêtes", "LANTERN_KEY").status == TODO
    assert find(result.section("ironclad"), "Cartes", "BLAZE").status == EXCLUDED  # co-op card
    assert 0 < result.percent < 100
    assert result.tally == sum((section.tally for section in result.sections), Tally())


def section(slug: str, *tallies: Tally) -> progress.Section:
    def items(t: Tally) -> tuple[Item, ...]:
        counts = ((DONE, t.done), (PARTIAL, t.partial), (TODO, t.todo), (EXCLUDED, t.excluded))
        return tuple(Item(f"{status}{i}", "x", status) for status, n in counts for i in range(n))

    return progress.Section(slug, slug, tuple(progress.Category(f"c{i}", items(t)) for i, t in enumerate(tallies)))


def test_global_percent_is_the_mean_of_the_section_percentages() -> None:
    big = section("big", Tally(done=1, todo=99))  # 1 %
    small = section("small", Tally(done=1, todo=1))  # 50 %
    assert progress.Progress((big, small)).percent == pytest.approx(25.5)  # not 2/101 of the items


def test_global_percent_skips_sections_without_counted_items() -> None:
    empty = section("empty", Tally(excluded=3))
    nothing = section("nothing")
    full = section("full", Tally(done=2, partial=2))  # 75 %
    assert progress.Progress((empty, nothing, full)).percent == pytest.approx(75.0)
    assert progress.Progress((empty, nothing)).percent == 0.0


def test_summary_total_row_shows_the_global_percentage() -> None:
    prog = progress.Progress((section("a", Tally(done=1)), section("b", Tally(todo=3))))
    assert progress.summary_table(prog)[-1].endswith(f"**{progress.percent_fr(50.0)}** |")
    assert progress.GLOBAL_RULE in progress.markdown_report(prog)


def test_version_follows_the_percentage_until_everything_is_done() -> None:
    assert progress.next_version(Tally(done=10, partial=0, todo=90), []) == "0.10.0"
    assert progress.next_version(Tally(done=199, partial=0, todo=1), ["v0.99.0", "v0.99.1", "v0.9.7"]) == "0.99.2"
    assert progress.next_version(Tally(done=99, partial=1, todo=0), []) == "0.99.0"  # a partial item is not done
    assert progress.next_version(Tally(done=100, partial=0, todo=0, excluded=5), ["v0.99.3"]) == "1.0.0"
    assert progress.next_version(Tally(done=100, partial=0, todo=0), ["v1.0.0"]) == "1.0.1"
    assert progress.next_version(Tally(), []) == "0.0.0"
    assert progress.next_version(Tally(done=1, todo=99), [], percent=25.5) == "0.25.0"  # the global figure wins


def test_override_of_an_unknown_entry_is_rejected() -> None:
    catalogue = Catalogue({"overrides": {"cards": {"NOT_A_CARD": {"status": "oui"}}}})
    catalogue.category("Cartes", "cards", [{"id": "BASH", "name": "Heurt", "detail": ""}], {"BASH"})
    with pytest.raises(SystemExit):
        catalogue.check_overrides()


def test_invalid_status_is_rejected() -> None:
    with pytest.raises(SystemExit):
        progress.read_entry({"name": "x", "status": "done"}, "test")
