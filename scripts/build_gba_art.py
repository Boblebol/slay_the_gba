# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow>=10", "numpy>=1.26"]
# ///
"""Build every GBA-ready art asset (sprites, backgrounds, font) from the bible/ image dump.

Run from the repository root:  uv run scripts/build_gba_art.py

Outputs land in graphics/ (Butano does not scan sub-folders) and include/gba/slay_font.h.
Every sprite is 4bpp (index 0 = transparent) so that many of them can share the 16 OBJ palettes;
backgrounds are 8bpp, one per scene, composited at build time with their static UI frames.
"""

from __future__ import annotations

import json
import math
import random
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
BIBLE = ROOT / "bible" / "images"
OUT = ROOT / "graphics"
FONT_HEADER = ROOT / "include" / "gba" / "slay_font.h"
CARD_ART_HEADER = ROOT / "include" / "gba" / "card_art.h"
BUTANO_FONT = ROOT / "butano" / "common" / "graphics" / "common_variable_8x8_font.bmp"

TRANSPARENT = (255, 0, 255)
HEROES = ["ironclad", "silent", "defect", "regent", "necrobinder"]

# ---------------------------------------------------------------------------------------------
# Generic helpers
# ---------------------------------------------------------------------------------------------


def to_gba(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    """Snap a colour to the 15-bit GBA gamut so previews match the hardware."""
    return tuple((c >> 3) << 3 for c in rgb)  # type: ignore[return-value]


def write_indexed_bmp(path: Path, indices: np.ndarray, palette: list[tuple[int, int, int]]) -> None:
    """Write an 8-bit indexed BMP (Butano derives the colour count from the highest index used)."""
    img = Image.fromarray(indices.astype(np.uint8), mode="P")
    flat: list[int] = []
    for color in palette:
        flat.extend(to_gba(color))
    flat.extend([0, 0, 0] * (256 - len(palette)))
    img.putpalette(flat)
    img.save(path, format="BMP")


def write_json(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, indent=4) + "\n")


def quantize_opaque(pixels: np.ndarray, colors: int) -> list[tuple[int, int, int]]:
    """Median-cut palette for a (N, 3) array of opaque RGB pixels."""
    if len(pixels) == 0:
        return [(0, 0, 0)]
    strip = Image.fromarray(pixels.reshape(1, -1, 3).astype(np.uint8), mode="RGB")
    quant = strip.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = quant.getpalette()[: colors * 3]
    used = sorted(set(np.array(quant).flatten().tolist()))
    return [tuple(pal[i * 3 : i * 3 + 3]) for i in used]  # type: ignore[misc]


def nearest_indices(rgb: np.ndarray, palette: list[tuple[int, int, int]]) -> np.ndarray:
    pal = np.array(palette, dtype=np.int32)
    diff = rgb[:, :, None, :].astype(np.int32) - pal[None, None, :, :]
    # Weighted distance (green matters most to the eye).
    dist = (diff[..., 0] ** 2) * 3 + (diff[..., 1] ** 2) * 4 + (diff[..., 2] ** 2) * 2
    return np.argmin(dist, axis=2)


def rgba_to_sprite(img: Image.Image, colors: int = 15, outline: bool = False,
                   palette: list[tuple[int, int, int]] | None = None) -> tuple[np.ndarray, list[tuple[int, int, int]]]:
    """Quantise an RGBA image into 4bpp sprite indices (0 = transparent)."""
    arr = np.array(img.convert("RGBA"))
    alpha = arr[:, :, 3] >= 110
    rgb = arr[:, :, :3]
    if palette is None:
        palette = quantize_opaque(rgb[alpha], colors)
    idx = nearest_indices(rgb, palette) + 1
    idx[~alpha] = 0
    if outline:
        darkest = 1 + int(np.argmin([sum(c) for c in palette]))
        grown = alpha.copy()
        grown[1:, :] |= alpha[:-1, :]
        grown[:-1, :] |= alpha[1:, :]
        grown[:, 1:] |= alpha[:, :-1]
        grown[:, :-1] |= alpha[:, 1:]
        ring = grown & ~alpha
        idx[ring] = darkest
    return idx, [TRANSPARENT] + list(palette)


def fit_art(img: Image.Image, box: tuple[int, int], bottom_align: bool = True) -> Image.Image:
    """Crop to the alpha bounding box and fit into `box`, keeping aspect ratio."""
    img = img.convert("RGBA")
    bbox = img.getchannel("A").point(lambda a: 255 if a > 40 else 0).getbbox()
    if bbox:
        img = img.crop(bbox)
    scale = min(box[0] / img.width, box[1] / img.height)
    size = (max(1, round(img.width * scale)), max(1, round(img.height * scale)))
    img = img.resize(size, Image.Resampling.LANCZOS)
    img = ImageEnhance.Contrast(img).enhance(1.12)
    img = ImageEnhance.Color(img).enhance(1.15)
    canvas = Image.new("RGBA", box, (0, 0, 0, 0))
    x = (box[0] - size[0]) // 2
    y = box[1] - size[1] if bottom_align else (box[1] - size[1]) // 2
    canvas.paste(img, (x, y), img)
    return canvas


def save_sprite(name: str, frames: list[np.ndarray], palette: list[tuple[int, int, int]], height: int) -> None:
    stacked = np.vstack(frames)
    write_indexed_bmp(OUT / f"{name}.bmp", stacked, palette)
    write_json(OUT / f"{name}.json", {"type": "sprite", "bpp_mode": "bpp_4", "height": height})


def save_bg(name: str, img: Image.Image, colors: int = 255, dither: bool = False, bpp4: bool = False) -> None:
    """Save a regular BG. `img` is the visible 240xH content, padded to a 256-multiple canvas."""
    w = 256 * math.ceil(img.width / 256)
    h = 256 * math.ceil(img.height / 256)
    canvas = Image.new("RGB", (w, h), (0, 0, 0))
    canvas.paste(img.convert("RGB"), (0, 0))
    ncolors = 15 if bpp4 else colors
    quant = canvas.quantize(colors=ncolors, method=Image.Quantize.MEDIANCUT,
                            dither=Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE)
    pal = quant.getpalette()[: ncolors * 3]
    idx = np.array(quant).astype(np.int32) + 1  # keep index 0 as the backdrop colour
    palette = [(0, 0, 0)] + [tuple(pal[i * 3 : i * 3 + 3]) for i in range(ncolors)]
    write_indexed_bmp(OUT / f"{name}.bmp", idx, palette)  # type: ignore[arg-type]
    write_json(OUT / f"{name}.json", {"type": "regular_bg", "bpp_mode": "bpp_4" if bpp4 else "bpp_8"})


def pixel_art(rows: list[str], colors: dict[str, tuple[int, int, int]]) -> Image.Image:
    h, w = len(rows), max(len(r) for r in rows)
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch in colors:
                img.putpixel((x, y), colors[ch] + (255,))
    return img


# ---------------------------------------------------------------------------------------------
# Monsters & bosses
# ---------------------------------------------------------------------------------------------

# Engine EnemyType -> STS2 monster art. Bosses get a 128x128 sprite split in four 64x64 frames.
MONSTERS: dict[str, str] = {
    "cultist": "damp_cultist",
    "jaw_worm": "fuzzy_wurm_crawler",
    "louse": "shrinker_beetle",
    "gremlin_nob": "gremlin_merc",
    "fuzzy_wurm_crawler": "fuzzy_wurm_crawler",
    "crossbow_ruby_raider": "crossbow_ruby_raider",
    "brute_ruby_raider": "brute_ruby_raider",
    "assassin_ruby_raider": "assassin_ruby_raider",
    "axe_ruby_raider": "axe_ruby_raider",
    "tracker_ruby_raider": "tracker_ruby_raider",
    "calcified_cultist": "calcified_cultist",
    "damp_cultist": "damp_cultist",
    "sewer_clam": "sewer_clam",
    "the_forgotten": "the_forgotten",
    "living_shield": "living_shield",
    "fat_gremlin": "fat_gremlin",
    "vine_shambler": "vine_shambler",
    "eye_with_teeth": "eye_with_teeth",
    "stabbot": "stabbot",
    "spiny_toad": "spiny_toad",
    "noisebot": "noisebot",
    "zapbot": "zapbot",
    "cubex_construct": "cubex_construct",
    "punch_construct": "punch_construct",
    "globe_head": "globe_head",
    "bowlbug_nectar": "bowlbug_nectar",
    "axebot": "axebot",
    "turret_operator": "turret_operator",
    "bowlbug_rock": "bowlbug_rock",
    "inklet": "inklet",
    "nibbit": "nibbit",
    "flyconid": "flyconid",
    "corpse_slug": "corpse_slug",
    "gas_bomb": "gas_bomb",
    "gremlin_merc": "gremlin_merc",
    "sneaky_gremlin": "sneaky_gremlin",
    "haunted_ship": "haunted_ship",
    "sludge_spinner": "sludge_spinner",
    "toadpole": "toadpole",
    "hunter_killer": "hunter_killer",
    "mawler": "mawler",
    "fossil_stalker": "fossil_stalker",
    "seapunk": "seapunk",
    "osty": "osty",
    "chomper": "chomper",
    "slumbering_beetle": "slumbering_beetle",
    "parafright": "parafright",
    "tunneler": "tunneler",
    "slimed_berserker": "slimed_berserker",
    "frog_knight": "frog_knight",
    "louse_progenitor": "louse_progenitor",
    "guardbot": "guardbot",
    "devoted_sculptor": "devoted_sculptor",
    "the_lost": "the_lost",
    "bowlbug_silk": "bowlbug_silk",
    "bowlbug_egg": "bowlbug_egg",
    "phrog_parasite": "phrog_parasite",
}
# Art drawn half transparent in the dump: the body would fall under the sprite's opacity threshold and only
# a few bright pixels would be left. Its faint parts are made solid first, on a dark body.
TRANSLUCENT_MONSTERS = {"eye_with_teeth"}
BOSSES: dict[str, str] = {
    "ceremonial_beast": "ceremonial_beast",
}


def solid_art(img: Image.Image) -> Image.Image:
    """Translucent art over a dark body, opaque wherever the artist drew something."""
    img = img.convert("RGBA")
    alpha = img.getchannel("A")
    body = Image.new("RGBA", img.size, (28, 40, 36, 255))
    body.alpha_composite(img)
    body.putalpha(alpha.point(lambda a: 255 if a >= 45 else 0))
    return body


def build_monsters() -> None:
    for name, src in MONSTERS.items():
        source = Image.open(BIBLE / "monsters" / f"{src}.webp")
        if name in TRANSLUCENT_MONSTERS:
            source = solid_art(source)
        art = fit_art(source, (62, 62))
        canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
        canvas.paste(art, (1, 1), art)
        idx, pal = rgba_to_sprite(canvas, 15, outline=False)
        save_sprite(f"mon_{name}", [idx], pal, 64)

    for name, src in BOSSES.items():
        # The arena is only ~90px tall above the ground line: keep bosses wide rather than tall.
        art = fit_art(Image.open(BIBLE / "monsters" / f"{src}.webp"), (126, 90))
        canvas = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
        canvas.paste(art, (1, 128 - 90 - 1), art)
        idx, pal = rgba_to_sprite(canvas, 15)
        quads = [idx[0:64, 0:64], idx[0:64, 64:128], idx[64:128, 0:64], idx[64:128, 64:128]]
        save_sprite(f"boss_{name}", quads, pal, 64)

        icon = fit_art(Image.open(BIBLE / "monsters" / f"{src}.webp"), (30, 30), bottom_align=False)
        canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        canvas.paste(icon, (1, 1), icon)
        idx, pal = rgba_to_sprite(canvas, 15, outline=True)
        save_sprite(f"map_boss_{name}", [idx], pal, 32)


# ---------------------------------------------------------------------------------------------
# Cards
# ---------------------------------------------------------------------------------------------

NO_COST = -2  # CardArt.costs of an Unplayable card: no energy gem and no digit


@dataclass(frozen=True)
class CardArt:
    enum_name: str   # engine CardId name (lower case)
    # Hero owning the card (frame colour). "hero" = the ROM's own hero, one art file per cartridge.
    # SHARED_OWNERS are on every ROM with a single art file: "status" and "colorless" (grey frame),
    # "curse" (purple frame), "ancient" (event cards of Ancient rarity: full-card art and the Ancient border).
    owner: str
    art: str         # art file relative to bible/images/cards/<folder>/ ; "{hero}" is substituted
    kind: str        # attack | skill | power | status
    costs: tuple[int, int]  # base cost, upgraded cost; -1 = X, NO_COST = unplayable
    stars: tuple[int, int] = (0, 0)  # Star cost (Regent), base and upgraded; -1 = X
    folder: str = ""  # art folder when it is not the owner's (tokens)


# Owners whose cards look the same on every cartridge: no hero colour in the frame, the gem or the mini card.
SHARED_OWNERS = ("status", "curse", "colorless", "ancient")

