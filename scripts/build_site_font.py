# /// script
# requires-python = ">=3.12"
# dependencies = ["pillow>=10", "fonttools>=4.50", "brotli>=1.1"]
# ///
"""Turn the cartridge's 8x8 sprite font into the website's web font (site/assets/fonts/slay-gba.woff2).

Run from the repository root after a font change:  uv run scripts/build_site_font.py

The glyphs and widths come from graphics/slay_font.bmp and include/gba/slay_font.h (Butano's variable 8x8 font,
zlib, plus the project's French glyphs). Only the fill pixels become outlines: the site draws the in-game
bottom-right shadow with a CSS text-shadow. A few glyphs the web pages need and the game does not (middle dot,
dashes, curly quotes, arrows) are drawn here. One font pixel is 1/8 em: at 16px the text is the GBA's at 2x.
"""

from __future__ import annotations

import re
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
FONT_BMP = ROOT / "graphics" / "slay_font.bmp"
FONT_HEADER = ROOT / "include" / "gba" / "slay_font.h"
OUT = ROOT / "site" / "assets" / "fonts" / "slay-gba.woff2"

FILL = 14  # palette index of the glyph body; index 12 is the baked shadow
PX = 128  # font units per GBA pixel
ASCENT = 7  # rows above the baseline; the last row holds the shadow and the commas
SPACE = 4  # narrower than the game's 6: web paragraphs are longer than a GBA text box

WEB_GLYPHS = {  # character: (8x8 fill mask, advance in pixels)
    # The game's comma is a period with a tail lost in its shadow: French decimals (100,0 %) need a visible one.
    ",": (["........", "........", "........", "........", "........", "##......", ".#......", "#......."], 3),
    "·": (["........", "........", "........", "##......", "##......", "........", "........", "........"], 3),
    "–": (["........", "........", "........", "........", "#####...", "........", "........", "........"], 6),
    "‘": ([".#......", "#.......", "##......", "##......", "........", "........", "........", "........"], 3),
    "“": ([".#.#....", "#.#.....", "####....", "####....", "........", "........", "........", "........"], 5),
    "”": (["####....", "####....", ".#.#....", "#.#.....", "........", "........", "........", "........"], 5),
    "→": (["........", "....#...", "....##..", "#######.", "....##..", "....#...", "........", "........"], 8),
    "←": (["........", "..#.....", ".##.....", "#######.", ".##.....", "..#.....", "........", "........"], 8),
    "↑": (["...#....", "..###...", ".#####..", "...#....", "...#....", "...#....", "...#....", "........"], 8),
    "↓": (["...#....", "...#....", "...#....", "...#....", ".#####..", "..###...", "...#....", "........"], 8),
    "✓": (["........", "......#.", ".....##.", "#...##..", "##.##...", ".###....", "..#.....", "........"], 8),
    "✔": (["........", "......#.", ".....##.", "#...##..", "##.##...", ".###....", "..#.....", "........"], 8),
    "▶": (["#.......", "##......", "###.....", "####....", "###.....", "##......", "#.......", "........"], 6),
    "◀": (["...#....", "..##....", ".###....", "####....", ".###....", "..##....", "...#....", "........"], 6),
    "◐": (["........", "..###...", ".##..#..", ".###.#..", ".##..#..", "..###...", "........", "........"], 8),
    "☰": (["........", "######..", "........", "######..", "........", "######..", "........", "........"], 8),
    "€": ([".####...", "#.......", "####....", "#.......", "####....", "#.......", ".####...", "........"], 6),
}


def game_glyphs() -> dict[str, tuple[list[str], int]]:
    """Every character of the cartridge font, as fill masks with the game's advance widths."""
    header = FONT_HEADER.read_text(encoding="utf-8")
    extra = re.findall(r'"([^"]+)"', header.split("utf8_characters[] = {")[1].split("};")[0])
    widths = [int(w) for w in header.split("character_widths[] = {")[1].split("};")[0].replace("\n", "").split(",") if w.strip()]
    chars = [chr(c) for c in range(33, 127)] + extra
    image = Image.open(FONT_BMP)
    pixels = image.load()
    assert image.height == 8 * len(chars) and len(widths) == len(chars) + 1, "font sheet and header disagree"
    glyphs = {}
    for i, ch in enumerate(chars):
        rows = ["".join("#" if pixels[x, i * 8 + y] == FILL else "." for x in range(8)) for y in range(8)]
        glyphs[ch] = (rows, widths[i + 1])  # widths[0] is the space
    return glyphs


def draw(rows: list[str]):
    """TrueType outline of a mask: one rectangle per horizontal run of pixels, clockwise."""
    pen = TTGlyphPen(None)
    for y, row in enumerate(rows):
        top, bottom = (ASCENT - y) * PX, (ASCENT - y - 1) * PX
        for run in re.finditer(r"#+", row):
            left, right = run.start() * PX, run.end() * PX
            pen.moveTo((left, bottom))
            pen.lineTo((left, top))
            pen.lineTo((right, top))
            pen.lineTo((right, bottom))
            pen.closePath()
    return pen.glyph()


def build() -> Path:
    glyphs = game_glyphs() | WEB_GLYPHS
    names = {ch: f"uni{ord(ch):04X}" for ch in glyphs}
    order = [".notdef", "space", *names.values()]
    outlines = {".notdef": draw([]), "space": draw([])} | {names[ch]: draw(rows) for ch, (rows, _) in glyphs.items()}
    metrics = {".notdef": (6 * PX, 0), "space": (SPACE * PX, 0)}
    for ch, (rows, width) in glyphs.items():
        metrics[names[ch]] = (width * PX, 0)
    fb = FontBuilder(8 * PX, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap({32: "space", 160: "space"} | {ord(ch): name for ch, name in names.items()})
    fb.setupGlyf(outlines)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ASCENT * PX, descent=-(8 - ASCENT) * PX)
    fb.setupNameTable({"familyName": "Slay GBA", "styleName": "Regular",
                       "copyright": "Butano variable 8x8 font (c) Gustavo Valiente, zlib; French glyphs (c) Alexandre Enouf, MIT"})
    fb.setupOS2(sTypoAscender=ASCENT * PX, sTypoDescender=-(8 - ASCENT) * PX, sTypoLineGap=0,
                usWinAscent=ASCENT * PX, usWinDescent=(8 - ASCENT) * PX, fsSelection=0x40)
    fb.setupPost()
    fb.font.flavor = "woff2"
    OUT.parent.mkdir(parents=True, exist_ok=True)
    fb.save(OUT)
    return OUT


if __name__ == "__main__":
    print(f"Web font written to {build().relative_to(ROOT)}")
