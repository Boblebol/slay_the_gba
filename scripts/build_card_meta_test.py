"""The generated card table follows the engine's cards and the reference.

Run from the repository root:  uv run --with pytest pytest scripts/build_card_meta_test.py
"""

from __future__ import annotations

import re

import build_card_meta


def test_header_is_up_to_date() -> None:
    assert build_card_meta.OUT.read_text() == build_card_meta.render()


def test_one_row_per_engine_card() -> None:
    rows = re.findall(r"^    \{ CardRarity::(\w+), CardPool::(\w+) \}, // (\w+)$", build_card_meta.render(), re.M)
    assert [name for _, _, name in rows] == build_card_meta.engine_cards()
    assert rows[0] == ("NONE", "NONE", "NONE")


def test_known_cards() -> None:
    reference = build_card_meta.reference_cards()
    assert build_card_meta.meta_of("BASH", reference) == ("BASIC", "IRONCLAD")
    assert build_card_meta.meta_of("MANIFEST_AUTHORITY", reference) == ("UNCOMMON", "REGENT")
    # Named differently in the engine and in the reference.
    assert build_card_meta.meta_of("POISON_STAB", reference) == ("COMMON", "SILENT")
    assert build_card_meta.meta_of("NULL_CARD", reference) == ("UNCOMMON", "DEFECT")
    # One Strike for the five heroes, and cards the reference does not list.
    assert build_card_meta.meta_of("STRIKE", reference) == ("BASIC", "NONE")
    assert build_card_meta.meta_of("DAZED", reference) == ("NONE", "NONE")
    assert build_card_meta.meta_of("SHIV", reference) == ("NONE", "NONE")