# Mirrors engine::CardId order (card.h). Index 0 (NONE) is a blank card.
CARDS: list[CardArt] = [
    CardArt("none", "hero", "", "skill", (0, 0)),
    CardArt("strike", "hero", "strike_{hero}", "attack", (1, 1)),
    CardArt("defend", "hero", "defend_{hero}", "skill", (1, 1)),
    CardArt("bash", "ironclad", "bash", "attack", (2, 2)),
    CardArt("iron_wave", "ironclad", "iron_wave", "attack", (1, 1)),
    CardArt("cleave", "ironclad", "thunderclap", "attack", (1, 1)),
    CardArt("anger", "ironclad", "anger", "attack", (0, 0)),
    CardArt("metallicize", "ironclad", "stone_armor", "power", (1, 1)),
    CardArt("neutralize", "silent", "neutralize", "attack", (0, 0)),
    CardArt("survivor", "silent", "survivor", "skill", (1, 1)),
    CardArt("poison_stab", "silent", "poisoned_stab", "attack", (1, 1)),
    CardArt("deadly_poison", "silent", "deadly_poison", "skill", (1, 1)),
    CardArt("zap", "defect", "zap", "skill", (1, 0)),
    CardArt("dualcast", "defect", "dualcast", "skill", (1, 0)),
    CardArt("falling_star", "regent", "falling_star", "attack", (0, 0), (2, 2)),
    CardArt("venerate", "regent", "venerate", "skill", (1, 1)),
    CardArt("unleash", "necrobinder", "unleash", "attack", (1, 1)),
    CardArt("bodyguard", "necrobinder", "bodyguard", "skill", (1, 1)),
    CardArt("twin_strike", "ironclad", "twin_strike", "attack", (1, 1)),
    CardArt("pommel_strike", "ironclad", "pommel_strike", "attack", (1, 1)),
    CardArt("shrug_it_off", "ironclad", "shrug_it_off", "skill", (1, 1)),
    CardArt("taunt", "ironclad", "taunt", "skill", (1, 1)),
    CardArt("slice", "silent", "slice", "attack", (0, 0)),
    CardArt("backflip", "silent", "backflip", "skill", (1, 1)),
    CardArt("sucker_punch", "silent", "sucker_punch", "attack", (1, 1)),
    CardArt("dagger_spray", "silent", "dagger_spray", "attack", (1, 1)),
    CardArt("beam_cell", "defect", "beam_cell", "attack", (0, 0)),
    CardArt("ball_lightning", "defect", "ball_lightning", "attack", (1, 1)),
    CardArt("cold_snap", "defect", "cold_snap", "attack", (1, 1)),
    CardArt("leap", "defect", "leap", "skill", (1, 1)),
    CardArt("solar_strike", "regent", "solar_strike", "attack", (1, 1)),
    CardArt("gather_light", "regent", "gather_light", "skill", (1, 1)),
    CardArt("celestial_might", "regent", "celestial_might", "attack", (2, 2)),
    CardArt("pull_aggro", "necrobinder", "pull_aggro", "skill", (2, 2)),
    CardArt("drain_power", "necrobinder", "drain_power", "attack", (1, 1)),
    CardArt("body_slam", "ironclad", "body_slam", "attack", (1, 0)),
    CardArt("deflect", "silent", "deflect", "skill", (0, 0)),
    CardArt("coolheaded", "defect", "coolheaded", "skill", (1, 1)),
    CardArt("sweeping_beam", "defect", "sweeping_beam", "attack", (1, 1)),
    CardArt("thunderclap", "ironclad", "thunderclap", "attack", (1, 1)),
    CardArt("dismantle", "ironclad", "dismantle", "attack", (1, 1)),
    CardArt("inflame", "ironclad", "inflame", "power", (1, 1)),
    CardArt("perfected_strike", "ironclad", "perfected_strike", "attack", (2, 2)),
    CardArt("bludgeon", "ironclad", "bludgeon", "attack", (3, 3)),
    CardArt("bully", "ironclad", "bully", "attack", (0, 0)),
    CardArt("sword_boomerang", "ironclad", "sword_boomerang", "attack", (1, 1)),
    CardArt("break", "ironclad", "break", "attack", (1, 1)),
    CardArt("conflagration", "ironclad", "conflagration", "attack", (1, 1)),
    CardArt("expect_a_fight", "ironclad", "expect_a_fight", "skill", (3, 3)),
    CardArt("pillage", "ironclad", "pillage", "attack", (1, 1)),
    CardArt("uppercut", "ironclad", "uppercut", "attack", (2, 2)),
    CardArt("fight_me", "ironclad", "fight_me", "attack", (2, 2)),
    CardArt("leg_sweep", "silent", "leg_sweep", "skill", (2, 2)),
    CardArt("haze", "silent", "haze", "skill", (2, 2)),
    CardArt("precise_cut", "silent", "precise_cut", "attack", (0, 0)),
    CardArt("bouncing_flask", "silent", "bouncing_flask", "skill", (2, 2)),
    CardArt("flechettes", "silent", "flechettes", "attack", (1, 1)),
    CardArt("footwork", "silent", "footwork", "power", (1, 1)),
    CardArt("escape_plan", "silent", "escape_plan", "skill", (0, 0)),
    CardArt("dash", "silent", "dash", "attack", (2, 2)),
    CardArt("bubble_bubble", "silent", "bubble_bubble", "skill", (1, 1)),
    CardArt("outbreak", "silent", "outbreak", "skill", (3, 3)),
    CardArt("shadow_shield", "defect", "shadow_shield", "skill", (2, 2)),
    CardArt("meteor_strike", "defect", "meteor_strike", "attack", (5, 5)),
    CardArt("glacier", "defect", "glacier", "skill", (2, 2)),
    CardArt("ice_lance", "defect", "ice_lance", "attack", (3, 3)),
    CardArt("skim", "defect", "skim", "skill", (1, 1)),
    CardArt("all_for_one", "defect", "all_for_one", "attack", (2, 2)),
    CardArt("null_card", "defect", "null", "attack", (2, 2)),
    CardArt("go_for_the_eyes", "defect", "go_for_the_eyes", "attack", (0, 0)),
    CardArt("scrape", "defect", "scrape", "attack", (1, 1)),
    CardArt("bury", "necrobinder", "bury", "attack", (4, 4)),
    CardArt("misery", "necrobinder", "misery", "attack", (0, 0)),
    CardArt("undeath", "necrobinder", "undeath", "skill", (0, 0)),
    CardArt("ashen_strike", "ironclad", "ashen_strike", "attack", (1, 1)),
    CardArt("tremble", "ironclad", "tremble", "skill", (1, 1)),
    CardArt("evil_eye", "ironclad", "evil_eye", "skill", (1, 1)),
    CardArt("molten_fist", "ironclad", "molten_fist", "attack", (1, 1)),
    CardArt("cinder", "ironclad", "cinder", "attack", (2, 2)),
    CardArt("second_wind", "ironclad", "second_wind", "skill", (1, 1)),
    CardArt("true_grit", "ironclad", "true_grit", "skill", (1, 1)),
    CardArt("dominate", "ironclad", "dominate", "skill", (1, 1)),
    CardArt("fiend_fire", "ironclad", "fiend_fire", "attack", (2, 2)),
    CardArt("thrash", "ironclad", "thrash", "attack", (1, 1)),
    CardArt("impervious", "ironclad", "impervious", "skill", (2, 2)),
    CardArt("pacts_end", "ironclad", "pacts_end", "attack", (0, 0)),
    CardArt("not_yet", "ironclad", "not_yet", "skill", (2, 2)),
    CardArt("adrenaline", "silent", "adrenaline", "skill", (0, 0)),
    CardArt("calculated_gamble", "silent", "calculated_gamble", "skill", (0, 0)),
    CardArt("mirage", "silent", "mirage", "skill", (1, 1)),
    CardArt("rainbow", "defect", "rainbow", "skill", (2, 2)),
    CardArt("chill", "defect", "chill", "skill", (0, 0)),
    CardArt("fusion", "defect", "fusion", "skill", (1, 1)),
    CardArt("reboot", "defect", "reboot", "skill", (0, 0)),
    CardArt("supercritical", "defect", "supercritical", "skill", (0, 0)),
    CardArt("double_energy", "defect", "double_energy", "skill", (1, 0)),
    CardArt("shared_fate", "necrobinder", "shared_fate", "skill", (0, 0)),
    CardArt("wisp", "necrobinder", "wisp", "skill", (0, 0)),
    CardArt("putrefy", "necrobinder", "putrefy", "skill", (1, 1)),
    CardArt("reanimate", "necrobinder", "reanimate", "skill", (3, 3)),
    CardArt("afterlife", "necrobinder", "afterlife", "skill", (1, 1)),
    CardArt("blood_wall", "ironclad", "blood_wall", "skill", (2, 2)),
    CardArt("bloodletting", "ironclad", "bloodletting", "skill", (0, 0)),
    CardArt("breakthrough", "ironclad", "breakthrough", "attack", (1, 1)),
    CardArt("forgotten_ritual", "ironclad", "forgotten_ritual", "skill", (1, 1)),
    CardArt("hemokinesis", "ironclad", "hemokinesis", "attack", (1, 1)),
    CardArt("mangle", "ironclad", "mangle", "attack", (3, 3)),
    CardArt("offering", "ironclad", "offering", "skill", (0, 0)),
    CardArt("setup_strike", "ironclad", "setup_strike", "attack", (1, 1)),
    CardArt("spite", "ironclad", "spite", "attack", (0, 0)),
    CardArt("whirlwind", "ironclad", "whirlwind", "attack", (-1, -1)),
    CardArt("anticipate", "silent", "anticipate", "skill", (0, 0)),
    CardArt("assassinate", "silent", "assassinate", "attack", (0, 0)),
    CardArt("backstab", "silent", "backstab", "attack", (0, 0)),
    CardArt("blur", "silent", "blur", "skill", (1, 1)),
    CardArt("dodge_and_roll", "silent", "dodge_and_roll", "skill", (1, 1)),
    CardArt("finisher", "silent", "finisher", "attack", (1, 1)),
    CardArt("malaise", "silent", "malaise", "skill", (-1, -1)),
    CardArt("memento_mori", "silent", "memento_mori", "attack", (1, 1)),
    CardArt("murder", "silent", "murder", "attack", (3, 2)),
    CardArt("piercing_wail", "silent", "piercing_wail", "skill", (1, 1)),
    CardArt("predator", "silent", "predator", "attack", (2, 2)),
    CardArt("sidestep", "silent", "sidestep", "skill", (0, 0)),
    CardArt("skewer", "silent", "skewer", "attack", (-1, -1)),
    CardArt("suppress", "silent", "suppress", "attack", (0, 0)),
    CardArt("expertise", "silent", "expertise", "skill", (1, 1)),
    CardArt("snakebite", "silent", "snakebite", "skill", (2, 2)),
    CardArt("boot_sequence", "defect", "boot_sequence", "skill", (0, 0)),
    CardArt("charge_battery", "defect", "charge_battery", "skill", (1, 1)),
    CardArt("ftl", "defect", "ftl", "attack", (0, 0)),
    CardArt("helix_drill", "defect", "helix_drill", "attack", (0, 0)),
    CardArt("lightning_rod", "defect", "lightning_rod", "skill", (1, 1)),
    CardArt("tempest", "defect", "tempest", "skill", (-1, -1)),
    CardArt("crush_under", "regent", "crush_under", "attack", (1, 1)),
    CardArt("glitterstream", "regent", "glitterstream", "skill", (2, 2)),
    CardArt("heavenly_drill", "regent", "heavenly_drill", "attack", (-1, -1)),
    CardArt("hegemony", "regent", "hegemony", "attack", (2, 2)),
    CardArt("know_thy_place", "regent", "know_thy_place", "skill", (0, 0)),
    CardArt("lunar_blast", "regent", "lunar_blast", "attack", (0, 0)),
    CardArt("prophesize", "regent", "prophesize", "skill", (2, 2)),
    CardArt("supermassive", "regent", "supermassive", "attack", (1, 1)),
    CardArt("convergence", "regent", "convergence", "skill", (1, 1)),
    CardArt("death_march", "necrobinder", "death_march", "attack", (1, 1)),
    CardArt("defile", "necrobinder", "defile", "attack", (1, 1)),
    CardArt("defy", "necrobinder", "defy", "skill", (1, 1)),
    CardArt("delay", "necrobinder", "delay", "skill", (2, 2)),
    CardArt("enfeebling_touch", "necrobinder", "enfeebling_touch", "skill", (1, 1)),
    CardArt("fear", "necrobinder", "fear", "attack", (1, 1)),
    CardArt("invoke", "necrobinder", "invoke", "skill", (1, 1)),
    CardArt("parse", "necrobinder", "parse", "skill", (1, 1)),
    CardArt("pull_from_below", "necrobinder", "pull_from_below", "attack", (1, 1)),
    CardArt("reap", "necrobinder", "reap", "attack", (3, 3)),
    CardArt("sow", "necrobinder", "sow", "attack", (1, 1)),
    CardArt("eradicate", "necrobinder", "eradicate", "attack", (-1, -1)),
    CardArt("alignment", "regent", "alignment", "skill", (0, 0), (2, 2)),
    CardArt("astral_pulse", "regent", "astral_pulse", "attack", (0, 0), (3, 3)),
    CardArt("cloak_of_stars", "regent", "cloak_of_stars", "skill", (0, 0), (1, 1)),
    CardArt("comet", "regent", "comet", "attack", (0, 0), (5, 5)),
    CardArt("crescent_spear", "regent", "crescent_spear", "attack", (1, 1), (1, 1)),
    CardArt("devastate", "regent", "devastate", "attack", (1, 1), (4, 4)),
    CardArt("dying_star", "regent", "dying_star", "attack", (1, 1), (3, 3)),
    CardArt("gamma_blast", "regent", "gamma_blast", "attack", (0, 0), (3, 3)),
    CardArt("glow", "regent", "glow", "skill", (1, 1)),
    CardArt("guiding_star", "regent", "guiding_star", "attack", (1, 1), (1, 1)),
    CardArt("hidden_cache", "regent", "hidden_cache", "skill", (1, 1)),
    CardArt("meteor_shower", "regent", "meteor_shower", "attack", (0, 0), (2, 2)),
    CardArt("particle_wall", "regent", "particle_wall", "skill", (0, 0), (2, 2)),
    CardArt("radiate", "regent", "radiate", "attack", (0, 0)),
    CardArt("resonance", "regent", "resonance", "skill", (1, 1), (2, 2)),
    CardArt("royal_gamble", "regent", "royal_gamble", "skill", (0, 0), (5, 5)),
    CardArt("seven_stars", "regent", "seven_stars", "attack", (2, 1), (7, 7)),
    CardArt("shining_strike", "regent", "shining_strike", "attack", (1, 1)),
    CardArt("stardust", "regent", "stardust", "attack", (0, 0), (-1, -1)),
    CardArt("feed", "ironclad", "feed", "attack", (1, 1)),
    CardArt("battle_trance", "ironclad", "battle_trance", "skill", (0, 0)),
    CardArt("echoing_slash", "silent", "echoing_slash", "attack", (1, 1)),
    CardArt("grand_finale", "silent", "grand_finale", "attack", (0, 0)),
    CardArt("sunder", "defect", "sunder", "attack", (3, 3)),
    CardArt("knockout_blow", "regent", "knockout_blow", "attack", (3, 3)),
    CardArt("feel_no_pain", "ironclad", "feel_no_pain", "power", (1, 1)),
    CardArt("dark_embrace", "ironclad", "dark_embrace", "power", (2, 1)),
    CardArt("drum_of_battle", "ironclad", "drum_of_battle", "skill", (1, 1)),
    CardArt("barricade", "ironclad", "barricade", "power", (3, 2)),
    CardArt("inferno", "ironclad", "inferno", "power", (1, 1)),
    CardArt("crimson_mantle", "ironclad", "crimson_mantle", "power", (1, 1)),
    CardArt("rupture", "ironclad", "rupture", "power", (1, 1)),
    CardArt("aggression", "ironclad", "aggression", "power", (1, 1)),
    CardArt("pyre", "ironclad", "pyre", "power", (2, 2)),
    CardArt("demon_form", "ironclad", "demon_form", "power", (3, 3)),
    CardArt("juggernaut", "ironclad", "juggernaut", "power", (2, 2)),
    CardArt("rage", "ironclad", "rage", "skill", (0, 0)),
    CardArt("vicious", "ironclad", "vicious", "power", (1, 1)),
    CardArt("accelerant", "silent", "accelerant", "power", (1, 1)),
    CardArt("speedster", "silent", "speedster", "power", (2, 2)),
    CardArt("envenom", "silent", "envenom", "power", (2, 2)),
    CardArt("serpent_form", "silent", "serpent_form", "power", (3, 3)),
    CardArt("afterimage", "silent", "afterimage", "power", (1, 1)),
    CardArt("well_laid_plans", "silent", "well_laid_plans", "power", (2, 1)),
    CardArt("strangle", "silent", "strangle", "attack", (1, 1)),
    CardArt("corrosive_wave", "silent", "corrosive_wave", "skill", (1, 1)),
    CardArt("noxious_fumes", "silent", "noxious_fumes", "power", (1, 1)),
    CardArt("machine_learning", "defect", "machine_learning", "power", (1, 1)),
    CardArt("storm", "defect", "storm", "power", (1, 1)),
    CardArt("feral", "defect", "feral", "power", (2, 1)),
    CardArt("subroutine", "defect", "subroutine", "power", (1, 0)),
    CardArt("child_of_the_stars", "regent", "child_of_the_stars", "power", (1, 1)),
    CardArt("genesis", "regent", "genesis", "power", (2, 2)),
    CardArt("the_sealed_throne", "regent", "the_sealed_throne", "power", (1, 0), (3, 3)),
    CardArt("black_hole", "regent", "black_hole", "power", (1, 1)),
    CardArt("make_it_so", "regent", "make_it_so", "attack", (0, 0)),
    CardArt("pale_blue_dot", "regent", "pale_blue_dot", "power", (1, 1)),
    CardArt("arsenal", "regent", "arsenal", "power", (1, 1)),
    CardArt("monologue", "regent", "monologue", "skill", (0, 0)),
    CardArt("orbit", "regent", "orbit", "power", (2, 1)),
    CardArt("pillar_of_creation", "regent", "pillar_of_creation", "power", (1, 1)),
    CardArt("monarchs_gaze", "regent", "monarchs_gaze", "power", (2, 1)),
    CardArt("friendship", "necrobinder", "friendship", "power", (1, 1)),
    CardArt("danse_macabre", "necrobinder", "danse_macabre", "power", (1, 1)),
    CardArt("demesne", "necrobinder", "demesne", "power", (3, 2)),
    CardArt("spirit_of_ash", "necrobinder", "spirit_of_ash", "power", (1, 1)),
    CardArt("pagestorm", "necrobinder", "pagestorm", "power", (1, 0)),
    CardArt("sleight_of_flesh", "necrobinder", "sleight_of_flesh", "power", (2, 2)),
    CardArt("shatter", "defect", "shatter", "attack", (1, 1)),
    CardArt("voltaic", "defect", "voltaic", "skill", (3, 3)),
    CardArt("quadcast", "defect", "quadcast", "skill", (1, 0)),
    CardArt("barrage", "defect", "barrage", "attack", (1, 1)),
    CardArt("tesla_coil", "defect", "tesla_coil", "attack", (0, 0)),
    CardArt("chaos", "defect", "chaos", "skill", (1, 1)),
    CardArt("compile_driver", "defect", "compile_driver", "attack", (1, 1)),
    CardArt("darkness", "defect", "darkness", "skill", (1, 1)),
    CardArt("multi_cast", "defect", "multi_cast", "skill", (-1, -1)),
    CardArt("loop", "defect", "loop", "power", (1, 1)),
    CardArt("consuming_shadow", "defect", "consuming_shadow", "power", (2, 2)),
    CardArt("hailstorm", "defect", "hailstorm", "power", (1, 1)),
    CardArt("thunder", "defect", "thunder", "power", (1, 1)),
    CardArt("coolant", "defect", "coolant", "power", (1, 1)),
    CardArt("biased_cognition", "defect", "biased_cognition", "power", (1, 1)),
    CardArt("hotfix", "defect", "hotfix", "skill", (0, 0)),
    CardArt("defragment", "defect", "defragment", "power", (1, 1)),
    CardArt("focused_strike", "defect", "focused_strike", "attack", (1, 1)),
    CardArt("hyperbeam", "defect", "hyperbeam", "attack", (2, 2)),
    CardArt("negative_pulse", "necrobinder", "negative_pulse", "skill", (1, 1)),
    CardArt("end_of_days", "necrobinder", "end_of_days", "skill", (3, 3)),
    CardArt("blight_strike", "necrobinder", "blight_strike", "attack", (1, 1)),
    CardArt("no_escape", "necrobinder", "no_escape", "skill", (1, 1)),
    CardArt("scourge", "necrobinder", "scourge", "skill", (1, 1)),
    CardArt("deathbringer", "necrobinder", "deathbringer", "skill", (2, 2)),
    CardArt("deaths_door", "necrobinder", "deaths_door", "skill", (1, 1)),
    CardArt("countdown", "necrobinder", "countdown", "power", (1, 1)),
    CardArt("reaper_form", "necrobinder", "reaper_form", "power", (3, 3)),
    CardArt("neurosurge", "necrobinder", "neurosurge", "power", (0, 0)),
    CardArt("times_up", "necrobinder", "times_up", "attack", (2, 2)),
    CardArt("oblivion", "necrobinder", "oblivion", "skill", (0, 0)),
    CardArt("shroud", "necrobinder", "shroud", "power", (1, 1)),
    CardArt("capacitor", "defect", "capacitor", "power", (1, 1)),
    CardArt("bulk_up", "defect", "bulk_up", "power", (2, 2)),
    CardArt("synchronize", "defect", "synchronize", "skill", (1, 1)),
    CardArt("howl_from_beyond", "ironclad", "howl_from_beyond", "attack", (3, 3)),
    CardArt("hellraiser", "ironclad", "hellraiser", "power", (2, 1)),
    CardArt("stampede", "ironclad", "stampede", "power", (2, 1)),
    CardArt("cascade", "ironclad", "cascade", "skill", (-1, -1)),
    CardArt("one_two_punch", "ironclad", "one_two_punch", "skill", (1, 1)),
    CardArt("tear_asunder", "ironclad", "tear_asunder", "attack", (2, 2)),
    CardArt("havoc", "ironclad", "havoc", "skill", (1, 0)),
    CardArt("corruption", "ironclad", "corruption", "power", (3, 2)),
    CardArt("unrelenting", "ironclad", "unrelenting", "attack", (2, 2)),
    CardArt("midnight", "ironclad", "midnight", "attack", (12, 12)),
    CardArt("stomp", "ironclad", "stomp", "attack", (3, 3)),
    CardArt("rampage", "ironclad", "rampage", "attack", (1, 1)),
    CardArt("burst", "silent", "burst", "skill", (1, 1)),
    CardArt("bullet_time", "silent", "bullet_time", "skill", (3, 2)),
    CardArt("pinpoint", "silent", "pinpoint", "attack", (3, 3)),
    CardArt("pounce", "silent", "pounce", "attack", (2, 2)),
    CardArt("echo_form", "defect", "echo_form", "power", (3, 3)),
    CardArt("signal_boost", "defect", "signal_boost", "skill", (1, 0)),
    CardArt("uproar", "defect", "uproar", "attack", (2, 2)),
    CardArt("adaptive_strike", "defect", "adaptive_strike", "attack", (2, 2)),
    CardArt("synthesis", "defect", "synthesis", "attack", (2, 2)),
    CardArt("genetic_algorithm", "defect", "genetic_algorithm", "skill", (1, 1)),
    CardArt("momentum_strike", "defect", "momentum_strike", "attack", (1, 1)),
    CardArt("claw", "defect", "claw", "attack", (0, 0)),
    CardArt("modded", "defect", "modded", "skill", (0, 0)),
    CardArt("bombardment", "regent", "bombardment", "attack", (3, 3)),
    CardArt("i_am_invincible", "regent", "i_am_invincible", "skill", (1, 1)),
    CardArt("void_form", "regent", "void_form", "power", (3, 3)),
    CardArt("kingly_kick", "regent", "kingly_kick", "attack", (4, 4)),
    CardArt("kingly_punch", "regent", "kingly_punch", "attack", (1, 1)),
    CardArt("eidolon", "necrobinder", "eidolon", "skill", (2, 1)),
    CardArt("banshees_cry", "necrobinder", "banshees_cry", "attack", (9, 7)),
    CardArt("veilpiercer", "necrobinder", "veilpiercer", "attack", (1, 1)),
    CardArt("borrowed_time", "necrobinder", "borrowed_time", "skill", (1, 1)),
    CardArt("the_scythe", "necrobinder", "the_scythe", "attack", (2, 2)),
    CardArt("melancholy", "necrobinder", "melancholy", "skill", (3, 3)),
    CardArt("hang", "necrobinder", "hang", "attack", (1, 1)),
    CardArt("burn", "status", "burn", "status", (NO_COST, NO_COST)),
    CardArt("dazed", "status", "dazed", "status", (NO_COST, NO_COST)),
    CardArt("debris", "status", "debris", "status", (1, 1)),
    CardArt("slimed", "status", "slimed", "status", (1, 1)),
    CardArt("soot", "status", "soot", "status", (NO_COST, NO_COST)),
    CardArt("void_card", "status", "void", "status", (NO_COST, NO_COST)),
    CardArt("wound", "status", "wound", "status", (NO_COST, NO_COST)),
    CardArt("fuel", "defect", "fuel", "skill", (0, 0), folder="token"),
    CardArt("boost_away", "defect", "boost_away", "skill", (0, 0)),
    CardArt("flak_cannon", "defect", "flak_cannon", "attack", (2, 2)),
    CardArt("smokestack", "defect", "smokestack", "power", (1, 1)),
    CardArt("compact", "defect", "compact", "skill", (1, 1)),
    CardArt("gunk_up", "defect", "gunk_up", "attack", (1, 1)),
    CardArt("iteration", "defect", "iteration", "power", (1, 1)),
    CardArt("fight_through", "defect", "fight_through", "skill", (1, 1)),
    CardArt("overclock", "defect", "overclock", "skill", (0, 0)),
    CardArt("rocket_punch", "defect", "rocket_punch", "attack", (2, 2)),
    CardArt("trash_to_treasure", "defect", "trash_to_treasure", "power", (1, 0)),
    CardArt("turbo", "defect", "turbo", "skill", (0, 0)),
    CardArt("collision_course", "regent", "collision_course", "attack", (0, 0)),
    CardArt("crash_landing", "regent", "crash_landing", "attack", (1, 1)),
    CardArt("shiv", "silent", "shiv", "attack", (0, 0), folder="token"),
    CardArt("blade_dance", "silent", "blade_dance", "skill", (1, 1)),
    CardArt("cloak_and_dagger", "silent", "cloak_and_dagger", "skill", (1, 1)),
    CardArt("leading_strike", "silent", "leading_strike", "attack", (1, 1)),
    CardArt("blade_of_ink", "silent", "blade_of_ink", "skill", (1, 1)),
    CardArt("infinite_blades", "silent", "infinite_blades", "power", (1, 1)),
    CardArt("storm_of_steel", "silent", "storm_of_steel", "skill", (1, 1)),
    CardArt("fan_of_knives", "silent", "fan_of_knives", "power", (2, 2)),
    CardArt("phantom_blades", "silent", "phantom_blades", "power", (1, 1)),
    CardArt("accuracy", "silent", "accuracy", "power", (1, 1)),
    CardArt("up_my_sleeve", "silent", "up_my_sleeve", "skill", (2, 2)),
    CardArt("knife_trap", "silent", "knife_trap", "skill", (2, 2)),
    CardArt("armaments", "ironclad", "armaments", "skill", (1, 1)),
    CardArt("brand", "ironclad", "brand", "skill", (0, 0)),
    CardArt("burning_pact", "ironclad", "burning_pact", "skill", (1, 1)),
    CardArt("acrobatics", "silent", "acrobatics", "skill", (1, 1)),
    CardArt("dagger_throw", "silent", "dagger_throw", "attack", (1, 1)),
    CardArt("prepared", "silent", "prepared", "skill", (0, 0)),
    CardArt("hidden_daggers", "silent", "hidden_daggers", "skill", (0, 0)),
    CardArt("scavenge", "defect", "scavenge", "skill", (1, 1)),
    CardArt("photon_cut", "regent", "photon_cut", "attack", (1, 1)),
    CardArt("glimmer", "regent", "glimmer", "skill", (1, 1)),
    CardArt("tyranny", "regent", "tyranny", "power", (1, 1)),
    CardArt("sculpting_strike", "necrobinder", "sculpting_strike", "attack", (1, 1)),
    CardArt("juggling", "ironclad", "juggling", "power", (1, 1)),
    CardArt("expose", "silent", "expose", "skill", (0, 0)),
    CardArt("headbutt", "ironclad", "headbutt", "attack", (1, 1)),
    CardArt("hologram", "defect", "hologram", "skill", (1, 1)),
    CardArt("cosmic_indifference", "regent", "cosmic_indifference", "skill", (1, 1)),
    CardArt("foregone_conclusion", "regent", "foregone_conclusion", "skill", (1, 1)),
    CardArt("graveblast", "necrobinder", "graveblast", "attack", (1, 1)),
    CardArt("dredge", "necrobinder", "dredge", "skill", (1, 1)),
    CardArt("cleanse", "necrobinder", "cleanse", "skill", (1, 1)),
    CardArt("stone_armor", "ironclad", "stone_armor", "power", (1, 1)),
    CardArt("flame_barrier", "ironclad", "flame_barrier", "skill", (2, 2)),
    CardArt("wraith_form", "silent", "wraith_form", "power", (3, 3)),
    CardArt("buffer", "defect", "buffer", "power", (2, 2)),
    CardArt("neutron_aegis", "regent", "neutron_aegis", "power", (1, 1)),
    CardArt("reflect", "regent", "reflect", "skill", (1, 1)),
    CardArt("patter", "regent", "patter", "skill", (1, 1)),
    CardArt("terraforming", "regent", "terraforming", "skill", (1, 1)),
    CardArt("sovereign_blade", "regent", "sovereign_blade", "attack", (2, 2), folder="token"),
    CardArt("refine_blade", "regent", "refine_blade", "skill", (1, 1)),
    CardArt("spoils_of_battle", "regent", "spoils_of_battle", "skill", (1, 1)),
    CardArt("wrought_in_war", "regent", "wrought_in_war", "attack", (1, 1)),
    CardArt("bulwark", "regent", "bulwark", "skill", (2, 2)),
    CardArt("conqueror", "regent", "conqueror", "skill", (1, 1)),
    CardArt("furnace", "regent", "furnace", "power", (1, 1)),
    CardArt("parry", "regent", "parry", "power", (1, 1)),
    CardArt("summon_forth", "regent", "summon_forth", "skill", (1, 1)),
    CardArt("beat_into_shape", "regent", "beat_into_shape", "attack", (1, 1)),
    CardArt("big_bang", "regent", "big_bang", "skill", (0, 0)),
    CardArt("seeking_edge", "regent", "seeking_edge", "power", (1, 1)),
    CardArt("sword_sage", "regent", "sword_sage", "power", (2, 1)),
    CardArt("the_smith", "regent", "the_smith", "skill", (1, 1), stars=(4, 4)),
    CardArt("poke", "necrobinder", "poke", "attack", (0, 0)),
    CardArt("snap", "necrobinder", "snap", "attack", (1, 1)),
    CardArt("rattle", "necrobinder", "rattle", "attack", (1, 1)),
    CardArt("bone_shards", "necrobinder", "bone_shards", "attack", (1, 1)),
    CardArt("squeeze", "necrobinder", "squeeze", "attack", (3, 3)),
    CardArt("flatten", "necrobinder", "flatten", "attack", (2, 2)),
    CardArt("high_five", "necrobinder", "high_five", "attack", (2, 2)),
    CardArt("fetch", "necrobinder", "fetch", "attack", (0, 0)),
    CardArt("calcify", "necrobinder", "calcify", "power", (1, 1)),
    CardArt("right_hand_hand", "necrobinder", "right_hand_hand", "attack", (0, 0)),
    CardArt("necro_mastery", "necrobinder", "necro_mastery", "power", (2, 2)),
    CardArt("protector", "necrobinder", "protector", "attack", (1, 0)),
    CardArt("sacrifice", "necrobinder", "sacrifice", "skill", (1, 0)),
    CardArt("spur", "necrobinder", "spur", "skill", (1, 1)),
    CardArt("sic_em", "necrobinder", "sic_em", "attack", (1, 1)),
    CardArt("sentry_mode", "necrobinder", "sentry_mode", "power", (2, 1)),
    CardArt("sweeping_gaze", "necrobinder", "sweeping_gaze", "attack", (0, 0), folder="token"),
    CardArt("soul", "necrobinder", "soul", "skill", (0, 0), folder="token"),
    CardArt("reave", "necrobinder", "reave", "attack", (1, 1)),
    CardArt("grave_warden", "necrobinder", "grave_warden", "skill", (1, 1)),
    CardArt("capture_spirit", "necrobinder", "capture_spirit", "skill", (1, 1)),
    CardArt("severance", "necrobinder", "severance", "attack", (2, 2)),
    CardArt("dirge", "necrobinder", "dirge", "skill", (-1, -1)),
    CardArt("haunt", "necrobinder", "haunt", "power", (1, 1)),
    CardArt("devour_life", "necrobinder", "devour_life", "power", (1, 1)),
    CardArt("soul_storm", "necrobinder", "soul_storm", "attack", (1, 1)),
    CardArt("glimpse_beyond", "necrobinder", "glimpse_beyond", "skill", (1, 1)),
    CardArt("manifest_authority", "regent", "manifest_authority", "skill", (1, 1)),
    CardArt("giant_rock", "hero", "giant_rock", "attack", (1, 1), folder="token"),
    CardArt("luminesce", "hero", "luminesce", "skill", (0, 0), folder="token"),
    CardArt("apparition", "ancient", "apparition", "skill", (1, 1), folder="event"),
    CardArt("maul", "ancient", "maul", "attack", (1, 1), folder="event"),
    CardArt("apotheosis", "ancient", "apotheosis", "skill", (2, 1), folder="event"),
    CardArt("wish", "ancient", "wish", "skill", (0, 0), folder="event"),
    CardArt("whistle", "ancient", "whistle", "attack", (2, 2), folder="event"),
    CardArt("brightest_flame", "ancient", "brightest_flame", "skill", (0, 0), folder="event"),
    CardArt("folly", "curse", "folly", "status", (NO_COST, NO_COST)),
    CardArt("enthralled", "curse", "enthralled", "status", (2, 2)),
    CardArt("flash_of_steel", "colorless", "flash_of_steel", "attack", (0, 0)),
    CardArt("peck", "hero", "peck", "attack", (1, 1), folder="event"),
    CardArt("production", "colorless", "production", "skill", (0, 0)),
    CardArt("master_of_strategy", "colorless", "master_of_strategy", "skill", (0, 0)),
    CardArt("byrd_swoop", "hero", "byrd_swoop", "attack", (0, 0), folder="event"),
    CardArt("ultimate_strike", "colorless", "ultimate_strike", "attack", (1, 1)),
    CardArt("ultimate_defend", "colorless", "ultimate_defend", "skill", (1, 1)),
    CardArt("finesse", "colorless", "finesse", "skill", (0, 0)),
    CardArt("prowess", "colorless", "prowess", "power", (1, 1)),
    CardArt("squash", "hero", "squash", "attack", (1, 1), folder="event"),
    CardArt("exterminate", "hero", "exterminate", "attack", (1, 1), folder="event"),
    CardArt("feeding_frenzy", "hero", "feeding_frenzy", "skill", (0, 0), folder="event"),
    CardArt("outmaneuver", "hero", "outmaneuver", "skill", (1, 1), folder="event"),
    CardArt("toric_toughness", "hero", "toric_toughness", "skill", (2, 2), folder="event"),
    CardArt("entrench", "hero", "entrench", "skill", (2, 1), folder="event"),
    CardArt("stack", "hero", "stack", "skill", (1, 1), folder="event"),
    CardArt("rip_and_tear", "hero", "rip_and_tear", "attack", (1, 1), folder="event"),
    CardArt("scrawl", "colorless", "scrawl", "skill", (1, 1)),
    CardArt("mind_blast", "colorless", "mind_blast", "attack", (1, 0)),
    CardArt("shockwave", "colorless", "shockwave", "skill", (2, 2)),
    CardArt("bolas", "colorless", "bolas", "attack", (0, 0)),
    CardArt("thrumming_hatchet", "colorless", "thrumming_hatchet", "attack", (1, 1)),
    CardArt("impatience", "colorless", "impatience", "skill", (0, 0)),
    CardArt("clash", "hero", "clash", "attack", (0, 0), folder="event"),
    CardArt("volley", "colorless", "volley", "attack", (-1, -1)),
    CardArt("omnislice", "colorless", "omnislice", "attack", (0, 0)),
    CardArt("restlessness", "colorless", "restlessness", "skill", (0, 0)),
    CardArt("eternal_armor", "colorless", "eternal_armor", "power", (3, 3)),
    CardArt("fasten", "colorless", "fasten", "power", (1, 1)),
    CardArt("rebound", "hero", "rebound", "attack", (1, 1), folder="event"),
    CardArt("panic_button", "colorless", "panic_button", "skill", (0, 0)),
    CardArt("gold_axe", "colorless", "gold_axe", "attack", (1, 1)),
    CardArt("anointed", "colorless", "anointed", "skill", (1, 1)),
    CardArt("fisticuffs", "colorless", "fisticuffs", "attack", (1, 1)),
    CardArt("rend", "colorless", "rend", "attack", (1, 1)),
    CardArt("the_bomb", "colorless", "the_bomb", "skill", (2, 2)),
    CardArt("rolling_boulder", "colorless", "rolling_boulder", "power", (3, 3)),
    CardArt("panache", "colorless", "panache", "power", (0, 0)),
    CardArt("jackpot", "colorless", "jackpot", "attack", (3, 3)),
    CardArt("beat_down", "colorless", "beat_down", "skill", (3, 3)),
    CardArt("catastrophe", "colorless", "catastrophe", "skill", (2, 2)),
    CardArt("nostalgia", "colorless", "nostalgia", "power", (1, 0)),
    CardArt("relax", "ancient", "relax", "skill", (3, 3), folder="event"),
    CardArt("neows_fury", "ancient", "neows_fury", "attack", (1, 1), folder="event"),
    CardArt("metamorphosis", "hero", "metamorphosis", "skill", (2, 2), folder="event"),
    CardArt("distraction", "hero", "distraction", "skill", (1, 0), folder="event"),
    CardArt("caltrops", "hero", "caltrops", "power", (1, 1), folder="event"),
    CardArt("prolong", "colorless", "prolong", "skill", (0, 0)),
    CardArt("salvo", "colorless", "salvo", "attack", (1, 1)),
    CardArt("thinking_ahead", "colorless", "thinking_ahead", "skill", (0, 0)),
    CardArt("jack_of_all_trades", "colorless", "jack_of_all_trades", "skill", (0, 0)),
    CardArt("hello_world", "hero", "hello_world", "power", (1, 1), folder="event"),
    CardArt("dark_shackles", "colorless", "dark_shackles", "skill", (0, 0)),
    CardArt("equilibrium", "colorless", "equilibrium", "skill", (2, 2)),
    CardArt("purity", "colorless", "purity", "skill", (0, 0)),
    CardArt("secret_technique", "colorless", "secret_technique", "skill", (0, 0)),
    CardArt("dramatic_entrance", "colorless", "dramatic_entrance", "attack", (0, 0)),
    CardArt("secret_weapon", "colorless", "secret_weapon", "skill", (0, 0)),
    CardArt("mayhem", "colorless", "mayhem", "power", (2, 1)),
    CardArt("prep_time", "colorless", "prep_time", "power", (1, 1)),
    CardArt("stratagem", "colorless", "stratagem", "power", (1, 0)),
    CardArt("calamity", "colorless", "calamity", "power", (3, 2)),
    CardArt("automation", "colorless", "automation", "power", (1, 0)),
    CardArt("enlightenment", "hero", "enlightenment", "skill", (0, 0), folder="event"),
    CardArt("dual_wield", "hero", "dual_wield", "skill", (1, 1), folder="event"),
    CardArt("call_of_the_void", "necrobinder", "call_of_the_void", "power", (1, 1)),
    CardArt("heirloom_hammer", "regent", "heirloom_hammer", "attack", (2, 2)),
    CardArt("bundle_of_joy", "regent", "bundle_of_joy", "skill", (1, 1)),
    CardArt("spectrum_shift", "regent", "spectrum_shift", "power", (2, 1)),
    CardArt("decisions_decisions", "regent", "decisions_decisions", "skill", (0, 0), stars=(6, 6)),
    CardArt("white_noise", "defect", "white_noise", "skill", (1, 0)),
    CardArt("creative_ai", "defect", "creative_ai", "power", (3, 2)),
    CardArt("nightmare", "silent", "nightmare", "skill", (3, 2)),
    CardArt("tools_of_the_trade", "silent", "tools_of_the_trade", "power", (1, 0)),
    CardArt("infernal_blade", "ironclad", "infernal_blade", "skill", (1, 0)),
    CardArt("stoke", "ironclad", "stoke", "skill", (1, 1)),
    CardArt("injury", "curse", "injury", "status", (NO_COST, NO_COST)),
    CardArt("greed", "curse", "greed", "status", (NO_COST, NO_COST)),
    CardArt("clumsy", "curse", "clumsy", "status", (NO_COST, NO_COST)),
    CardArt("ascenders_bane", "curse", "ascenders_bane", "status", (NO_COST, NO_COST)),
    CardArt("curse_of_the_bell", "curse", "curse_of_the_bell", "status", (NO_COST, NO_COST)),
    CardArt("poor_sleep", "curse", "poor_sleep", "status", (NO_COST, NO_COST)),
    CardArt("writhe", "curse", "writhe", "status", (NO_COST, NO_COST)),
    CardArt("normality", "curse", "normality", "status", (NO_COST, NO_COST)),
    CardArt("doubt", "curse", "doubt", "status", (NO_COST, NO_COST)),
    CardArt("shame", "curse", "shame", "status", (NO_COST, NO_COST)),
    CardArt("decay", "curse", "decay", "status", (NO_COST, NO_COST)),
    CardArt("regret", "curse", "regret", "status", (NO_COST, NO_COST)),
    CardArt("bad_luck", "curse", "bad_luck", "status", (NO_COST, NO_COST)),
    CardArt("debt", "curse", "debt", "status", (NO_COST, NO_COST)),
    CardArt("spore_mind", "curse", "spore_mind", "status", (1, 1)),
    CardArt("guilty", "curse", "guilty", "status", (NO_COST, NO_COST)),
    CardArt("tactician", "silent", "tactician", "skill", (3, 3)),
    CardArt("abrasive", "silent", "abrasive", "power", (3, 3)),
    CardArt("master_planner", "silent", "master_planner", "power", (2, 1)),
    CardArt("hand_trick", "silent", "hand_trick", "skill", (1, 1)),
    CardArt("untouchable", "silent", "untouchable", "skill", (2, 2)),
    CardArt("ricochet", "silent", "ricochet", "attack", (2, 2)),
    CardArt("reflex", "silent", "reflex", "skill", (3, 3)),
    CardArt("flick_flack", "silent", "flick_flack", "attack", (1, 1)),
    CardArt("shadowmeld", "silent", "shadowmeld", "skill", (1, 0)),
    CardArt("unmovable", "ironclad", "unmovable", "power", (2, 1)),
    CardArt("colossus", "ironclad", "colossus", "skill", (1, 1)),
    CardArt("cruelty", "ironclad", "cruelty", "power", (1, 1)),
    CardArt("tracking", "silent", "tracking", "power", (2, 1)),
    CardArt("lethality", "necrobinder", "lethality", "power", (1, 1)),
    CardArt("shadow_step", "silent", "shadow_step", "skill", (1, 0)),
    CardArt("debilitate", "necrobinder", "debilitate", "attack", (1, 1)),
    CardArt("hand_of_greed", "colorless", "hand_of_greed", "attack", (2, 2)),
    CardArt("the_hunt", "silent", "the_hunt", "attack", (1, 1)),
    CardArt("royalties", "regent", "royalties", "power", (1, 1)),
    CardArt("forbidden_grimoire", "necrobinder", "forbidden_grimoire", "power", (2, 2)),
    CardArt("alchemize", "colorless", "alchemize", "skill", (1, 1)),
    CardArt("the_gambit", "colorless", "the_gambit", "skill", (0, 0)),
    CardArt("seeker_strike", "colorless", "seeker_strike", "attack", (1, 1)),
    CardArt("primal_force", "ironclad", "primal_force", "skill", (0, 0)),
    CardArt("seance", "necrobinder", "seance", "skill", (1, 0)),
    CardArt("entropy", "colorless", "entropy", "power", (1, 1)),
    CardArt("splash", "colorless", "splash", "skill", (1, 1)),
    CardArt("mad_science", "hero", "mad_science_attack", "attack", (1, 1), folder="event"),
    CardArt("quasar", "regent", "quasar", "skill", (0, 0), stars=(2, 2)),
    CardArt("discovery", "colorless", "discovery", "skill", (1, 1)),
    CardArt("abundance", "ancient", "abundance", "skill", (1, 0), folder="event"),
    CardArt("minion_strike", "regent", "minion_strike", "attack", (0, 0), folder="token"),
    CardArt("minion_dive_bomb", "regent", "minion_dive_bomb", "attack", (0, 0), folder="token"),
    CardArt("minion_sacrifice", "regent", "minion_sacrifice", "skill", (0, 0), folder="token"),
    CardArt("begone", "regent", "begone", "skill", (1, 1)),
    CardArt("charge", "regent", "charge", "skill", (1, 1)),
    CardArt("guards", "regent", "guards", "skill", (2, 2)),
    CardArt("hidden_gem", "colorless", "hidden_gem", "skill", (1, 1)),
    CardArt("transfigure", "necrobinder", "transfigure", "skill", (1, 1)),
    CardArt("glasswork", "defect", "glasswork", "skill", (1, 1)),
    CardArt("refract", "defect", "refract", "attack", (3, 3)),
    CardArt("spinner", "defect", "spinner", "power", (1, 1)),
    CardArt("infection", "status", "wound", "status", (NO_COST, NO_COST)),  # no portrait of its own in the reference
]

