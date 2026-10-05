# /// script
# requires-python = ">=3.11"
# dependencies = ["cffi", "pillow"]
# ///
"""Run a cartridge without a window: press buttons, let frames pass, save screenshots.

Replaces driving the mGBA application by hand for checks an agent or the CI can do alone. The emulator is
built once by scripts/setup_headless_mgba.py.

From the repository root, a list of steps played in order:

    uv run scripts/gba_headless.py slay_ironclad.gba wait:300 A A shot:combat.png

Steps: a button (A B L R START SELECT UP DOWN LEFT RIGHT), `wait:<frames>`, `shot:<file.png>`.
A button is held 6 frames, then 30 frames pass (`A:90` waits 90 frames instead). The game runs at 60 frames
per second. No save file is loaded or written: every run starts from an empty cartridge.

From Python:

    from gba_headless import Gba
    gba = Gba(Path("slay_ironclad.gba"))
    gba.wait(300)
    gba.press("A")
    gba.screenshot(Path("title.png"))
"""

from __future__ import annotations

import ctypes
import importlib.util
import platform
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
EMULATOR = ROOT / "tools" / "mgba"
BUTTONS = ("A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L")
HOLD_FRAMES = 6    # long enough for the game to see the press
AFTER_FRAMES = 30  # default pause after a press, so the screen settles


def available() -> bool:
    """Whether the emulator is built (scripts/setup_headless_mgba.py) and its bindings can be imported."""
    return (EMULATOR / "mgba" / "__init__.py").exists() and importlib.util.find_spec("_cffi_backend") is not None


def load_bindings() -> None:
    if not available():
        raise SystemExit("gba_headless: emulator not built (run `uv run scripts/setup_headless_mgba.py`) "
                         "or cffi missing (run this script with `uv run`)")
    if platform.system() == "Linux":
        # The extension names the library without a path: load it first so the loader finds it.
        ctypes.CDLL(str(EMULATOR / "mgba" / "libmgba.so.0.10"), mode=ctypes.RTLD_GLOBAL)
    if str(EMULATOR) not in sys.path:
        sys.path.insert(0, str(EMULATOR))


class Gba:
    """One cartridge running in memory."""

    def __init__(self, rom: Path) -> None:
        load_bindings()
        import mgba.core
        import mgba.image
        import mgba.log
        from mgba.gba import GBA

        if not rom.exists():
            raise SystemExit(f"gba_headless: {rom} not found, build it first (make ironclad, make silent…)")
        mgba.log.silence()
        self._keys = {name: getattr(GBA, f"KEY_{name}") for name in BUTTONS}
        self._core = mgba.core.load_path(str(rom))
        if self._core is None:
            raise SystemExit(f"gba_headless: {rom} is not a cartridge the emulator can load")
        width, height = self._core.desired_video_dimensions()
        self._image = mgba.image.Image(width, height)
        self._core.set_video_buffer(self._image)
        self._core.reset()

    def wait(self, frames: int) -> None:
        for _ in range(frames):
            self._core.run_frame()

    def press(self, button: str, after: int = AFTER_FRAMES) -> None:
        """Holds `button` a few frames, releases it, then lets `after` frames pass."""
        if button not in self._keys:
            raise SystemExit(f"gba_headless: unknown button {button!r}, expected one of {' '.join(BUTTONS)}")
        self._core.set_keys(self._keys[button])
        self.wait(HOLD_FRAMES)
        self._core.clear_keys(self._keys[button])
        self.wait(after)

    def screen(self) -> Image.Image:
        """The 240x160 picture currently displayed."""
        return self._image.to_pil().convert("RGB")

    def screenshot(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        self.screen().save(path)


def play(gba: Gba, steps: list[str]) -> None:
    for step in steps:
        name, _, argument = step.partition(":")
        if name == "wait":
            gba.wait(int(argument))
        elif name == "shot":
            gba.screenshot(Path(argument))
            print(f"saved {argument}")
        else:
            gba.press(name.upper(), int(argument) if argument else AFTER_FRAMES)


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    play(Gba(Path(sys.argv[1])), sys.argv[2:])


if __name__ == "__main__":
    main()
