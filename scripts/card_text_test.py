"""Checks that every card text fits the combat screen, measured with the widths of the GBA font.

The card text of the combat screen is a single line of CARD_TEXT_MAX_WIDTH pixels. The text is built in C++ by
card_description (src/gba/card_text.cpp) from the values of the card definitions (src/engine/card.cpp), and its
width comes from slay_font_character_widths (include/gba/slay_font.h). This test reads the three sources, rebuilds
every text for the base and the upgraded version of each card, and adds the character widths up.

A piece of the C++ stream that this reader does not understand fails the test with the name of the card.

Run from the repository root:  uv run --with pytest pytest scripts/card_text_test.py
"""

from __future__ import annotations

import re
import warnings
from dataclasses import dataclass
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
FONT_HEADER = ROOT / "include" / "gba" / "slay_font.h"
CARD_TEXT_SOURCE = ROOT / "src" / "gba" / "card_text.cpp"
CARD_SOURCE = ROOT / "src" / "engine" / "card.cpp"

CARD_TEXT_MAX_WIDTH = 224  # pixels of the single line of text under the card art
CARD_NAME_MAX_WIDTH = 115  # beyond it the name runs under the START reminder
ASCII_FIRST = 32
ASCII_LAST = 126

# Texts known to be wider than CARD_TEXT_MAX_WIDTH, as (card id, upgraded) -> measured width. They are listed
# rather than fixed here: card_text.cpp is not touched by this test. The test fails for a text that is wider and
# not listed, for a listed text that got wider, and for a listed text that now fits (remove it from the list).
KNOWN_TOO_WIDE: dict[tuple[str, bool], int] = {}


class ParseError(Exception):
    """A piece of source that the reader does not understand."""


# --- Reading the C++ sources -------------------------------------------------------------------------------------


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def string_literal_end(text: str, start: int) -> int:
    """Index just after the string literal that opens at text[start] (a double quote)."""
    i = start + 1
    while i < len(text):
        if text[i] == "\\":
            i += 2
        elif text[i] == '"':
            return i + 1
        else:
            i += 1
    raise ParseError(f"unterminated string literal: {text[start:start + 40]!r}")


def unquote(literal: str) -> str:
    body = literal[1:-1]
    if "\\" in body:
        raise ParseError(f"escape sequence in a string literal: {literal}")
    return body


def strip_comments(text: str) -> str:
    """Remove // and /* */ comments, leaving string literals alone."""
    out: list[str] = []
    i = 0
    while i < len(text):
        if text[i] == '"':
            end = string_literal_end(text, i)
            out.append(text[i:end])
            i = end
        elif text.startswith("//", i):
            while i < len(text) and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                raise ParseError("unterminated block comment")
            i = end + 2
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def split_top_level(text: str, separator: str) -> list[str]:
    """Split on a separator that is outside string literals and brackets."""
    parts: list[str] = []
    depth = 0
    start = 0
    i = 0
    while i < len(text):
        char = text[i]
        if char == '"':
            i = string_literal_end(text, i)
            continue
        if char in "({[":
            depth += 1
        elif char in ")}]":
            depth -= 1
        elif depth == 0 and text.startswith(separator, i):
            parts.append(text[start:i])
            i += len(separator)
            start = i
            continue
        i += 1
    parts.append(text[start:])
    return [part.strip() for part in parts]


def function_body(text: str, signature: str) -> str:
    """The text between the braces of the function whose definition contains `signature`."""
    at = text.find(signature)
    if at < 0:
        raise ParseError(f"function not found: {signature}")
    i = text.index("{", at)
    depth = 0
    start = i + 1
    while i < len(text):
        if text[i] == '"':
            i = string_literal_end(text, i)
            continue
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
        i += 1
    raise ParseError(f"unbalanced braces in {signature}")


# --- Font ---------------------------------------------------------------------------------------------------------


def array_body(text: str, name: str) -> str:
    match = re.search(rf"{re.escape(name)}\[\]\s*=\s*\{{(.*?)\}};", text, re.DOTALL)
    if match is None:
        raise ParseError(f"array not found: {name}")
    return match.group(1)


def load_font_widths(header: Path) -> dict[str, int]:
    """Width in pixels of every character of the font: ASCII 32..126, then slay_font_utf8_characters."""
    text = read(header)
    widths = [int(w) for w in re.findall(r"-?\d+", array_body(text, "slay_font_character_widths"))]
    utf8 = re.findall(r'"([^"]*)"', array_body(text, "slay_font_utf8_characters"))
    characters = [chr(code) for code in range(ASCII_FIRST, ASCII_LAST + 1)] + utf8
    if len(widths) != len(characters):
        raise ParseError(f"{len(widths)} widths for {len(characters)} characters in {header.name}")
    return dict(zip(characters, widths))