FRAME_COLOR = {"ironclad": "red", "silent": "green", "defect": "blue", "regent": "orange", "necrobinder": "pink",
               "status": "grey", "colorless": "grey", "curse": "purple", "ancient": "ancient"}
PORTRAIT_BOX = (47, 84, 547, 464)  # transparent window inside the 598x844 frame templates

# 3x5 digits baked into the mini cards' energy gem.
DIGITS = {
    "0": ["###", "#.#", "#.#", "#.#", "###"],
    "1": [".#.", "##.", ".#.", ".#.", "###"],
    "2": ["###", "..#", "###", "#..", "###"],
    "3": ["###", "..#", ".##", "..#", "###"],
    "4": ["#.#", "#.#", "###", "..#", "..#"],
    "5": ["###", "#..", "###", "..#", "###"],
    "6": ["###", "#..", "###", "#.#", "###"],
    "7": ["###", "..#", ".#.", ".#.", ".#."],
    "8": ["###", "#.#", "###", "#.#", "###"],
    "9": ["###", "#.#", "###", "..#", "###"],
    "X": ["#.#", "#.#", ".#.", "#.#", "#.#"],
}


FRAME_BORDER_VALUE = 100  # HSV value above which a coloured frame pixel belongs to the border, not the text panel


def frame_tone(hsv: np.ndarray, mask: np.ndarray) -> tuple[float, float, float]:
    """Average hue (0-255, circular), saturation and value of the masked pixels."""
    angles = hsv[..., 0][mask] * (2 * math.pi / 256)
    hue = math.atan2(np.sin(angles).mean(), np.cos(angles).mean()) * 256 / (2 * math.pi)
    return hue, float(hsv[..., 1][mask].mean()), float(hsv[..., 2][mask].mean())


