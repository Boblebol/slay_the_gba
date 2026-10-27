"""Web font made from the cartridge font: every game glyph plus the web extras, one GBA pixel per 1/8 em."""

import pytest

fontTools = pytest.importorskip("fontTools")  # only installed by the font script itself (uv run)

import build_site_font  # noqa: E402


def test_game_glyphs_follow_the_header_widths() -> None:
    glyphs = build_site_font.game_glyphs()
    assert {"A", "é", "Œ", "★", "►"} <= glyphs.keys()
    rows, width = glyphs["A"]
    assert len(rows) == 8 and width == 7 and rows[0].count("#") > 0


def test_build_writes_a_woff2_with_french_and_web_glyphs(tmp_path, monkeypatch) -> None:
    from fontTools.ttLib import TTFont

    monkeypatch.setattr(build_site_font, "OUT", tmp_path / "slay.woff2")
    font = TTFont(build_site_font.build())
    cmap = font.getBestCmap()
    assert all(ord(ch) in cmap for ch in "Aé·–→✓ ")
    assert font["head"].unitsPerEm == 8 * build_site_font.PX
