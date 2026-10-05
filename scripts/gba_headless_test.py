"""The cartridge boots in the headless emulator and reacts to the buttons.

Skipped when the emulator (scripts/setup_headless_mgba.py) or the ROM (make ironclad) is not built.
"""

from __future__ import annotations

from pathlib import Path

import pytest

import gba_headless

ROM = gba_headless.ROOT / "slay_ironclad.gba"

pytestmark = pytest.mark.skipif(not gba_headless.available() or not ROM.exists(),
                                reason="needs scripts/setup_headless_mgba.py and make ironclad")


def colours(gba: gba_headless.Gba) -> int:
    return len(gba.screen().getcolors(maxcolors=240 * 160) or [])


def test_cartridge_boots_to_a_drawn_screen() -> None:
    # A cartridge that hangs at boot leaves a blank screen: one colour.
    gba = gba_headless.Gba(ROM)
    gba.wait(300)
    assert gba.screen().size == (240, 160)
    assert colours(gba) > 8


def test_buttons_change_the_screen(tmp_path: Path) -> None:
    gba = gba_headless.Gba(ROM)
    gba.wait(300)
    title = gba.screen().tobytes()
    for _ in range(3):
        gba.press("A", after=90)
    assert gba.screen().tobytes() != title
    gba.screenshot(tmp_path / "after.png")
    assert (tmp_path / "after.png").stat().st_size > 0