def frame_zones(image: Image.Image) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """HSV pixels of a frame, with the masks of its coloured border and of its coloured text panel."""
    hsv = np.asarray(image.convert("RGB").convert("HSV"), dtype=np.float32)
    coloured = (np.asarray(image.getchannel("A")) > 128) & (hsv[..., 1] > 96)
    border = coloured & (hsv[..., 2] > FRAME_BORDER_VALUE)
    return hsv, border, coloured & ~border


def load_frame(kind: str, color: str) -> Image.Image:
    """Card frame of a kind and hero colour. The dump has no orange Power frame: a missing frame is the red
    one of that kind, with its border and its text panel moved by the hue, saturation and value differences
    seen between the two Skill frames."""
    template = BIBLE / "card_template"
    if color == "grey":
        # Status and colorless cards: the red frame with its colour washed out. A Status card has no frame
        # of its own and takes the Skill shape; a colorless card keeps the shape of its kind.
        shape = "skill" if kind == "status" else kind
        red = Image.open(template / f"frame_{shape}_red.webp").convert("RGBA")
        hsv, border, panel = frame_zones(red)
        hsv[..., 1] = np.where(border | panel, hsv[..., 1] * 0.12, hsv[..., 1])
        hsv[..., 2] = np.where(border, np.clip(hsv[..., 2] * 0.85, 0, 255), hsv[..., 2])
        grey = Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGBA")
        grey.putalpha(red.getchannel("A"))
        return grey
    if color == "purple":
        # Curse cards: the Skill shape, border and text panel moved to a dim purple.
        red = Image.open(template / "frame_skill_red.webp").convert("RGBA")
        hsv, border, panel = frame_zones(red)
        zone = border | panel
        hsv[..., 0] = np.where(zone, 200, hsv[..., 0])
        hsv[..., 1] = np.where(zone, hsv[..., 1] * 0.55, hsv[..., 1])
        hsv[..., 2] = np.where(zone, hsv[..., 2] * 0.7, hsv[..., 2])
        purple = Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGBA")
        purple.putalpha(red.getchannel("A"))
        return purple
    path = template / f"frame_{kind}_{color}.webp"
    if path.exists():
        return Image.open(path).convert("RGBA")
    red = Image.open(template / f"frame_{kind}_red.webp").convert("RGBA")
    hsv, *zones = frame_zones(red)
    wanted_hsv, *wanted_zones = frame_zones(Image.open(template / f"frame_skill_{color}.webp").convert("RGBA"))
    known_hsv, *known_zones = frame_zones(Image.open(template / "frame_skill_red.webp").convert("RGBA"))
    for zone, wanted_zone, known_zone in zip(zones, wanted_zones, known_zones):
        wanted, known = frame_tone(wanted_hsv, wanted_zone), frame_tone(known_hsv, known_zone)
        hsv[..., 0] = np.where(zone, np.mod(hsv[..., 0] + wanted[0] - known[0], 256), hsv[..., 0])
        hsv[..., 1] = np.where(zone, np.clip(hsv[..., 1] * wanted[1] / known[1], 0, 255), hsv[..., 1])
        hsv[..., 2] = np.where(zone, np.clip(hsv[..., 2] * wanted[2] / known[2], 0, 255), hsv[..., 2])
    recoloured = Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGBA")
    recoloured.putalpha(red.getchannel("A"))
    return recoloured


