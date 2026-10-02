"""Check that a built cartridge leaves enough internal RAM for its stack.

The GBA has 32 KB of fast internal RAM (IWRAM): static data sits at the bottom, the stack grows down from the
top. Data that should live in the ROM but ends up in IWRAM (a table built at start-up instead of at compile
time) eats the stack room until the game hangs on a white screen at boot, with nothing failing at build time.

Run from the repository root after a ROM build:  uv run scripts/check_rom_memory.py [hero ...]
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MIN_STACK_ROOM = 8 * 1024  # bytes between the end of the static data and the top of the user stack


def symbol(map_text: str, name: str) -> int:
    """Address the linker map gives to `name`."""
    match = re.search(rf"^\s+(0x[0-9a-f]+)\s+{re.escape(name)} = ", map_text, re.M)
    if not match:
        raise SystemExit(f"check_rom_memory: symbol {name} not found in the linker map")
    return int(match.group(1), 16)


def stack_room(map_text: str) -> int:
    """Bytes of IWRAM left between the static data and the top of the user stack."""
    return symbol(map_text, "__sp_usr") - symbol(map_text, "__iheap_start")


def built_maps(heroes: list[str]) -> list[Path]:
    if heroes:
        return [ROOT / f"build_slay_{hero}" / f"slay_{hero}.map" for hero in heroes]
    return sorted(ROOT.glob("build_slay_*/slay_*.map"))


def main() -> None:
    maps = built_maps(sys.argv[1:])
    if not maps:
        raise SystemExit("check_rom_memory: no linker map found, build a ROM first")
    failed = False
    for path in maps:
        if not path.exists():
            raise SystemExit(f"check_rom_memory: {path} not found, build that ROM first")
        room = stack_room(path.read_text())
        verdict = "ok" if room >= MIN_STACK_ROOM else f"TOO SMALL (minimum {MIN_STACK_ROOM})"
        print(f"{path.stem}: {room} bytes of IWRAM left for the stack - {verdict}")
        failed = failed or room < MIN_STACK_ROOM
    if failed:
        raise SystemExit("check_rom_memory: static data fills the internal RAM. Look for a table that is not "
                         "constexpr: the map lists it under .bss or .data in the 0x03000000 range.")


if __name__ == "__main__":
    main()
