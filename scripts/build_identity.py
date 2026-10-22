# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow>=10", "numpy>=1.26"]
# ///
"""Build the visual identity of Slay the GBA: its 16-colour palette, the title logo and the web icon.

Run from the repository root:  uv run scripts/build_identity.py

Writes graphics/title_logo.bmp (+ .json): the pixel-art "SLAY the GBA" logo of the title screen, cut in
32x32 pieces (one sprite frame each) so that the title can drop them in one after the other.
The web icon (favicon) is drawn by `icon_image()`, which scripts/build_site.py calls.

Everything is drawn with IDENTITY_PALETTE: 15 colours (+ transparency) that also hold the in-game text
colours (include/gba/slay_font.h) and the dark night blue of the Pokemon-like dialog boxes.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image

from build_gba_art import OUT, TRANSPARENT, to_gba, write_indexed_bmp, write_json

# One OBJ palette: index 0 is transparent, these are indices 1 to 15.
IDENTITY_PALETTE: dict[str, tuple[int, int, int]] = {
    "ink": (16, 16, 24),           # outlines, text shadow
    "night": (14, 16, 30),         # dialog box body
    "dusk": (40, 44, 70),          # dialog box sheen, logo back shadow
    "maroon": (96, 16, 32),        # logo drop shadow
    "blood": (176, 32, 40),        # bottom of the logo letters
    "red": (248, 88, 72),          # damage text, the red of the cartridges
    "ember": (232, 128, 40),       # middle of the logo letters
    "gold": (248, 208, 72),        # titles and gold text
    "pale_gold": (248, 240, 168),  # top of the logo letters
    "white": (248, 248, 248),      # body text, highlights
    "steel": (176, 184, 200),      # small logo words
    "slate": (104, 112, 136),      # small logo words shading
    "gray": (152, 152, 168),       # secondary text
    "blue": (120, 184, 248),       # keywords
    "violet": (216, 152, 248),     # rare keywords
}
assert len(IDENTITY_PALETTE) == 15

# The text colours of include/gba/slay_font.h that the identity keeps unchanged.
FONT_COLORS = ("white", "red", "gold", "blue", "gray", "violet")
DIALOG_COLORS = ("night", "dusk")  # draw_panel() of scripts/build_gba_art.py

LOGO_PIECE = 32  # width and height of one logo sprite frame
LOGO_WIDTH = 5 * LOGO_PIECE

# 7x9 block letters, strokes two columns wide.
GLYPHS: dict[str, list[str]] = {
    "S": [".#####.", "##...##", "##.....", "##.....", ".#####.", ".....##", ".....##", "##...##", ".#####."],
    "L": ["##.....", "##.....", "##.....", "##.....", "##.....", "##.....", "##.....", "##.....", "#######"],
    "A": ["..###..", ".##.##.", "##...##", "##...##", "#######", "##...##", "##...##", "##...##", "##...##"],
    "Y": ["##...##", "##...##", ".##.##.", "..###..", "..###..", "..###..", "..###..", "..###..", "..###.."],
    "T": ["#######", "#######", "..###..", "..###..", "..###..", "..###..", "..###..", "..###..", "..###.."],
    "H": ["##...##", "##...##", "##...##", "#######", "#######", "##...##", "##...##", "##...##", "##...##"],
    "E": ["#######", "##.....", "##.....", "#####..", "##.....", "##.....", "##.....", "##.....", "#######"],
    "G": [".#####.", "##...##", "##.....", "##.....", "##.####", "##...##", "##...##", "##...##", ".#####."],
    "B": ["######.", "##...##", "##...##", "######.", "##...##", "##...##", "##...##", "##...##", "######."],
}
GLYPH_W, GLYPH_H = 7, 9

# (word, scale): the big words take two pixels per glyph cell, "THE" one.
LOGO_WORDS = (("SLAY", 2), ("THE", 1), ("GBA", 2))
LETTER_GAP = 2
WORD_GAP = 7


def word_mask(word: str, scale: int) -> np.ndarray:
    """Boolean mask of a word in the block letters, each cell `scale` pixels wide."""
    width = len(word) * GLYPH_W * scale + (len(word) - 1) * LETTER_GAP
    mask = np.zeros((GLYPH_H * scale, width), dtype=bool)
    for i, letter in enumerate(word):
        cells = np.array([[c == "#" for c in row] for row in GLYPHS[letter]])
        x = i * (GLYPH_W * scale + LETTER_GAP)
        mask[:, x:x + GLYPH_W * scale] = np.kron(cells, np.ones((scale, scale), dtype=bool))
    return mask


def grow(mask: np.ndarray) -> np.ndarray:
    """The mask and its four neighbours."""
    out = mask.copy()
    out[1:, :] |= mask[:-1, :]
    out[:-1, :] |= mask[1:, :]
    out[:, 1:] |= mask[:, :-1]
    out[:, :-1] |= mask[:, 1:]
    return out


def shift(mask: np.ndarray, dx: int, dy: int) -> np.ndarray:
    out = np.zeros_like(mask)
    h, w = mask.shape
    out[dy:, dx:] = mask[:h - dy, :w - dx]
    return out


def paint_letters(canvas: np.ndarray, mask: np.ndarray, x: int, y: int, ramp: list[str]) -> None:
    """Paint a word mask at (x, y): the colours of `ramp` from top to bottom, white on upper top edges."""
    h, w = mask.shape
    names = list(IDENTITY_PALETTE)
    for row in range(h):
        name = ramp[min(len(ramp) - 1, row * len(ramp) // h)]
        cols = mask[row]
        canvas[y + row, x:x + w][cols] = names.index(name) + 1
    top_edge = mask & ~shift(mask, 0, 1)
    top_edge[h // 2:] = False  # light comes from above: only the upper half catches it
    canvas[y:y + h, x:x + w][top_edge] = names.index("white") + 1


def logo_indices() -> np.ndarray:
    """Indexed logo (0 = transparent), LOGO_WIDTH x LOGO_PIECE."""
    names = list(IDENTITY_PALETTE)
    canvas = np.zeros((LOGO_PIECE, LOGO_WIDTH), dtype=np.uint8)
    masks = [(word_mask(word, scale), scale) for word, scale in LOGO_WORDS]
    total = sum(m.shape[1] for m, _ in masks) + WORD_GAP * (len(masks) - 1)
    big_h = GLYPH_H * 2
    top = (LOGO_PIECE - big_h - 4) // 2
    x = (LOGO_WIDTH - total) // 2
    letters = np.zeros_like(canvas, dtype=bool)
    placed = []
    for mask, scale in masks:
        y = top + big_h - mask.shape[0] if scale == 1 else top  # small words sit on the baseline
        letters[y:y + mask.shape[0], x:x + mask.shape[1]] |= mask
        placed.append((mask, scale, x, y))
        x += mask.shape[1] + WORD_GAP
    outline = grow(letters)
    shadow = shift(outline, 2, 2) | shift(outline, 1, 2)
    canvas[shadow] = names.index("maroon") + 1
    canvas[outline] = names.index("ink") + 1
    for mask, scale, x, y in placed:
        ramp = ["pale_gold", "gold", "gold", "ember", "blood"] if scale == 2 else ["white", "steel", "slate"]
        paint_letters(canvas, mask, x, y, ramp)
    return canvas


def identity_palette() -> list[tuple[int, int, int]]:
    """The 16 entries of the logo BMP: transparency first."""
    return [TRANSPARENT] + [to_gba(c) for c in IDENTITY_PALETTE.values()]


def logo_frames() -> np.ndarray:
    """The logo cut in LOGO_PIECE-wide frames, stacked vertically as a Butano sprite sheet expects."""
    logo = logo_indices()
    pieces = [logo[:, i:i + LOGO_PIECE] for i in range(0, LOGO_WIDTH, LOGO_PIECE)]
    return np.vstack(pieces)


def to_rgba(indices: np.ndarray) -> Image.Image:
    palette = np.array(identity_palette(), dtype=np.uint8)
    rgba = np.zeros(indices.shape + (4,), dtype=np.uint8)
    rgba[..., :3] = palette[indices]
    rgba[..., 3] = np.where(indices > 0, 255, 0)
    return Image.fromarray(rgba, "RGBA")


def icon_indices(size: int = 32) -> np.ndarray:
    """Web icon: the logo's golden S on a round blood-red shield with an ink rim."""
    names = list(IDENTITY_PALETTE)
    canvas = np.zeros((size, size), dtype=np.uint8)
    ys, xs = np.mgrid[0:size, 0:size]
    r = np.hypot(xs + 0.5 - size / 2, ys + 0.5 - size / 2)
    canvas[r <= size / 2] = names.index("ink") + 1
    canvas[r <= size / 2 - 1.5] = names.index("maroon") + 1
    canvas[r <= size / 2 - 3] = names.index("blood") + 1
    canvas[(r <= size / 2 - 3) & (ys < size * 0.42)] = names.index("red") + 1
    s = word_mask("S", 3)
    h, w = s.shape
    x, y = (size - w) // 2, (size - h) // 2
    outline = np.zeros_like(canvas, dtype=bool)
    outline[y:y + h, x:x + w] = s
    canvas[shift(grow(outline), 1, 1) | grow(outline)] = names.index("ink") + 1
    paint_letters(canvas, s, x, y, ["pale_gold", "gold", "gold", "ember"])
    return canvas


def icon_image(size: int = 32) -> Image.Image:
    return to_rgba(icon_indices(size))


def logo_image() -> Image.Image:
    return to_rgba(logo_indices())


def build_logo(out: Path = OUT) -> None:
    write_indexed_bmp(out / "title_logo.bmp", logo_frames(), identity_palette())
    write_json(out / "title_logo.json", {"type": "sprite", "bpp_mode": "bpp_4", "height": LOGO_PIECE})


def main() -> None:
    build_logo()
    print("Logo written to", OUT / "title_logo.bmp")


if __name__ == "__main__":
    main()
