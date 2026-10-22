# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow>=10", "numpy>=1.26"]
# ///
"""Build the sprites animating the title screen: graphics/title_mist.bmp and graphics/title_glow.bmp.

Run from the repository root:  uv run scripts/build_title_fx.py

Both are procedural (no source image) and 4bpp with at most 15 colours each, so they only take two 16-colour
sprite palettes: the 8bpp key art already fills the whole background palette.
  - title_mist: 64x32, two frames (clouds, mist), seamless on the horizontal axis, drawn with hardware blending.
  - title_glow: 64x64, four frames of a dithered warm halo of growing intensity, pulsing on top of the Spire.
"""

from __future__ import annotations

import math

import numpy as np

from build_gba_art import OUT, TRANSPARENT, save_sprite

MIST_W, MIST_H = 64, 32
GLOW_SIZE = 64
GLOW_FRAMES = 4

MIST_PALETTE = [(72, 52, 88), (104, 72, 104), (144, 96, 120), (184, 128, 136), (216, 160, 152), (240, 200, 176)]
GLOW_PALETTE = [(120, 40, 24), (176, 64, 24), (224, 104, 32), (248, 160, 56), (255, 208, 96), (255, 240, 176)]

# 4x4 ordered-dither thresholds in [0, 1).
BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def tileable_noise(width: int, height: int, seed: int) -> np.ndarray:
    """Smooth value noise in [0, 1], periodic along x so that neighbouring tiles join without a seam."""
    rng = np.random.default_rng(seed)
    cells_x, cells_y = 4, 3
    grid = rng.random((cells_y + 1, cells_x))
    out = np.zeros((height, width))
    for y in range(height):
        gy = y / height * cells_y
        y0 = int(gy)
        fy = gy - y0
        fy = fy * fy * (3 - 2 * fy)
        for x in range(width):
            gx = x / width * cells_x
            x0 = int(gx) % cells_x
            x1 = (x0 + 1) % cells_x
            fx = gx - int(gx)
            fx = fx * fx * (3 - 2 * fx)
            top = grid[y0, x0] * (1 - fx) + grid[y0, x1] * fx
            bottom = grid[y0 + 1, x0] * (1 - fx) + grid[y0 + 1, x1] * fx
            out[y, x] = top * (1 - fy) + bottom * fy
    return out


def mist_frame(seed: int, density: float) -> np.ndarray:
    value = 0.65 * tileable_noise(MIST_W, MIST_H, seed) + 0.35 * tileable_noise(MIST_W, MIST_H, seed + 7)
    # Soft top and bottom edges so that the band melts into the picture.
    edge = np.sin(np.arange(MIST_H) / (MIST_H - 1) * math.pi) ** 0.7
    value = value * edge[:, None]
    frame = np.zeros((MIST_H, MIST_W), dtype=np.int32)
    levels = len(MIST_PALETTE)
    for y in range(MIST_H):
        for x in range(MIST_W):
            v = (value[y, x] - (1 - density)) / density
            if v <= 0:
                continue
            v = min(v, 0.999) * levels + BAYER[y % 4, x % 4] - 0.5
            frame[y, x] = 1 + int(min(max(v, 0), levels - 1))
    return frame


def glow_frame(level: int) -> np.ndarray:
    frame = np.zeros((GLOW_SIZE, GLOW_SIZE), dtype=np.int32)
    centre = (GLOW_SIZE - 1) / 2
    strength = (level + 1) / GLOW_FRAMES
    radius = GLOW_SIZE / 2 * (0.55 + 0.45 * strength)
    for y in range(GLOW_SIZE):
        for x in range(GLOW_SIZE):
            d = math.hypot((x - centre) / 0.8, y - centre) / radius
            if d >= 1:
                continue
            v = (1 - d) ** 1.6 * strength
            v = v * len(GLOW_PALETTE) + BAYER[y % 4, x % 4] - 0.5
            if v >= 0.5:
                frame[y, x] = 1 + min(len(GLOW_PALETTE) - 1, int(v))
    return frame


def main() -> None:
    save_sprite("title_mist", [mist_frame(11, 0.55), mist_frame(29, 0.7)], [TRANSPARENT] + MIST_PALETTE, MIST_H)
    save_sprite("title_glow", [glow_frame(level) for level in range(GLOW_FRAMES)], [TRANSPARENT] + GLOW_PALETTE,
                GLOW_SIZE)
    print("Title animation sprites generated in", OUT)


if __name__ == "__main__":
    main()
