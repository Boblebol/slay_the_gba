# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow>=10", "numpy>=1.26"]
# ///
"""Build graphics/bg_splash.bmp: the pixel-art portrait shown by the boot splash screen.

Run from the repository root:  uv run scripts/build_splash_bg.py <photo.jpg>

The photo is only read here and never committed: only the generated 48x48 portrait lands in the BMP.
"""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance

from build_gba_art import save_bg

# Square crop (left, top, right, bottom) around the face of the reference photo, in source pixels.
FACE_BOX = (700, 340, 1220, 860)
PORTRAIT_SIZE = 48
PORTRAIT_COLORS = 14
FRAME_COLOR = (214, 170, 72)
BACKDROP_COLOR = (10, 6, 16)


def build_portrait(photo: Path) -> Image.Image:
    face = Image.open(photo).convert("RGB").crop(FACE_BOX)
    face = face.resize((PORTRAIT_SIZE, PORTRAIT_SIZE), Image.Resampling.BOX)
    face = ImageEnhance.Contrast(face).enhance(1.2)
    return face.quantize(colors=PORTRAIT_COLORS, method=Image.Quantize.MEDIANCUT,
                         dither=Image.Dither.NONE).convert("RGB")


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit("usage: build_splash_bg.py <photo.jpg>")

    canvas = Image.new("RGB", (240, 160), BACKDROP_COLOR)
    left = (240 - PORTRAIT_SIZE) // 2
    top = 26
    draw = ImageDraw.Draw(canvas)
    draw.rectangle((left - 2, top - 2, left + PORTRAIT_SIZE + 1, top + PORTRAIT_SIZE + 1), outline=FRAME_COLOR)
    canvas.paste(build_portrait(Path(sys.argv[1])), (left, top))
    save_bg("bg_splash", canvas)
    print("Splash portrait generated")


if __name__ == "__main__":
    main()
