"""Tests of the title animation sprites: palette budget and seamless mist tiles.

Run from the repository root:  uv run --with pytest --with pillow --with numpy pytest scripts/build_title_fx_test.py
"""

from __future__ import annotations

import numpy as np

import build_title_fx


def test_mist_tiles_are_seamless_and_within_one_palette():
    for frame in (build_title_fx.mist_frame(11, 0.55), build_title_fx.mist_frame(29, 0.7)):
        assert frame.shape == (build_title_fx.MIST_H, build_title_fx.MIST_W)
        assert frame.max() <= 15  # 4bpp: index 0 transparent + 15 colours
        assert frame.max() > 0
        # Neighbouring columns across the wrap point stay as close as inside the tile (no seam).
        inside = np.abs(frame[:, 1:] > 0).mean()
        assert abs((frame[:, 0] > 0).mean() - (frame[:, -1] > 0).mean()) <= inside


def test_glow_grows_with_intensity():
    covered = [int((build_title_fx.glow_frame(level) > 0).sum()) for level in range(build_title_fx.GLOW_FRAMES)]
    assert covered == sorted(covered) and covered[0] < covered[-1]
    assert max(int(build_title_fx.glow_frame(level).max()) for level in range(build_title_fx.GLOW_FRAMES)) <= 15
