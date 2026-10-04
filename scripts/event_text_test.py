"""Checks that every text of every event fits the event screen, measured with the widths of the GBA font.

The event screen (src/gba/scene_event.cpp) shows, on lines of TEXT_WIDTH pixels: the title on one line, the
story on at most STORY_LINES lines, each choice on one line behind the cursor, what the selected choice costs
and gives on at most DETAIL_LINES lines, then the result on at most RESULT_LINES lines above the summary of
what the choice did. The texts come from the event table of src/engine/event_manager.cpp.

The table is also compared with bible/data: an event of the reference is met in the acts the reference gives.

Run from the repository root:  uv run --with pytest pytest scripts/event_text_test.py
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path

import pytest

from card_text_test import FONT, text_width
from potion_belt_text_test import sanitize, wrap

ROOT = Path(__file__).resolve().parent.parent
EVENT_SOURCE = ROOT / "src" / "engine" / "event_manager.cpp"
EVENT_SCENE = ROOT / "src" / "gba" / "scene_event.cpp"
EVENT_DATA = ROOT / "bible" / "data" / "event_v0.111.0.json"

TEXT_WIDTH = 224
STORY_LINES = 5
DETAIL_LINES = 2
RESULT_LINES = 3
MAX_CHOICES = 4
CURSOR = "► "
CURSOR_BOB = 3  # pixels the selected choice moves to the right
LINE_BYTES = 63  # a TextLine is a bn::string<64>
SANITIZE_MARGIN = 4  # gba::sanitize stops this many bytes before the end of a TextLine

FIELD = re.compile(r'\.(id|title|dialogue|zones|label|cost_benefit|result_message|condition) = (?:"([^"]*)"|([^,\n]+))')
ZONE_OF_ACT = {"act1a": "EVENT_ZONE_ACT1A", "act1b": "EVENT_ZONE_ACT1B", "act2": "EVENT_ZONE_ACT2",
               "act3": "EVENT_ZONE_ACT3"}
ALL_ZONES = set(ZONE_OF_ACT.values())


@dataclass
class Choice:
    label: str
    cost_benefit: str = ""
    result_message: str = ""
    conditional: bool = False


@dataclass
class Event:
    event_id: str
    title: str = ""
    dialogue: str = ""
    zones: set[str] = field(default_factory=set)
    choices: list[Choice] = field(default_factory=list)


def table_source() -> str:
    """The initializer of EVENT_TABLE, without its comments."""
    source = EVENT_SOURCE.read_text("utf-8")
    start = source.index("constexpr EventEncounter EVENT_TABLE[]")
    end = source.index("\n};", start)
    return re.sub(r"//.*", "", source[start:end])


def load_events() -> list[Event]:
    events: list[Event] = []
    for match in FIELD.finditer(table_source()):
        name, text, expression = match.group(1), match.group(2), (match.group(3) or "").strip()
        if name == "id":
            events.append(Event(text))
        elif name == "title":
            events[-1].title = text
        elif name == "dialogue":
            events[-1].dialogue = text
        elif name == "zones":
            zones = set(re.findall(r"EVENT_ZONE_\w+", expression))
            events[-1].zones = ALL_ZONES if "EVENT_ZONE_ALL" in zones else zones
        elif name == "label":
            events[-1].choices.append(Choice(text))
        elif name == "cost_benefit":
            events[-1].choices[-1].cost_benefit = text
        elif name == "result_message":
            events[-1].choices[-1].result_message = text
        elif name == "condition":
            events[-1].choices[-1].conditional = True
    return events


def scene_constant(name: str) -> int:
    match = re.search(rf"constexpr int {name} = (\d+);", EVENT_SCENE.read_text("utf-8"))
    assert match, f"{name} not found in {EVENT_SCENE.name}"
    return int(match.group(1))


def assert_fits(owner: str, what: str, text: str, max_lines: int) -> None:
    lines = wrap(text, TEXT_WIDTH)
    assert 1 <= len(lines) <= max_lines, f"{owner}: {what} takes {len(lines)} lines, {max_lines} at most: {lines}"
    for line in lines:
        assert text_width(sanitize(line), FONT) <= TEXT_WIDTH, f"{owner}: {what} line too wide: {line!r}"
        assert len(line.encode("utf-8")) <= LINE_BYTES - SANITIZE_MARGIN, f"{owner}: {what} line too long: {line!r}"


EVENTS = load_events()
REFERENCE = {event["id"]: event for event in json.loads(EVENT_DATA.read_text("utf-8"))["events"]}


def test_reader_finds_the_events() -> None:
    ids = [event.event_id for event in EVENTS]
    assert len(ids) == len(set(ids)) and len(ids) >= 4
    assert {"ABYSSAL_BATHS", "BATTLEWORN_DUMMY", "COLOSSAL_FLOWER", "MYSTIC_ALTAR"} <= set(ids)
    for event in EVENTS:
        assert event.title and event.dialogue and event.zones, event.event_id
        assert 2 <= len(event.choices) <= MAX_CHOICES, event.event_id
        for choice in event.choices:
            assert choice.label and choice.cost_benefit and choice.result_message, event.event_id


def test_the_test_measures_the_screen_as_it_is() -> None:
    assert scene_constant("TEXT_WIDTH") == TEXT_WIDTH
    assert scene_constant("MAX_LINES") == STORY_LINES
    assert scene_constant("DETAIL_LINES") == DETAIL_LINES
    assert scene_constant("RESULT_MESSAGE_LINES") == RESULT_LINES
    # The result is copied into `char message[N]` before it is shown.
    assert scene_constant("RESULT_MESSAGE_BYTES") >= RESULT_LINES * (LINE_BYTES + 1)


@pytest.mark.parametrize("event", EVENTS, ids=[event.event_id for event in EVENTS])
def test_event_fits_the_screen(event: Event) -> None:
    assert_fits(event.event_id, "title", event.title, 1)
    assert_fits(event.event_id, "story", event.dialogue, STORY_LINES)

    for choice in event.choices:
        owner = f"{event.event_id} / {choice.label}"
        # The choice is drawn behind the cursor, in a single TextLine.
        row = CURSOR + choice.label
        assert text_width(sanitize(row), FONT) + CURSOR_BOB <= TEXT_WIDTH, f"{owner}: choice too wide"
        assert len(row.encode("utf-8")) <= LINE_BYTES - SANITIZE_MARGIN, f"{owner}: choice too long for a TextLine"
        # A choice that can be refused gives its reason on the first line: one line is left for the detail.
        assert_fits(owner, "detail", choice.cost_benefit, 1 if choice.conditional else DETAIL_LINES)
        assert_fits(owner, "result", choice.result_message, RESULT_LINES)


def test_engine_messages_fit_the_screen() -> None:
    """Result messages the engine picks by itself, outside the table."""
    source = re.sub(r"//.*", "", EVENT_SOURCE.read_text("utf-8"))
    messages = re.findall(r'message = "([^"]*)"sv', source)
    assert messages, "no engine message found: update this test"
    for message in messages:
        assert_fits("engine", "result", message, RESULT_LINES)


@pytest.mark.parametrize("event", EVENTS, ids=[event.event_id for event in EVENTS])
def test_event_is_met_in_the_acts_of_the_reference(event: Event) -> None:
    reference = REFERENCE.get(event.event_id)
    if reference is None:
        pytest.skip("not an event of the reference")
    assert event.zones == {ZONE_OF_ACT[act] for act in reference["acts"]}