def card_art_path(card: CardArt, hero: str) -> Path:
    owner = hero if card.owner == "hero" else card.owner
    return BIBLE / "cards" / (card.folder or owner) / (card.art.format(hero=hero) + ".webp")


SHEET_WHITE = (248, 248, 248)    # palette entry every mini-card sheet keeps
ANCIENT_LINE = (248, 248, 240)   # the Ancient border is a white glowing line
ANCIENT_PANEL = (40, 36, 48)     # text panel laid over the art; lighter than black so the type icon keeps its outline
ANCIENT_PANEL_ALPHA = 0.85
ANCIENT_LINE_GAIN = 4.0          # the line is under half a pixel wide at GBA size: its coverage is multiplied
ANCIENT_LINE_THRESHOLD = 110     # multiplied coverage (0-255) from which a pixel belongs to the line


def ancient_panel(kind: str) -> Image.Image:
    """Text panel of an Ancient card, as a mask over the frame template: the dark panel of the kind's frame."""
    red = Image.open(BIBLE / "card_template" / f"frame_{kind}_red.webp").convert("RGBA")
    hsv = np.asarray(red.convert("RGB").convert("HSV"), dtype=np.float32)
    mask = (np.asarray(red.getchannel("A")) > 128) & (hsv[..., 1] <= 96)
    mask[: PORTRAIT_BOX[1] + 4, :] = False  # the dark strip above the portrait is not part of the panel
    return Image.fromarray((mask * 255).astype(np.uint8), "L")


def compose_ancient_card(card: CardArt, hero: str, size: tuple[int, int]) -> Image.Image:
    """Ancient card: its art covers the whole card, under a dark text panel and inside the white Ancient
    border (ancient_border.webp). The Ancient banner is left out: at this size it is a grey smudge over the
    art, and the card name is not drawn on the sprite."""
    panel = ancient_panel(card.kind)
    full = Image.open(card_art_path(card, hero)).convert("RGBA").resize(panel.size, Image.Resampling.LANCZOS)
    shade = Image.new("RGBA", panel.size, ANCIENT_PANEL + (255,))
    shade.putalpha(panel.point(lambda a: int(a * ANCIENT_PANEL_ALPHA)))
    full.alpha_composite(shade)
    outline = Image.new("L", panel.size, 0)
    ImageDraw.Draw(outline).rounded_rectangle((0, 0, panel.width - 1, panel.height - 1), radius=44, fill=255)
    full.putalpha(outline)
    card_img = ImageEnhance.Contrast(full.resize(size, Image.Resampling.LANCZOS)).enhance(1.1)
    border = Image.open(BIBLE / "card_template" / "ancient_border.webp").getchannel("A")
    coverage = np.asarray(border.resize(size, Image.Resampling.BOX), dtype=np.float32) * ANCIENT_LINE_GAIN
    line = Image.new("RGBA", size, ANCIENT_LINE + (255,))
    # All or nothing: a half-covered pixel would blend with the art and take a palette entry of its own.
    line.putalpha(Image.fromarray(((coverage >= ANCIENT_LINE_THRESHOLD) * 255).astype(np.uint8), "L"))
    card_img.alpha_composite(line)
    return card_img


def compose_card(card: CardArt, hero: str, size: tuple[int, int]) -> Image.Image:
    owner = hero if card.owner == "hero" else card.owner
    if owner == "ancient":
        return compose_ancient_card(card, hero, size)
    frame = load_frame(card.kind, FRAME_COLOR[owner])
    full = Image.new("RGBA", frame.size, (0, 0, 0, 0))
    if card.art:
        art = Image.open(card_art_path(card, hero)).convert("RGBA")
        x0, y0, x1, y1 = PORTRAIT_BOX
        art = art.resize((x1 - x0 + 1, y1 - y0 + 1), Image.Resampling.LANCZOS)
        full.paste(art, (x0, y0))
    full.alpha_composite(frame)
    card_img = full.resize(size, Image.Resampling.LANCZOS)
    card_img = ImageEnhance.Contrast(card_img).enhance(1.1)
    return card_img


HERO_RGB = {"ironclad": (200, 48, 40), "silent": (64, 160, 72), "defect": (56, 112, 216),
            "regent": (232, 152, 40), "necrobinder": (200, 96, 184), "status": (120, 120, 132),
            "colorless": (120, 120, 132), "curse": (112, 64, 144), "ancient": ANCIENT_LINE}

TYPE_ICONS = {
    "attack": [
        "......KK.",
        ".....KWK.",
        "....KWK..",
        ".K.KWK...",
        "..KWK....",
        "..KYK....",
        ".KYKK....",
        "KYK......",
        "KK.......",
    ],
    "skill": [
        ".........",
        ".KKKKKKK.",
        ".KWBBBBK.",
        ".KBBBBbK.",
        ".KBBBBbK.",
        "..KBBbK..",
        "...KbK...",
        "....K....",
        ".........",
    ],
    "status": [
        ".........",
        ".KK...KK.",
        ".KSK.KSK.",
        "..KSKSK..",
        "...KSK...",
        "..KSKSK..",
        ".KSK.KSK.",
        ".KK...KK.",
        ".........",
    ],
    "power": [
        "....K....",
        "...KYK...",
        "KKKKYKKKK",
        ".KYYWYYK.",
        "..KYYYK..",
        ".KYKKKYK.",
        ".KK...KK.",
        ".........",
        ".........",
    ],
}