# --- Card definitions -----------------------------------------------------------------------------------------------


@dataclass(frozen=True)
class CardValues:
    """The numbers of a CardDef row that the texts print, for the base and the upgraded version."""

    card_type: str
    value: int
    secondary: int
    upgraded_value: int
    upgraded_secondary: int

    def pick(self, upgraded: bool) -> tuple[int, int]:
        return (self.upgraded_value, self.upgraded_secondary) if upgraded else (self.value, self.secondary)


# Positions of the fields of a CardDef initializer (include/engine/card.h).
FIELD_ID, FIELD_TYPE, FIELD_VALUE, FIELD_SECONDARY = 0, 3, 5, 6
FIELD_UPGRADED_VALUE, FIELD_UPGRADED_SECONDARY = 10, 11
MIN_FIELDS = 13


def parse_card_int(field: str, card_id: str, what: str) -> int:
    if not re.fullmatch(r"\d+", field):
        raise ParseError(f"{card_id}: {what} is not a plain integer in card.cpp: {field!r}")
    return int(field)


def load_card_values(source: Path) -> dict[str, CardValues]:
    text = strip_comments(read(source))
    rows = split_top_level(function_body(text, "CardDef CARDS[]"), ",")
    cards: dict[str, CardValues] = {}
    for row in rows:
        if not row:
            continue
        if not (row.startswith("{") and row.endswith("}")):
            raise ParseError(f"unexpected entry in CARDS: {row[:60]!r}")
        fields = split_top_level(row[1:-1], ",")
        if len(fields) < MIN_FIELDS or not fields[FIELD_ID].startswith("CardId::"):
            raise ParseError(f"unreadable CardDef row: {row[:80]!r}")
        card_id = fields[FIELD_ID].removeprefix("CardId::")
        cards[card_id] = CardValues(
            card_type=fields[FIELD_TYPE].removeprefix("CardType::"),
            value=parse_card_int(fields[FIELD_VALUE], card_id, "value"),
            secondary=parse_card_int(fields[FIELD_SECONDARY], card_id, "secondary"),
            upgraded_value=parse_card_int(fields[FIELD_UPGRADED_VALUE], card_id, "upgraded value"),
            upgraded_secondary=parse_card_int(fields[FIELD_UPGRADED_SECONDARY], card_id, "upgraded secondary"),
        )
    return cards


# --- card_description and base_name ---------------------------------------------------------------------------------

CASE_LABEL = re.compile(r"case\s+engine::CardId::(\w+)\s*:")
DEFAULT_LABEL = re.compile(r"default\s*:")
COMPARISON = re.compile(r"(value|secondary)\s*(>=|<=|==|!=|>|<)\s*(\d+)")
ENCHANTMENT_TEST = re.compile(r"card\.enchantment\s*(==|!=)\s*engine::Enchantment::(\w+)")
LITERALS = re.compile(r'"[^"]*"(?:\s*"[^"]*")*')
# Enchantments are applied during a run, and a card starts without one.
DEFAULT_ENCHANTMENT = "NONE"


def evaluate_condition(condition: str, upgraded: bool, value: int, secondary: int) -> bool:
    condition = condition.strip()
    if condition == "card.upgraded":
        return upgraded
    compare = COMPARISON.fullmatch(condition)
    if compare is not None:
        left = value if compare.group(1) == "value" else secondary
        right = int(compare.group(3))
        return {">=": left >= right, "<=": left <= right, "==": left == right, "!=": left != right,
                ">": left > right, "<": left < right}[compare.group(2)]
    enchantment = ENCHANTMENT_TEST.fullmatch(condition)
    if enchantment is not None:
        same = enchantment.group(2) == DEFAULT_ENCHANTMENT
        return same if enchantment.group(1) == "==" else not same
    raise ParseError(f"unknown condition: {condition!r}")


def evaluate_piece(piece: str, upgraded: bool, value: int, secondary: int) -> str:
    """The text one `<<` piece writes into the stream."""
    piece = piece.strip()
    if LITERALS.fullmatch(piece):
        return "".join(unquote(literal) for literal in re.findall(r'"[^"]*"', piece))
    if piece == "value":
        return str(value)
    if piece == "secondary":
        return str(secondary)
    if piece.startswith("(") and piece.endswith(")"):
        inner = piece[1:-1]
        halves = split_top_level(inner, "?")
        if len(halves) == 2:
            branches = split_top_level(halves[1], ":")
            if len(branches) == 2:
                taken = branches[0] if evaluate_condition(halves[0], upgraded, value, secondary) else branches[1]
                return evaluate_piece(taken, upgraded, value, secondary)
        raise ParseError(f"unreadable ternary: {piece!r}")
    raise ParseError(f"unreadable piece: {piece!r}")


