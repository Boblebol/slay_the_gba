"""Tests of the visual identity: one 16-colour palette for the logo, the web icon, the text and the dialog boxes.

Run from the repository root:  uv run --with pytest --with pillow --with numpy pytest scripts/build_identity_test.py
"""

from __future__ import annotations

import inspect
import re
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import build_gba_art
import build_identity
from build_identity import DIALOG_COLORS, FONT_COLORS, GLYPHS, IDENTITY_PALETTE, LOGO_PIECE

FONT_HEADER = build_gba_art.FONT_HEADER
FONT_VARIANT = {"white": "white", "red": "red", "gold": "gold", "blue": "blue", "gray": "gray", "violet": "purple"}


def font_fill(variant: str) -> tuple[int, int, int]:
    """Fill colour of a text colour variant of slay_font.h, back in 8-bit channels."""
    row = re.search(rf"slay_font_{variant}_colors\[\] = \{{\s*(.*?)\n", FONT_HEADER.read_text())
    assert row, variant
    colors = re.findall(r"bn::color\((\d+), (\d+), (\d+)\)", row.group(1))
    return tuple(int(c) << 3 for c in colors[build_gba_art.FILL])  # type: ignore[return-value]


def gba_palette() -> set[tuple[int, int, int]]:
    return {build_gba_art.to_gba(c) for c in IDENTITY_PALETTE.values()}


def test_the_palette_is_fifteen_distinct_gba_colours_plus_transparency() -> None:
    assert len(gba_palette()) == 15
    assert build_identity.identity_palette()[0] == build_gba_art.TRANSPARENT
    assert len(build_identity.identity_palette()) == 16


@pytest.mark.parametrize("name", FONT_COLORS)
def test_the_text_colours_of_the_game_font_belong_to_the_palette(name: str) -> None:
    assert font_fill(FONT_VARIANT[name]) == build_gba_art.to_gba(IDENTITY_PALETTE[name])


def test_the_dialog_box_colours_belong_to_the_palette() -> None:
    source = inspect.getsource(build_gba_art.draw_panel)
    for name in DIALOG_COLORS:
        assert str(IDENTITY_PALETTE[name]) in source, name


def test_the_committed_logo_is_the_built_one(tmp_path: Path) -> None:
    build_identity.build_logo(tmp_path)
    with Image.open(tmp_path / "title_logo.bmp") as built, \
            Image.open(build_gba_art.OUT / "title_logo.bmp") as committed:
        assert np.array_equal(np.asarray(built), np.asarray(committed))
        assert built.getpalette()[:48] == committed.getpalette()[:48]
    assert (tmp_path / "title_logo.json").read_text() == (build_gba_art.OUT / "title_logo.json").read_text()


def test_the_logo_is_five_full_32x32_frames_of_one_obj_palette() -> None:
    with Image.open(build_gba_art.OUT / "title_logo.bmp") as bmp:
        sheet = np.asarray(bmp)
        palette = bmp.getpalette()[:48]
    assert sheet.shape == (5 * LOGO_PIECE, LOGO_PIECE)
    assert int(sheet.max()) <= 15
    assert [tuple(palette[i:i + 3]) for i in range(0, 48, 3)] == [build_gba_art.to_gba(c) for c in build_identity.identity_palette()]
    for frame in range(5):
        assert (sheet[frame * LOGO_PIECE:(frame + 1) * LOGO_PIECE] > 0).sum() > 50, frame


def test_the_logo_keeps_a_transparent_margin_so_pieces_join_without_a_seam() -> None:
    logo = build_identity.logo_indices()
    assert not logo[0].any() and not logo[-1].any()
    assert not logo[:, 0].any() and not logo[:, -1].any()


def test_every_glyph_is_seven_by_nine() -> None:
    for letter, rows in GLYPHS.items():
        assert len(rows) == 9 and all(len(row) == 7 for row in rows), letter


def test_the_web_icon_is_a_round_32px_badge_drawn_with_the_palette() -> None:
    icon = np.asarray(build_identity.icon_image())
    assert icon.shape == (32, 32, 4)
    assert icon[0, 0, 3] == 0 and icon[16, 16, 3] == 255
    colours = {tuple(int(c) for c in px[:3]) for px in icon.reshape(-1, 4) if px[3]}
    assert colours <= gba_palette()
