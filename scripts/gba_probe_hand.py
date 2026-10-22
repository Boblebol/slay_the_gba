# /// script
# requires-python = ">=3.11"
# dependencies = ["cffi", "pillow"]
# ///
"""Put chosen cards in the hand of a cartridge and take screenshots (checks of #280 and #869).

The game has no debug menu: build a throwaway ROM whose CombatEngine::draw_card() takes cards from two RAM
variables, never commit it. Apply this to src/engine/combat.cpp before `make HERO=...`:

    extern "C" { volatile uint16_t pt_ids[8] __attribute__((used)); volatile uint8_t pt_ctl[4] __attribute__((used)); }
    ... at the top of CombatEngine::draw_card(), after the hand-limit test:
    if (pt_ctl[1] < pt_ctl[0]) { hand.push_back(DeckCard(static_cast<CardId>(pt_ids[pt_ctl[1]]), pt_ctl[2] != 0));
                                 pt_ctl[1] = pt_ctl[1] + 1; return true; }

Then read the addresses with `arm-none-eabi-nm *.elf | grep pt_` and pass them with --ctl and --ids.

    uv run scripts/gba_probe_hand.py slay_regent.gba out/ DECISIONS_DECISIONS,VENERATE --ctl 0x0300246c --ids 0x03002470 \
        "sstart A:20 srefus RIGHT:15 A:40 sv1"

Steps: a button (`A:40` = hold then wait 40 frames), `w<frames>`, `s<name>` for a screenshot. The run starts a game
(Neow, first fight), ends the first turn, and the next hand is the cards given (up to 8, names as in card.h).
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gba_headless import Gba  # noqa: E402


def card_ids() -> dict[str, int]:
    src = (Path(__file__).resolve().parent.parent / "include/engine/card.h").read_text()
    body = src[src.index("enum class CardId"):]
    body = re.sub(r"//.*", "", body[:body.index("};")])
    names = re.findall(r"\b([A-Z][A-Z0-9_]*)\b", body.split("{", 1)[1])
    return {n: i for i, n in enumerate(names)}


def main() -> None:
    args = [a for a in sys.argv[1:]]
    ctl = int(args.pop(args.index("--ctl") + 1), 0); args.remove("--ctl")
    ids = int(args.pop(args.index("--ids") + 1), 0); args.remove("--ids")
    rom, out, cards, steps = args
    out_dir = Path(out); out_dir.mkdir(parents=True, exist_ok=True)
    names = cards.split(",")
    table = card_ids()
    gba = Gba(Path(rom)); mem = gba._core.memory
    gba.wait(300)
    for _ in range(6):  # title, menu, hero, Neow, relic, map -> first fight
        gba.press("A", 90)
    for i, name in enumerate(names):
        mem.u16.raw_write(ids + 2 * i, table[name])
    mem.u8.raw_write(ctl + 2, 0); mem.u8.raw_write(ctl + 1, 0); mem.u8.raw_write(ctl, len(names))
    gba.wait(200); gba.press("R", 240); gba.wait(60)
    n = 0
    for step in steps.split():
        if step[0] == "w":
            gba.wait(int(step[1:]))
        elif step[0] == "s":
            gba.screenshot(out_dir / f"{n:02d}_{step[1:]}.png"); n += 1
        else:
            button, _, wait = step.partition(":")
            gba.press(button, int(wait) if wait else 30)


if __name__ == "__main__":
    main()