@dataclass(frozen=True)
class Statement:
    """One statement of a case body, as a list of the pieces of a `stream << ...;`, or an error."""

    pieces: tuple[str, ...]


def parse_stream_statement(statement: str) -> Statement:
    pieces = split_top_level(statement.removeprefix("stream").strip().removeprefix("<<"), "<<")
    if not pieces or any(not piece for piece in pieces):
        raise ParseError(f"empty piece in: {statement[:80]!r}")
    return Statement(tuple(pieces))


def split_cases(body: str) -> list[tuple[list[str], list[str] | ParseError]]:
    """Group a switch body into (case labels, statements); the statements may be a ParseError instead.

    Labels that follow each other share the statements after the last one. `default:` is dropped.
    """
    groups: list[tuple[list[str], list[str] | ParseError]] = []
    labels: list[str] = []
    statements: list[str] = []
    problem: ParseError | None = None
    in_default = False

    def close() -> None:
        nonlocal labels, statements, problem, in_default
        if labels:
            groups.append((labels, problem if problem is not None else statements))
        labels, statements, problem, in_default = [], [], None, False

    i = 0
    while i < len(body):
        if body[i].isspace():
            i += 1
            continue
        label = CASE_LABEL.match(body, i)
        default = DEFAULT_LABEL.match(body, i)
        if label is not None or default is not None:
            if statements or problem is not None or in_default:
                close()
            if label is not None:
                labels.append(label.group(1))
                i = label.end()
            else:
                in_default = True
                assert default is not None
                i = default.end()
            continue
        # A statement runs to the next top-level `;`.
        j = i
        while j < len(body) and body[j] != ";":
            if body[j] == '"':
                j = string_literal_end(body, j)
            else:
                j += 1
        statement = body[i:j].strip()
        i = j + 1
        if in_default or (not labels):
            continue
        if statement == "break":
            close()
        elif statement.startswith("stream") and "<<" in statement:
            statements.append(statement)
        else:
            problem = ParseError(f"statement other than a stream write: {statement[:80]!r}")
    close()
    return groups


def load_descriptions(source: Path) -> dict[str, tuple[Statement, ...] | ParseError]:
    """The stream writes of card_description for every card with a case, or why they could not be read."""
    body = function_body(strip_comments(read(source)), "void card_description(")
    switch_at = body.index("switch")
    cases = function_body(body[switch_at:], "{")
    result: dict[str, tuple[Statement, ...] | ParseError] = {}
    for labels, statements in split_cases(cases):
        if isinstance(statements, ParseError):
            parsed: tuple[Statement, ...] | ParseError = statements
        else:
            try:
                parsed = tuple(parse_stream_statement(statement) for statement in statements)
            except ParseError as error:
                parsed = error
        for label in labels:
            result[label] = parsed
    return result


def load_names(source: Path) -> dict[str, str | ParseError]:
    """The string base_name returns for every card, or why it could not be read."""
    body = function_body(strip_comments(read(source)), "bn::string_view base_name(")
    switch_at = body.index("switch")
    cases = function_body(body[switch_at:], "{")
    result: dict[str, str | ParseError] = {}
    for labels, statements in split_cases(cases.replace("return", "stream <<")):
        name: str | ParseError
        if isinstance(statements, ParseError) or len(statements) != 1:
            name = statements if isinstance(statements, ParseError) else ParseError("not a single return")
        else:
            literal = statements[0].removeprefix("stream <<").strip()
            if LITERALS.fullmatch(literal):
                name = evaluate_piece(literal, False, 0, 0)
            else:
                name = ParseError(f"base_name does not return a string literal: {literal[:60]!r}")
        for label in labels:
            result[label] = name
    return result


# --- Measuring ------------------------------------------------------------------------------------------------------


def text_width(text: str, widths: dict[str, int]) -> int:
    missing = sorted({char for char in text if char not in widths})
    if missing:
        raise ParseError(f"characters missing from the font: {''.join(missing)!r} in {text!r}")
    return sum(widths[char] for char in text)


def card_text(pieces: tuple[Statement, ...], upgraded: bool, values: CardValues) -> str:
    value, secondary = values.pick(upgraded)
    return "".join(
        evaluate_piece(piece, upgraded, value, secondary) for statement in pieces for piece in statement.pieces
    )


def card_name_text(base: str, upgraded: bool, values: CardValues) -> str:
    # card_name adds "+" to an upgraded card, except for a Status card.
    return base + "+" if upgraded and values.card_type != "STATUS" else base


FONT = load_font_widths(FONT_HEADER)
VALUES = load_card_values(CARD_SOURCE)
DESCRIPTIONS = load_descriptions(CARD_TEXT_SOURCE)
NAMES = load_names(CARD_TEXT_SOURCE)

VERSIONS = [(card_id, upgraded) for card_id in DESCRIPTIONS for upgraded in (False, True)]


