"""Checks the documentation of the events (#252) written by scripts/build_event_docs.py.

The committed files must agree with each other: bible/data/event_game_v0.111.0.json holds every event of the
registry, and bible/18_evenements.md shows each choice of that file. They are NOT compared with the code here (that
needs a C++ build, and events keep landing on main): `uv run scripts/build_event_docs.py --check` does it.

Run from the repository root:  uv run --with pytest pytest scripts/build_event_docs_test.py
"""

from __future__ import annotations

import json
import re

import build_event_docs as docs

GAME = json.loads(docs.GAME_JSON.read_text())
REFERENCE = json.loads((docs.DATA / "event_v0.111.0.json").read_text())["events"]
TOME = docs.TOME.read_text()


def test_every_event_of_the_reference_is_documented() -> None:
    ids = {event["id"] for event in GAME["events"]}
    assert {event["id"] for event in REFERENCE} <= ids
    assert len(REFERENCE) == 57


def test_tome_is_rendered_from_the_data_file() -> None:
    assert docs.render_tome(TOME, GAME) == TOME


def test_each_choice_of_the_game_is_in_the_tome() -> None:
    reference_ids = {event["id"] for event in REFERENCE}
    for event in GAME["events"]:
        if event["id"] not in reference_ids:
            continue  # Mystic Altar is not an event of the reference, so the tome has no entry for it
        section = re.search(rf"<!-- choix:{event['id']} -->(.*?)<!-- /choix:{event['id']} -->", TOME, re.S)
        assert section, event["id"]
        if not event["implemented"]:
            assert "Pas encore dans le jeu" in section.group(1)
            continue
        for page in event["pages"]:
            for option in page["options"]:
                assert f"**{option['label']}**" in section.group(1), (event["id"], option["label"])
                assert option["result"] in section.group(1), (event["id"], option["label"])


def test_implemented_choices_have_a_label_a_cost_and_a_result() -> None:
    for event in GAME["events"]:
        for page in event.get("pages", []):
            for option in page["options"]:
                assert option["label"] and option["cost_benefit"] and option["result"], event["id"]


def test_effects_are_described_in_french() -> None:
    effect = {"type": "LOSE_GOLD", "amount": 35, "ref": "", "card": "", "pick": "CHOOSE", "upgraded": False,
              "enchantment": "", "enchant_amount": 0, "only_type": -1, "relic_pool": [], "card_pool": []}
    assert docs.describe_effect(effect) == "perd 35 Or"
    assert docs.describe_effect({**effect, "type": "GOTO_PAGE", "amount": 2}) == "mène à la page 2"
    assert docs.describe_condition({"check": "MIN_GOLD", "amount": 35, "ref": "", "card": ""}) == "au moins 35 Or"