def compose_mini(card: CardArt, hero: str) -> Image.Image:
    """23x32 hand card drawn for legibility: full-bleed art, border in the owner's colour, type icon."""
    owner = hero if card.owner == "hero" else card.owner
    border = HERO_RGB[owner]
    img = Image.new("RGBA", (23, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, 22, 31), radius=2, fill=(12, 10, 16, 255))
    if owner == "ancient":
        # Ancient card: a white line around the art, which goes edge to edge, and the dark panel under it.
        # The art is a whole card: only its top, the part the text panel leaves visible, is shown.
        d.rounded_rectangle((1, 1, 21, 30), radius=2, fill=border + (255,))
        art = Image.open(card_art_path(card, hero)).convert("RGBA")
        art = art.crop((0, 0, art.width, art.width * 18 // 19)).resize((19, 18), Image.Resampling.LANCZOS)
        art = ImageEnhance.Contrast(art).enhance(1.2)
        art = ImageEnhance.Color(art).enhance(1.2)
        img.paste(art, (2, 2))
        d.rectangle((2, 20, 20, 29), fill=ANCIENT_PANEL + (255,))
        img.alpha_composite(pixel_art(TYPE_ICONS[card.kind], UI_COLORS), (7, 21))
        return img
    d.rounded_rectangle((1, 1, 21, 30), radius=2, fill=lerp(border, (255, 255, 255), 0.25) + (255,))
    d.rectangle((2, 2, 20, 29), fill=border + (255,))
    d.rectangle((3, 3, 19, 28), fill=(52, 42, 38, 255))
    if card.art:
        art = Image.open(card_art_path(card, hero)).convert("RGBA")
        cw = int(art.height * 17 / 17)
        x0 = (art.width - cw) // 2
        art = art.crop((x0, 0, x0 + cw, art.height)).resize((17, 17), Image.Resampling.LANCZOS)
        art = ImageEnhance.Contrast(art).enhance(1.2)
        art = ImageEnhance.Color(art).enhance(1.2)
        img.paste(art, (3, 3))
        d.line((3, 20, 19, 20), fill=border + (255,))
        icon = pixel_art(TYPE_ICONS[card.kind], UI_COLORS)
        img.alpha_composite(icon, (7, 21))
    return img


def energy_gem(hero: str, size: int) -> Image.Image:
    if hero in SHARED_OWNERS:
        # No colourless gem in the dump: the Ironclad one, without its colour.
        gem = Image.open(BIBLE / "card_template" / "energy_ironclad.webp").convert("RGBA")
        grey = ImageEnhance.Color(gem).enhance(0.0)
        return grey.resize((size, size), Image.Resampling.LANCZOS)
    gem = Image.open(BIBLE / "card_template" / f"energy_{hero}.webp").convert("RGBA")
    return gem.resize((size, size), Image.Resampling.LANCZOS)


def on_cartridge(card: CardArt, hero: str) -> bool:
    """A cartridge only carries the art of its hero's cards and of the cards every hero shares."""
    return card.owner in ("hero", hero) or card.owner in SHARED_OWNERS


def sheet_cards(hero: str) -> list[CardArt]:
    """Cards of the hero's mini-card sheet, in sheet order. Slot 0 is the blank card."""
    return [card for card in CARDS if on_cartridge(card, hero)]


def write_card_art_header() -> None:
    """C++ view of the card art, per cartridge: big card items and positions in the mini-card sheet."""
    lines = [
        "// Generated by scripts/build_gba_art.py - do not edit by hand.",
        "// Card art of the cartridge being built (-DHERO_*): the hero's own cards and the ones every hero shares.",
        "// A card of another hero has no art here: big() returns nullptr and its sheet slot is the blank card.",
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <iterator>",
        "",
        '#include "bn_sprite_item.h"',
        "",
        '#include "engine/card.h"',
        "",
    ]
    # The Ironclad cartridge is the default one, as in the Makefile.
    others = [hero for hero in HEROES if hero != "ironclad"]
    for position, hero in enumerate(others):
        lines.append(f"#{'if' if position == 0 else 'elif'} defined(HERO_{hero.upper()})")
        lines += card_art_section(hero)
    lines.append("#else")
    lines += card_art_section("ironclad")
    lines += ["#endif", ""]
    CARD_ART_HEADER.write_text("\n".join(lines))


def card_art_section(hero: str) -> list[str]:
    cards = sheet_cards(hero)
    slots = {card.enum_name: slot for slot, card in enumerate(cards)}

    def item(card: CardArt) -> str:
        return f"card_{card.enum_name}_{hero}" if card.owner == "hero" else f"card_{card.enum_name}"

    lines = [""]
    lines += [f'#include "bn_sprite_items_{item(card)}.h"' for card in cards[1:]]
    lines += [
        "",
        "namespace gba::card_art",
        "{",
        "",
        "// 64x64 illustrated card.",
        "[[nodiscard]] inline const bn::sprite_item* big(engine::CardId id)",
        "{",
        "    switch(id)",
        "    {",
    ]
    for card in cards[1:]:
        lines += [f"    case engine::CardId::{card.enum_name.upper()}:", f"        return &bn::sprite_items::{item(card)};"]
    lines += [
        "    default:",
        "        return nullptr;",
        "    }",
        "}",
        "",
        "// Slot of each card in the hero's mini-card sheet, indexed by CardId. A slot is two frames: plain, upgraded.",
        "constexpr uint16_t SHEET_SLOT[] = {",
    ]
    values = [str(slots.get(card.enum_name, 0)) for card in CARDS]
    for start in range(0, len(values), 20):
        lines.append("    " + ", ".join(values[start:start + 20]) + ",")
    lines += [
        "};",
        "",
        "// CARDS in build_gba_art.py mirrors engine::CardId: a card added on one side only stops the build here.",
        "static_assert(std::size(SHEET_SLOT) == static_cast<size_t>(engine::CardId::COUNT));",
        "",
        "}",
        "",
    ]
    return lines


def build_cards() -> None:
    write_card_art_header()
    for hero in HEROES:
        # Big cards: one sprite item (own palette) per card, 46x62 card inside a 64x64 sprite.
        for card in CARDS[1:]:
            if card.owner != "hero" and hero != "ironclad":
                continue  # hero-agnostic cards are generated once
            name = f"card_{card.enum_name}_{hero}" if card.owner == "hero" else f"card_{card.enum_name}"
            canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
            body = compose_card(card, hero, (46, 62))
            canvas.paste(body, (9, 1), body)
            icon = pixel_art(TYPE_ICONS[card.kind], UI_COLORS)
            icon = icon.resize((icon.width * 2, icon.height * 2), Image.Resampling.NEAREST)
            canvas.alpha_composite(icon, (32 - icon.width // 2, 38))
            owner = hero if card.owner == "hero" else card.owner
            if card.costs[0] != NO_COST:
                gem = energy_gem(owner, 16)
                canvas.alpha_composite(gem, (3, 0))
            idx, pal = rgba_to_sprite(canvas, 15)
            save_sprite(name, [idx], pal, 64)

        # Mini cards: every (card, upgraded) pair of the cartridge as frames of one item sharing a palette.
        minis: list[Image.Image] = []
        for card in sheet_cards(hero):
            for upgraded in (False, True):
                canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
                body = compose_mini(card, hero)
                canvas.paste(body, (5, 0), body)
                owner = hero if card.owner == "hero" else card.owner
                if card.costs[0] != NO_COST:
                    gem = energy_gem(owner, 11)
                    canvas.alpha_composite(gem, (1, 0))
                if card.enum_name != "none" and card.costs[0] != NO_COST:
                    value = card.costs[1] if upgraded else card.costs[0]
                    cost = "X" if value < 0 else str(value)
                    draw = ImageDraw.Draw(canvas)
                    # One digit sits in the middle of the gem; two digits (Midnight costs 12) share its width.
                    left = 5 if len(cost) == 1 else 3
                    for place, digit in enumerate(cost):
                        for dy, row in enumerate(DIGITS[digit]):
                            for dx, ch in enumerate(row):
                                if ch == "#":
                                    draw.point((left + 4 * place + dx, 3 + dy), fill=(255, 255, 255, 255))
                    stars = card.stars[1] if upgraded else card.stars[0]
                    if stars:
                        # Star cost: a yellow digit on a dark plate under the energy gem.
                        draw.rectangle((3, 12, 9, 20), fill=(24, 20, 40, 255))
                        for dy, row in enumerate(DIGITS["X" if stars < 0 else str(stars)]):
                            for dx, ch in enumerate(row):
                                if ch == "#":
                                    draw.point((5 + dx, 14 + dy), fill=(255, 216, 64, 255))
                    if upgraded:
                        # Small green "+" badge in the bottom-right corner.
                        for px, py in [(25, 26), (24, 27), (25, 27), (26, 27), (25, 28)]:
                            draw.point((px, py), fill=(96, 240, 96, 255))
                minis.append(canvas)
        samples = []
        for img in minis:
            arr = np.array(img)
            samples.append(arr[:, :, :3][arr[:, :, 3] >= 110])
        # One entry is kept for white: the median cut would otherwise merge the cost digits and the Ancient
        # border with the nearest art colour (cream, or the Regent's orange).
        palette = quantize_opaque(np.vstack(samples), 14)
        if SHEET_WHITE not in palette:
            palette.append(SHEET_WHITE)
        frames = [rgba_to_sprite(img, palette=palette)[0] for img in minis]
        save_sprite(f"hand_cards_{hero}", frames, [TRANSPARENT] + palette, 32)

        # Energy orb for the combat HUD.
        canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        gem = energy_gem(hero, 28)
        canvas.alpha_composite(gem, (2, 2))
        idx, pal = rgba_to_sprite(canvas, 15)
        save_sprite(f"ui_energy_{hero}", [idx], pal, 32)


# ---------------------------------------------------------------------------------------------
# UI sprites (hand-authored pixel art)
# ---------------------------------------------------------------------------------------------

K = (16, 16, 24)
W = (248, 248, 248)
UI_COLORS = {
    "K": K, "W": W,
    "S": (176, 184, 200), "s": (104, 112, 136),   # steel
    "Y": (248, 200, 64), "y": (184, 120, 40),      # gold
    "R": (232, 56, 48), "r": (144, 24, 32),        # red
    "B": (72, 144, 232), "b": (40, 72, 152),       # blue
    "G": (88, 216, 96), "g": (40, 128, 56),        # green
    "P": (184, 96, 232), "p": (96, 48, 144),       # purple
}

ICON_ATTACK = [
    "................",
    "............KKK.",
    "...........KWWK.",
    "..........KWSSK.",
    ".........KWSsK..",
    "........KWSsK...",
    "..KK...KWSsK....",
    "..KYK.KWSsK.....",
    "...KYKWSsK......",
    "....KYSsK.......",
    "....KyYK........",
    "...KyKKYK.......",
    "..KyK..KYK......",
    ".KyK....KK......",
    ".KK.............",
    "................",
]
ICON_DEFEND = [
    "................",
    "..KKKKKKKKKKKK..",
    "..KWBBBBBBBBBK..",
    "..KWBBBBBBBBbK..",
    "..KBBBBBBBBBbK..",
    "..KBBBBBBBBBbK..",
    "..KBBBBBBBBBbK..",
    "..KBBBBBBBBbbK..",
    "...KBBBBBBBbK...",
    "...KBBBBBBbbK...",
    "....KBBBBbbK....",
    ".....KBBbbK.....",
    "......KbbK......",
    ".......KK.......",
    "................",
    "................",
]
ICON_BUFF = [
    ".......KK.......",
    "......KRRK......",
    ".....KRWRRK.....",
    "....KRWRRRrK....",
    "...KRRRRRRRrK...",
    "..KKKKRRRrKKKK..",
    ".....KRRRrK.....",
    ".....KRRRrK.....",
    ".....KRRRrK.....",
    ".....KRRRrK.....",
    ".....KRRrrK.....",
    ".....KKKKKK.....",
    "................",
    "..KK..KK..KK....",
    ".KRrKKRrKKRrK...",
    "..KK..KK..KK....",
]
ICON_DEBUFF = [
    "................",
    ".......KK.......",
    "......KGGK......",
    ".....KGWGGK.....",
    ".....KGGGgK.....",
    "....KGWGGGgK....",
    "....KGGGGGgK....",
    "...KGGGGGGggK...",
    "...KGGGGGGggK...",
    "...KGGGGGggK....",
    "....KGGGggK.....",
    ".....KKKKK......",
    "................",
    "................",
    "................",
    "................",
]
ICON_UNKNOWN = [
    "................",
    ".....KKKKKK.....",
    "....KPPPPPPK....",
    "...KPPKKKKPPK...",
    "...KPK....KPK...",
    "...KKK...KPPK...",
    ".......KPPPK....",
    "......KPPKK.....",
    "......KPPK......",
    "......KKKK......",
    "................",
    "......KPPK......",
    "......KPPK......",
    "......KKKK......",
    "................",
    "................",
]
ICON_TARGET = [
    "................",
    "...KKKKKKKKKK...",
    "...KYYYYYYYYK...",
    "....KYYYYYYK....",
    ".....KYYYYK.....",
    "......KYYK......",
    ".......KK.......",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
]
ICON_HEART = [
    "................",
    "................",
    "..KKK....KKK....",
    ".KRRRK..KRRRK...",
    "KRWWRRKKRRRRrK..",
    "KRWRRRRRRRRRrK..",
    "KRRRRRRRRRRrrK..",
    ".KRRRRRRRRRrK...",
    "..KRRRRRRRrK....",
    "...KRRRRRrK.....",
    "....KRRRrK......",
    ".....KRrK.......",
    "......KK........",
    "................",
    "................",
    "................",
]
ICON_GOLD = [
    "................",
    "................",
    ".....KKKKK......",
    "....KYYYYYK.....",
    "...KYWYYYYyK....",
    "..KYWYYyYYYyK...",
    "..KYYYyyyYYyK...",
    "..KYYYyYYYYyK...",
    "..KYYYyyyYYyK...",
    "..KYYYYYyYYyK...",
    "...KYYyyyYyK....",
    "....KyyyyyK.....",
    ".....KKKKK......",
    "................",
    "................",
    "................",
]
ICON_STUN = [
    "................",
    ".......KK.......",
    "......KYYK......",
    "......KYYK......",
    "..KKKKKYYKKKKK..",
    "..KYYYYWYYYYyK..",
    "...KYYWYYYYyK...",
    "....KYYYYYyK....",
    "....KYYYYYyK....",
    "...KYYYyyYYyK...",
    "...KYYyKKyYyK...",
    "..KYyKK..KKyyK..",
    "..KKK......KKK..",
    "................",
    "................",
    "................",
]
ICON_HEAL = [
    "................",
    "................",
    "......KKKK......",
    "......KGGK......",
    "......KWGK......",
    "..KKKKKGGKKKKK..",
    "..KGWGGGGGGGgK..",
    "..KGGGGGGGGggK..",
    "..KKKKKGGKKKKK..",
    "......KGgK......",
    "......KGgK......",
    "......KggK......",
    "......KKKK......",
    "................",
    "................",
    "................",
]
ICON_ESCAPE = [
    "................",
    "................",
    ".........KK.....",
    ".........KWK....",
    ".........KWSK...",
    "..KKKKKKKKWSSK..",
    "..KWWWWWWWWSSSK.",
    "..KSSSSSSSSSSsK.",
    "..KKKKKKKKSSsK..",
    ".........KSsK...",
    ".........KsK....",
    ".........KK.....",
    "................",
    "................",
    "................",
    "................",
]
ICON_STATUS = [
    "................",
    "....KKKKKKKK....",
    "...KSSSSSSSSK...",
    "...KSWWWWWWsK...",
    "...KSWPPPPWsK...",
    "...KSWPppPWsK...",
    "...KSWPppPWsK...",
    "...KSWPPPPWsK...",
    "...KSWWWWWWsK...",
    "...KSWssssWsK...",
    "...KSWWWWWWsK...",
    "...KSssssssK....",
    "....KKKKKKKK....",
    "................",
    "................",
    "................",
]


def build_ui() -> None:
    icons = [ICON_ATTACK, ICON_DEFEND, ICON_BUFF, ICON_DEBUFF, ICON_UNKNOWN, ICON_TARGET, ICON_HEART, ICON_GOLD,
             ICON_STUN, ICON_HEAL, ICON_ESCAPE, ICON_STATUS]
    imgs = [pixel_art(rows, UI_COLORS) for rows in icons]
    palette = [K, W, (176, 184, 200), (104, 112, 136), (248, 200, 64), (184, 120, 40), (232, 56, 48),
               (144, 24, 32), (72, 144, 232), (40, 72, 152), (88, 216, 96), (40, 128, 56), (184, 96, 232),
               (96, 48, 144)]
    frames = [rgba_to_sprite(img, palette=palette)[0] for img in imgs]
    save_sprite("ui_icons", frames, [TRANSPARENT] + palette, 16)

    # HP bars: 64x8 (two 32x8 halves), 31 fill levels x 3 colours (green / yellow / red), Pokemon style.
    bar_pal = [K, (56, 56, 72), (24, 24, 32),
               (112, 248, 168), (56, 200, 88),     # green light / main
               (248, 232, 88), (224, 168, 16),     # yellow
               (248, 112, 88), (208, 40, 40),      # red
               (248, 248, 248), (120, 168, 248), (48, 96, 208)]  # white, block blue
    frames = []
    for color in range(3):
        light, main = [(4, 5), (6, 7), (8, 9)][color]
        for level in range(31):
            a = np.zeros((8, 64), dtype=np.int32)
            a[1:7, 1:63] = 1                 # outline
            a[0, 2:62] = 1
            a[7, 2:62] = 1
            a[2:6, 2:62] = 3                 # empty trough
            fill = level * 2
            if fill > 0:
                a[2, 2 : 2 + fill] = light
                a[3:6, 2 : 2 + fill] = main
            # 64x8 is not a GBA sprite size: store left and right 32x8 halves as consecutive frames.
            frames.append(a[:, :32])
            frames.append(a[:, 32:])
    save_sprite("ui_hp_bar", frames, [TRANSPARENT] + bar_pal, 8)

    # Attack slash VFX: 4 frames of a white/yellow arc, 64x64.
    frames_img = []
    for f in range(4):
        img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        start = -150 + f * 30
        sweep = 70 + f * 25
        for width, col in [(9, (248, 200, 64, 255)), (5, (248, 248, 248, 255))]:
            d.arc((6, 6, 58, 58), start, start + sweep, fill=col, width=width if f < 3 else width - 3)
        if f >= 2:
            for _ in range(6):
                ang = math.radians(start + sweep)
                r = 26
                x = 32 + int(math.cos(ang) * r) + random.randint(-6, 6)
                y = 32 + int(math.sin(ang) * r) + random.randint(-6, 6)
                d.point((x, y), fill=(248, 248, 248, 255))
        frames_img.append(img)
    pal = [(248, 200, 64), (248, 248, 248), (184, 120, 40)]
    frames = [rgba_to_sprite(img, palette=pal)[0] for img in frames_img]
    save_sprite("fx_slash", frames, [TRANSPARENT] + pal, 64)

    # Hit spark / block ring / buff sparkle: 32x32, 4 frames each, one item.
    frames_img = []
    for kind in range(3):
        for f in range(4):
            img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
            d = ImageDraw.Draw(img)
            if kind == 0:  # hit spark
                r = 4 + f * 4
                for i in range(8):
                    ang = i * math.pi / 4 + f * 0.2
                    x1, y1 = 16 + math.cos(ang) * (r - 3), 16 + math.sin(ang) * (r - 3)
                    x2, y2 = 16 + math.cos(ang) * r, 16 + math.sin(ang) * r
                    d.line((x1, y1, x2, y2), fill=(248, 248, 160, 255) if f < 2 else (248, 160, 64, 255), width=2)
                if f == 0:
                    d.ellipse((12, 12, 20, 20), fill=(248, 248, 248, 255))
            elif kind == 1:  # block ring
                r = 5 + f * 3
                d.ellipse((16 - r, 16 - r, 16 + r, 16 + r), outline=(120, 168, 248, 255), width=2)
                if f < 2:
                    d.ellipse((16 - r + 3, 16 - r + 3, 16 + r - 3, 16 + r - 3), outline=(248, 248, 248, 255), width=1)
            else:  # buff sparkle (rising)
                for i in range(4):
                    x = 6 + i * 7
                    y = 26 - f * 5 - (i % 2) * 4
                    d.line((x, y - 3, x, y + 3), fill=(248, 96, 64, 255))
                    d.line((x - 3, y, x + 3, y), fill=(248, 200, 64, 255))
            frames_img.append(img)
    pal = [(248, 248, 160), (248, 160, 64), (248, 248, 248), (120, 168, 248), (248, 96, 64), (248, 200, 64)]
    frames = [rgba_to_sprite(img, palette=pal)[0] for img in frames_img]
    save_sprite("fx_burst", frames, [TRANSPARENT] + pal, 32)


# ---------------------------------------------------------------------------------------------
# Backgrounds
# ---------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Theme:
    sky_top: tuple[int, int, int]
    sky_bottom: tuple[int, int, int]
    far: tuple[int, int, int]
    near: tuple[int, int, int]
    ground_top: tuple[int, int, int]
    ground_bottom: tuple[int, int, int]
    accent: tuple[int, int, int]
    glow: tuple[int, int, int]


THEMES: dict[str, Theme] = {
    "ironclad": Theme((24, 6, 10), (150, 48, 24), (70, 18, 20), (38, 10, 14), (88, 44, 32), (40, 20, 18),
                      (232, 88, 48), (255, 160, 64)),
    "silent": Theme((6, 18, 14), (70, 110, 60), (26, 52, 34), (14, 30, 22), (54, 72, 40), (24, 34, 22),
                    (120, 208, 96), (200, 240, 120)),
    "defect": Theme((4, 8, 28), (40, 90, 150), (20, 36, 80), (10, 18, 44), (40, 56, 96), (18, 24, 48),
                    (88, 176, 240), (160, 232, 255)),
    "regent": Theme((20, 8, 30), (150, 90, 60), (70, 36, 80), (38, 18, 46), (96, 72, 56), (44, 30, 34),
                    (240, 176, 64), (255, 216, 120)),
    "necrobinder": Theme((12, 6, 20), (90, 50, 110), (46, 22, 60), (24, 12, 34), (62, 46, 70), (28, 20, 34),
                         (200, 104, 216), (232, 176, 255)),
}

BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def lerp(a: tuple[int, int, int], b: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))  # type: ignore[return-value]


def banded_gradient(draw_arr: np.ndarray, y0: int, y1: int, top: tuple[int, int, int],
                    bottom: tuple[int, int, int], bands: int = 8) -> None:
    """Pixel-art gradient: a few flat bands blended with a 4x4 Bayer dither."""
    for y in range(y0, y1):
        t = (y - y0) / max(1, (y1 - y0 - 1)) * (bands - 1)
        lo = int(t)
        frac = t - lo
        c_lo = lerp(top, bottom, lo / (bands - 1))
        c_hi = lerp(top, bottom, min(bands - 1, lo + 1) / (bands - 1))
        for x in range(draw_arr.shape[1]):
            draw_arr[y, x] = c_hi if frac > BAYER4[y % 4, x % 4] else c_lo


def silhouette(arr: np.ndarray, base_y: int, amp: int, color: tuple[int, int, int], seed: int,
               step: int = 6) -> None:
    rng = random.Random(seed)
    h = base_y
    heights = []
    for x in range(0, arr.shape[1] + step, step):
        h = max(base_y - amp, min(base_y + amp // 3, h + rng.randint(-amp // 3, amp // 3)))
        heights.append(h)
    for x in range(arr.shape[1]):
        i = x // step
        t = (x % step) / step
        top = int(heights[i] + (heights[i + 1] - heights[i]) * t)
        arr[top:, x] = color


def draw_panel(img: Image.Image, box: tuple[int, int, int, int], accent: tuple[int, int, int],
               fill: tuple[int, int, int] = (14, 16, 30)) -> None:
    """Pokemon-like text box: rounded dark panel with a light double border."""
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = box
    d.rounded_rectangle(box, radius=4, fill=(8, 8, 14))
    d.rounded_rectangle((x0 + 1, y0 + 1, x1 - 1, y1 - 1), radius=3, fill=lerp(accent, (255, 255, 255), 0.35))
    d.rounded_rectangle((x0 + 2, y0 + 2, x1 - 2, y1 - 2), radius=3, fill=accent)
    d.rounded_rectangle((x0 + 3, y0 + 3, x1 - 3, y1 - 3), radius=2, fill=fill)
    # Subtle vertical sheen on the panel body.
    for y in range(y0 + 4, y1 - 3):
        t = (y - y0) / max(1, y1 - y0)
        d.line((x0 + 4, y, x1 - 4, y), fill=lerp(lerp(fill, (40, 44, 70), 0.5), fill, t))


def build_battle_bg(hero: str) -> None:
    th = THEMES[hero]
    W_, H_ = 240, 160
    arr = np.zeros((H_, W_, 3), dtype=np.uint8)
    banded_gradient(arr, 0, 86, th.sky_top, th.sky_bottom, bands=9)

    # Stars / embers in the sky.
    rng = random.Random(hash(hero) & 0xFFFF)
    for _ in range(40):
        x, y = rng.randrange(W_), rng.randrange(60)
        arr[y, x] = lerp(th.glow, (255, 255, 255), 0.5) if rng.random() < 0.5 else th.glow

    # The Spire in the distance.
    spire = Image.new("RGBA", (W_, H_), (0, 0, 0, 0))
    ds = ImageDraw.Draw(spire)
    cx = 150
    ds.polygon([(cx - 13, 86), (cx - 5, 18), (cx, 4), (cx + 5, 18), (cx + 13, 86)], fill=th.far + (255,))
    for y in range(24, 86, 9):
        ds.line((cx - 4 - (y - 18) // 9, y, cx + 4 + (y - 18) // 9, y), fill=lerp(th.far, th.glow, 0.25) + (255,))
    ds.ellipse((cx - 2, 10, cx + 2, 14), fill=th.glow + (255,))

    base = Image.fromarray(arr).convert("RGBA")
    base.alpha_composite(spire)
    arr = np.array(base.convert("RGB"))

    # Two layers of mountains / ruins.
    layer = arr.copy()
    silhouette(layer, 74, 18, th.far, seed=1, step=8)
    arr = np.where(np.all(layer == th.far, axis=2)[:, :, None], layer, arr)
    layer = arr.copy()
    silhouette(layer, 84, 10, th.near, seed=7, step=5)
    arr = np.where(np.all(layer == th.near, axis=2)[:, :, None], layer, arr)

    # Ground plane with perspective stripes.
    banded_gradient(arr, 86, 104, th.ground_top, th.ground_bottom, bands=5)
    img = Image.fromarray(arr).convert("RGBA")
    d = ImageDraw.Draw(img)
    for i in range(-6, 8):
        x_far = 120 + i * 16
        x_near = 120 + i * 48
        d.line((x_far, 86, x_near, 104), fill=lerp(th.ground_top, th.ground_bottom, 0.4) + (255,))
    # Horizon glow line.
    d.line((0, 86, W_, 86), fill=lerp(th.ground_top, th.glow, 0.45) + (255,))

    # Pokemon-like platforms: player (left) and enemies (right).
    def platform(cx_: int, cy: int, rx: int, ry: int) -> None:
        d.ellipse((cx_ - rx, cy - ry + 2, cx_ + rx, cy + ry + 2), fill=(10, 8, 12, 255))
        d.ellipse((cx_ - rx, cy - ry, cx_ + rx, cy + ry), fill=lerp(th.ground_top, (255, 255, 255), 0.18) + (255,))
        d.ellipse((cx_ - rx + 4, cy - ry + 2, cx_ + rx - 4, cy + ry - 1), fill=th.ground_top + (255,))
        d.ellipse((cx_ - rx + 10, cy - ry + 4, cx_ + rx - 10, cy + ry - 3), fill=lerp(th.ground_top, th.ground_bottom, 0.4) + (255,))

    platform(44, 94, 36, 8)
    platform(172, 90, 62, 9)

    # Bottom command panel (hand + card text).
    draw_panel(img, (0, 104, 239, 159), th.accent)
    save_bg(f"bg_battle_{hero}", img.convert("RGB"))


def load_scene_art(path: Path, size: tuple[int, int], brighten: float = 1.0, focus_y: float = 0.5,
                   fit_width: bool = False) -> Image.Image:
    img = Image.open(path).convert("RGB")
    scale = size[0] / img.width if fit_width else max(size[0] / img.width, size[1] / img.height)
    img = img.resize((math.ceil(img.width * scale), math.ceil(img.height * scale)), Image.Resampling.LANCZOS)
    x = (img.width - size[0]) // 2
    y = int((img.height - size[1]) * focus_y)
    img = img.crop((x, max(0, y), x + size[0], max(0, y) + size[1]))
    if brighten != 1.0:
        # Lift the shadows of the very dark event paintings (gamma), then add a bit of pop.
        arr = np.asarray(img).astype(np.float32) / 255.0
        arr = np.power(arr, 1.0 / brighten)
        img = Image.fromarray((arr * 255).clip(0, 255).astype(np.uint8))
        img = ImageEnhance.Contrast(img).enhance(1.08)
    return img


def build_scene_bgs() -> None:
    # Title: key art, with a darkened band behind the logo text.
    title = load_scene_art(BIBLE / "misc" / "hero-keyart.jpg", (240, 160), 1.0, 0.35)
    over = Image.new("RGBA", (240, 160), (0, 0, 0, 0))
    od = ImageDraw.Draw(over)
    for y in range(160):
        a = 0
        if y < 40:
            a = int(170 * (1 - y / 40))
        elif y > 104:
            a = int(200 * min(1, (y - 104) / 30))
        od.line((0, y, 240, y), fill=(6, 4, 10, a))
    title = title.convert("RGBA")
    title.alpha_composite(over)
    save_bg("bg_title", title.convert("RGB"), dither=True)

    # Event scenes: art on top, dialogue box below.
    events = {
        "battleworn_dummy": ("battleworn_dummy", 2.1, 0.0),
        "colossal_flower": ("colossal_flower", 2.0, 0.0),
        "abyssal_baths": ("abyssal_baths", 2.3, 0.0),
        "mystic_altar": ("the_legends_were_true", 1.8, 0.0),
    }
    for name, (src, gamma, fy) in events.items():
        art = load_scene_art(BIBLE / "events" / f"{src}.webp", (240, 160), gamma, fy, fit_width=True).convert("RGBA")
        draw_panel(art, (0, 88, 239, 159), (120, 104, 200))
        save_bg(f"bg_event_{name}", art.convert("RGB"), dither=True)

    scenes = {
        "campfire": ("events/unrest_site.webp", 1.7, 0.0, (232, 128, 48)),
        "shop": ("events/welcome_to_wongos.webp", 1.6, 0.0, (232, 184, 64)),
        "rewards": ("events/sunken_treasury.webp", 2.0, 0.0, (232, 184, 64)),
        "gameover": ("events/grave_of_the_forgotten.webp", 1.8, 0.5, (160, 64, 80)),
        "victory": ("ancients/art/neow.webp", 1.2, 0.45, (96, 200, 232)),
    }
    for name, (src, gamma, fy, accent) in scenes.items():
        art = load_scene_art(BIBLE / src, (240, 160), gamma, fy,
                             fit_width=name in ("campfire", "shop", "rewards")).convert("RGBA")
        if name in ("campfire", "shop"):
            draw_panel(art, (0, 100, 239, 159), accent)
        elif name == "rewards":
            draw_panel(art, (0, 118, 239, 159), accent)
        save_bg(f"bg_{name}", art.convert("RGB"), dither=True)


def build_map_bg() -> None:
    """Tall 256x512 tower-interior backdrop scrolled behind the map nodes (4bpp, tile friendly)."""
    W_, H_ = 256, 512
    img = Image.new("RGB", (W_, H_), (18, 16, 30))
    d = ImageDraw.Draw(img)
    brick_a, brick_b, mortar = (30, 28, 48), (26, 24, 42), (12, 10, 20)
    for y in range(0, H_, 8):
        offset = 0 if (y // 8) % 2 == 0 else 8
        for x in range(-16, W_, 16):
            c = brick_a if ((x + offset) // 16 + y // 8) % 3 else brick_b
            d.rectangle((x + offset, y, x + offset + 14, y + 6), fill=c)
            d.line((x + offset, y + 7, x + offset + 15, y + 7), fill=mortar)
            d.line((x + offset + 15, y, x + offset + 15, y + 7), fill=mortar)
    # Carpet runner down the middle and pillars on the sides.
    d.rectangle((40, 0, 215, H_), fill=(22, 18, 34))
    for y in range(0, H_, 16):
        d.rectangle((44, y, 211, y + 15), fill=(34, 22, 40) if (y // 16) % 2 else (30, 20, 36))
    d.line((43, 0, 43, H_), fill=(88, 64, 40))
    d.line((212, 0, 212, H_), fill=(88, 64, 40))
    for x0 in (8, 224):
        d.rectangle((x0, 0, x0 + 23, H_), fill=(44, 40, 64))
        d.line((x0, 0, x0, H_), fill=(70, 66, 96))
        d.line((x0 + 23, 0, x0 + 23, H_), fill=(20, 18, 30))
        for y in range(0, H_, 32):
            d.rectangle((x0 + 2, y + 4, x0 + 21, y + 6), fill=(64, 58, 88))
            d.rectangle((x0 + 8, y + 12, x0 + 15, y + 20), fill=(232, 136, 48))  # torch
            d.rectangle((x0 + 10, y + 10, x0 + 13, y + 12), fill=(255, 216, 96))
    save_bg("bg_map", img, bpp4=True)


# ---------------------------------------------------------------------------------------------
# French-capable sprite font (derived from Butano's variable 8x8 font)
# ---------------------------------------------------------------------------------------------

FILL, SHADOW = 14, 12

ACCENTS_LOWER = {  # accent rows 1-2 above a lowercase letter
    "acute": [".#", "#."], "grave": ["#.", ".#"], "circ": [".#.", "#.#"], "trema": ["#.#", "..."],
}
ACCENTS_UPPER = {  # accent rows 0-1 above a squashed capital
    "acute": ["..#.", ".#.."], "grave": [".#..", "..#."], "circ": ["..#..", ".#.#."], "trema": [".#.#.", "....."],
}

FRENCH = [  # (utf8 char, base char, accent)
    ("à", "a", "grave"), ("â", "a", "circ"), ("ä", "a", "trema"), ("ç", "c", "cedilla"),
    ("é", "e", "acute"), ("è", "e", "grave"), ("ê", "e", "circ"), ("ë", "e", "trema"),
    ("î", "i", "circ"), ("ï", "i", "trema"), ("ô", "o", "circ"), ("ö", "o", "trema"),
    ("ù", "u", "grave"), ("û", "u", "circ"), ("ü", "u", "trema"), ("ÿ", "y", "trema"),
    ("À", "A", "grave"), ("Â", "A", "circ"), ("Ç", "C", "cedilla"), ("É", "E", "acute"),
    ("È", "E", "grave"), ("Ê", "E", "circ"), ("Ë", "E", "trema"), ("Î", "I", "circ"),
    ("Ï", "I", "trema"), ("Ô", "O", "circ"), ("Ù", "U", "grave"), ("Û", "U", "circ"), ("Ü", "U", "trema"),
]

SYMBOLS = {  # extra glyphs as 8x8 fill masks (shadow added automatically)
    "œ": ["........", "........", "........", ".##.##..", "#..#..#.", "#..###..", "#..#....", ".##.###."],
    "Œ": [".#####..", "#..#....", "#..#....", "#..###..", "#..#....", "#..#....", ".#####..", "........"],
    "«": ["........", "........", "..#..#..", ".#..#...", "#..#....", ".#..#...", "..#..#..", "........"],
    "»": ["........", "........", "#..#....", ".#..#...", "..#..#..", ".#..#...", "#..#....", "........"],
    "’": ["##......", "##......", ".#......", "#.......", "........", "........", "........", "........"],
    "…": ["........", "........", "........", "........", "........", "........", "#.#.#...", "........"],
    "°": [".#......", "#.#.....", ".#......", "........", "........", "........", "........", "........"],
    "—": ["........", "........", "........", "........", "######..", "........", "........", "........"],
    "♥": ["........", ".##.##..", "#######.", "#######.", ".#####..", "..###...", "...#....", "........"],
    "★": ["...#....", "...#....", "#######.", ".#####..", "..###...", ".##.##..", ".#...#..", "........"],
    "☆": ["...#....", "..#.#...", "##...##.", ".#...#..", "..#.#...", ".#.#.#..", ".#...#..", "........"],
    "▲": ["........", "...#....", "..###...", ".#####..", "#######.", "........", "........", "........"],
    "▼": ["........", "#######.", ".#####..", "..###...", "...#....", "........", "........", "........"],
    "►": ["#.......", "##......", "###.....", "####....", "###.....", "##......", "#.......", "........"],
    "◄": ["...#....", "..##....", ".###....", "####....", ".###....", "..##....", "...#....", "........"],
    "●": ["........", "..###...", ".#####..", ".#####..", ".#####..", "..###...", "........", "........"],
    "○": ["........", "..###...", ".#...#..", ".#...#..", ".#...#..", "..###...", "........", "........"],
    "⚡": ["....##..", "...##...", "..##....", ".#####..", "...##...", "..##....", ".##.....", "........"],
    "✚": ["........", "..##....", "..##....", "######..", "######..", "..##....", "..##....", "........"],
    "×": ["........", "........", "#...#...", ".#.#....", "..#.....", ".#.#....", "#...#...", "........"],
}


def glyph_fill(font: np.ndarray, index: int) -> np.ndarray:
    return font[index * 8 : index * 8 + 8, :] == FILL


def with_shadow(fill: np.ndarray) -> np.ndarray:
    out = np.zeros((8, 8), dtype=np.int32)
    shadow = np.zeros_like(fill)
    shadow[:, 1:] |= fill[:, :-1]
    shadow[1:, :] |= fill[:-1, :]
    shadow[1:, 1:] |= fill[:-1, :-1]
    out[shadow & ~fill] = SHADOW
    out[fill] = FILL
    return out


def stamp(mask: np.ndarray, rows: list[str], y0: int, x0: int) -> None:
    for dy, row in enumerate(rows):
        for dx, ch in enumerate(row):
            if ch == "#" and 0 <= y0 + dy < 8 and 0 <= x0 + dx < 8:
                mask[y0 + dy, x0 + dx] = True


def french_glyph(font: np.ndarray, base: str, accent: str) -> np.ndarray:
    fill = glyph_fill(font, ord(base) - 33).copy()
    cols = np.where(fill.any(axis=0))[0]
    left, right = (int(cols.min()), int(cols.max())) if len(cols) else (0, 4)
    width = right - left + 1
    if accent == "cedilla":
        # Squash the letter one row up and hang a cedilla under it.
        rows = np.where(fill.any(axis=1))[0]
        top, bottom = int(rows.min()), int(rows.max())
        body = fill[top : bottom + 1]
        squashed = np.vstack([body[:-2], body[-1:]]) if len(body) > 3 else body
        fill[:] = False
        fill[top : top + len(squashed)] = squashed
        cx = left + width // 2 - 1
        fill[7, cx] = True
        fill[7, cx + 1] = True
        return with_shadow(fill)
    if base.isupper():
        # Capitals use all 8 rows: squash to rows 2..7 by dropping two interior rows.
        rows = [r for r in range(8) if r not in (2, 5)]
        squashed = fill[rows]
        fill[:] = False
        fill[2:8] = squashed
        pattern = ACCENTS_UPPER[accent]
        stamp(fill, pattern, 0, left + max(0, (width - len(pattern[0])) // 2))
    else:
        fill[0:3] = False
        pattern = ACCENTS_LOWER[accent]
        stamp(fill, pattern, 1, left + max(0, (width - len(pattern[0])) // 2))
    return with_shadow(fill)


def build_font() -> None:
    src = Image.open(BUTANO_FONT)
    font = np.array(src).astype(np.int32)
    ascii_glyphs = font[: 94 * 8]  # '!' .. '~'
    glyphs: list[np.ndarray] = [ascii_glyphs[i * 8 : i * 8 + 8] for i in range(94)]
    utf8_chars: list[str] = []
    for ch, base, accent in FRENCH:
        glyphs.append(french_glyph(font, base, accent))
        utf8_chars.append(ch)
    for ch, rows in SYMBOLS.items():
        mask = np.zeros((8, 8), dtype=bool)
        stamp(mask, rows, 0, 0)
        glyphs.append(with_shadow(mask))
        utf8_chars.append(ch)

    stacked = np.vstack(glyphs)
    palette = [tuple(src.getpalette()[i * 3 : i * 3 + 3]) for i in range(16)]
    write_indexed_bmp(OUT / "slay_font.bmp", stacked, palette)  # type: ignore[arg-type]
    write_json(OUT / "slay_font.json", {"type": "sprite", "bpp_mode": "bpp_4", "height": 8})

    # Widths: keep Butano's hand-tuned ASCII widths, measure the new glyphs.
    header_src = (ROOT / "butano" / "common" / "include" / "common_variable_8x8_sprite_font.h").read_text()
    table = header_src.split("variable_8x8_sprite_font_character_widths[] = {")[1].split("};")[0]
    ascii_widths = [int(line.strip().split(",")[0]) for line in table.strip().splitlines()][:95]
    widths = list(ascii_widths)
    for g in glyphs[94:]:
        cols = np.where((g != 0).any(axis=0))[0]
        widths.append(min(8, int(cols.max()) + 2) if len(cols) else 4)

    # Text colour variants: same palette with the fill colour swapped.
    variants = {
        "white": (248, 248, 248), "red": (248, 88, 72), "green": (104, 232, 104), "gold": (248, 208, 72),
        "blue": (120, 184, 248), "gray": (152, 152, 168), "purple": (216, 152, 248),
    }

    def c555(rgb: tuple[int, int, int]) -> str:
        return f"bn::color({rgb[0] >> 3}, {rgb[1] >> 3}, {rgb[2] >> 3})"

    lines = [
        "// Generated by scripts/build_gba_art.py - do not edit by hand.",
        "#pragma once",
        "",
        '#include "bn_sprite_font.h"',
        '#include "bn_sprite_palette_item.h"',
        '#include "bn_utf8_characters_map.h"',
        '#include "bn_sprite_items_slay_font.h"',
        "",
        "namespace gba",
        "{",
        "",
        "constexpr bn::utf8_character slay_font_utf8_characters[] = {",
        "    " + ", ".join(f'"{c}"' for c in utf8_chars),
        "};",
        "",
        "constexpr int8_t slay_font_character_widths[] = {",
        "    " + ", ".join(str(w) for w in widths),
        "};",
        "",
        "constexpr bn::span<const bn::utf8_character> slay_font_utf8_characters_span(slay_font_utf8_characters);",
        "constexpr auto slay_font_utf8_characters_map = bn::utf8_characters_map<slay_font_utf8_characters_span>();",
        "",
        "constexpr bn::sprite_font slay_font(",
        "        bn::sprite_items::slay_font, slay_font_utf8_characters_map.reference(), slay_font_character_widths);",
        "",
    ]
    for vname, rgb in variants.items():
        cols = [c555(to_gba(p)) for p in palette]
        cols[FILL] = c555(to_gba(rgb))
        lines.append(f"constexpr bn::color slay_font_{vname}_colors[] = {{")
        lines.append("    " + ", ".join(cols))
        lines.append("};")
        lines.append(f"constexpr bn::sprite_palette_item slay_font_{vname}(slay_font_{vname}_colors, bn::bpp_mode::BPP_4);")
        lines.append("")
    lines += ["}", ""]
    FONT_HEADER.parent.mkdir(parents=True, exist_ok=True)
    FONT_HEADER.write_text("\n".join(lines))


def build_heroes() -> None:
    """Outlined, rim-lit copies of the hand-made hero sprites so they pop against painted backdrops."""
    for hero in HEROES:
        src = Image.open(OUT / f"player_{hero}.bmp")
        idx = np.array(src)
        rgb = np.array(src.convert("RGB"))
        opaque = idx != 0  # index 0 is the magenta transparent colour in these sprites
        img = np.zeros((64, 64, 4), dtype=np.uint8)
        img[..., :3] = rgb
        img[..., 3] = np.where(opaque, 255, 0)

        # Rim light on the top-left edges of the silhouette.
        edge = opaque & ~np.roll(opaque, 1, axis=0) | opaque & ~np.roll(opaque, 1, axis=1)
        lit = np.clip(rgb.astype(np.int32) + 60, 0, 255).astype(np.uint8)
        img[edge, :3] = lit[edge]

        # One pixel dark outline around the silhouette.
        grown = opaque.copy()
        grown[1:, :] |= opaque[:-1, :]
        grown[:-1, :] |= opaque[1:, :]
        grown[:, 1:] |= opaque[:, :-1]
        grown[:, :-1] |= opaque[:, 1:]
        ring = grown & ~opaque
        img[ring] = (12, 8, 16, 255)

        indices, palette = rgba_to_sprite(Image.fromarray(img, "RGBA"), 15)
        save_sprite(f"hero_{hero}", [indices], palette, 64)


def main() -> None:
    random.seed(1234)
    OUT.mkdir(exist_ok=True)
    build_font()
    build_monsters()
    build_cards()
    build_ui()
    build_heroes()
    for hero in HEROES:
        build_battle_bg(hero)
    build_scene_bgs()
    build_map_bg()
    print("GBA art generated in", OUT)


if __name__ == "__main__":
    main()