def label(card_id: str, upgraded: bool) -> str:
    return f"{card_id}+" if upgraded else card_id


def measure_description(card_id: str, upgraded: bool) -> tuple[str, int]:
    """The text of a card and its width; raises a ParseError that names the card if it cannot be read."""
    try:
        statements = DESCRIPTIONS[card_id]
        if isinstance(statements, ParseError):
            raise statements
        if card_id not in VALUES:
            raise ParseError("no CardDef row in card.cpp")
        text = card_text(statements, upgraded, VALUES[card_id])
        return text, text_width(text, FONT)
    except ParseError as error:
        raise ParseError(f"{label(card_id, upgraded)}: {error}") from error


# --- Tests ----------------------------------------------------------------------------------------------------------


def test_font_covers_ascii_and_utf8_characters() -> None:
    assert len(FONT) == (ASCII_LAST - ASCII_FIRST + 1) + 49
    assert FONT[" "] == 6 and FONT["A"] == 7 and FONT["é"] == 6


def test_card_text_reader_finds_the_cards() -> None:
    assert len(DESCRIPTIONS) > 100, "card_description cases not found"
    assert set(DESCRIPTIONS) <= set(VALUES), f"cards without a CardDef row: {sorted(set(DESCRIPTIONS) - set(VALUES))}"


def test_reader_evaluates_a_text_by_hand() -> None:
    # "Inflige 6 dégâts." in Strike's base version, "Inflige 9 dégâts." upgraded.
    text, width = measure_description("STRIKE", False)
    assert text == "Inflige 6 dégâts."
    assert width == sum(FONT[char] for char in text)
    assert measure_description("STRIKE", True)[0] == "Inflige 9 dégâts."
    # The upgraded version of Infinite Blades starts with an extra word, through a ternary on card.upgraded.
    assert measure_description("INFINITE_BLADES", True)[0].startswith("Inné. ")
    assert measure_description("INFINITE_BLADES", False)[0].startswith("Début de tour")


@pytest.mark.parametrize("card_id,upgraded", VERSIONS, ids=[label(c, u) for c, u in VERSIONS])
def test_card_text_fits_the_combat_line(card_id: str, upgraded: bool) -> None:
    try:
        text, width = measure_description(card_id, upgraded)
    except ParseError as error:
        pytest.fail(f"unreadable card text, {error}")
    known = KNOWN_TOO_WIDE.get((card_id, upgraded))
    name = label(card_id, upgraded)
    if known is None:
        assert width <= CARD_TEXT_MAX_WIDTH, f"{name} is {width}px, over {CARD_TEXT_MAX_WIDTH}px: {text!r}"
    else:
        assert width > CARD_TEXT_MAX_WIDTH, f"{name} now fits ({width}px): remove it from KNOWN_TOO_WIDE"
        assert width <= known, f"{name} got wider: {width}px, listed at {known}px: {text!r}"


def test_known_too_wide_lists_real_cards() -> None:
    unknown = [label(card_id, upgraded) for card_id, upgraded in KNOWN_TOO_WIDE if card_id not in DESCRIPTIONS]
    assert not unknown, f"KNOWN_TOO_WIDE names cards without a text: {unknown}"


def test_card_names_under_the_start_reminder() -> None:
    """Reports the names wider than CARD_NAME_MAX_WIDTH; it does not fail for them, only for an unreadable name."""
    too_wide: list[str] = []
    unreadable: list[str] = []
    for card_id, upgraded in [(c, u) for c in NAMES for u in (False, True)]:
        base = NAMES[card_id]
        try:
            if isinstance(base, ParseError):
                raise base
            if card_id not in VALUES:
                raise ParseError("no CardDef row in card.cpp")
            name = card_name_text(base, upgraded, VALUES[card_id])
            width = text_width(name, FONT)
        except ParseError as error:
            unreadable.append(f"{label(card_id, upgraded)}: {error}")
            continue
        if width > CARD_NAME_MAX_WIDTH:
            too_wide.append(f"{width:3d}px  {label(card_id, upgraded)}  {name}")
    assert not unreadable, "unreadable card names:\n" + "\n".join(unreadable)
    if too_wide:
        report = f"{len(too_wide)} card names over {CARD_NAME_MAX_WIDTH}px (under the START reminder):\n" + "\n".join(
            sorted(too_wide, reverse=True)
        )
        print(report)
        warnings.warn(report, stacklevel=1)


def test_cards_without_a_text_case_are_listed() -> None:
    """A card with a CardDef row and no case in card_description shows no text: list them rather than skip them."""
    silent = sorted(set(VALUES) - set(DESCRIPTIONS) - {"NONE"})
    if silent:
        warnings.warn(f"{len(silent)} cards have no card_description case: {', '.join(silent)}", stacklevel=1)
