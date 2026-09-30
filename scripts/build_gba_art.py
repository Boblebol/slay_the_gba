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
}
BOSSES: dict[str, str] = {
    "ceremonial_beast": "ceremonial_beast",
}


def build_monsters() -> None:
    for name, src in MONSTERS.items():
        art = fit_art(Image.open(BIBLE / "monsters" / f"{src}.webp"), (62, 62))
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

@dataclass(frozen=True)
class CardArt:
    enum_name: str   # engine CardId name (lower case)
    owner: str       # hero owning the card (frame colour); "hero" = the ROM's own hero
    art: str         # art file relative to bible/images/cards/<owner>/ ; "{hero}" is substituted
    kind: str        # attack | skill | power
    costs: tuple[int, int]  # base cost, upgraded cost


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
    CardArt("falling_star", "regent", "falling_star", "attack", (1, 1)),
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
]

FRAME_COLOR = {"ironclad": "red", "silent": "green", "defect": "blue", "regent": "orange", "necrobinder": "pink"}
PORTRAIT_BOX = (47, 84, 547, 464)  # transparent window inside the 598x844 frame templates

# 3x5 digits baked into the mini cards' energy gem.
DIGITS = {
    "0": ["###", "#.#", "#.#", "#.#", "###"],
    "1": [".#.", "##.", ".#.", ".#.", "###"],
    "2": ["###", "..#", "###", "#..", "###"],
    "3": ["###", "..#", ".##", "..#", "###"],
}


def compose_card(card: CardArt, hero: str, size: tuple[int, int]) -> Image.Image:
    owner = hero if card.owner == "hero" else card.owner
    frame = Image.open(BIBLE / "card_template" / f"frame_{card.kind}_{FRAME_COLOR[owner]}.webp").convert("RGBA")
    full = Image.new("RGBA", frame.size, (0, 0, 0, 0))
    if card.art:
        art_path = BIBLE / "cards" / owner / (card.art.format(hero=hero) + ".webp")
        art = Image.open(art_path).convert("RGBA")
        x0, y0, x1, y1 = PORTRAIT_BOX
        art = art.resize((x1 - x0 + 1, y1 - y0 + 1), Image.Resampling.LANCZOS)
        full.paste(art, (x0, y0))
    full.alpha_composite(frame)
    card_img = full.resize(size, Image.Resampling.LANCZOS)
    card_img = ImageEnhance.Contrast(card_img).enhance(1.1)
    return card_img


HERO_RGB = {"ironclad": (200, 48, 40), "silent": (64, 160, 72), "defect": (56, 112, 216),
            "regent": (232, 152, 40), "necrobinder": (200, 96, 184)}

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
    """23x32 hand card drawn for legibility: full-bleed art, hero-coloured border, type icon."""
    owner = hero if card.owner == "hero" else card.owner
    border = HERO_RGB[owner]
    img = Image.new("RGBA", (23, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, 22, 31), radius=2, fill=(12, 10, 16, 255))
    d.rounded_rectangle((1, 1, 21, 30), radius=2, fill=lerp(border, (255, 255, 255), 0.25) + (255,))
    d.rectangle((2, 2, 20, 29), fill=border + (255,))
    d.rectangle((3, 3, 19, 28), fill=(52, 42, 38, 255))
    if card.art:
        art = Image.open(BIBLE / "cards" / owner / (card.art.format(hero=hero) + ".webp")).convert("RGBA")
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
    gem = Image.open(BIBLE / "card_template" / f"energy_{hero}.webp").convert("RGBA")
    return gem.resize((size, size), Image.Resampling.LANCZOS)


def build_cards() -> None:
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
            gem = energy_gem(owner, 16)
            canvas.alpha_composite(gem, (3, 0))
            idx, pal = rgba_to_sprite(canvas, 15)
            save_sprite(name, [idx], pal, 64)

        # Mini cards: every (card, upgraded) pair as frames of one item sharing a palette.
        minis: list[Image.Image] = []
        for card in CARDS:
            for upgraded in (False, True):
                canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
                body = compose_mini(card, hero)
                canvas.paste(body, (5, 0), body)
                owner = hero if card.owner == "hero" else card.owner
                gem = energy_gem(owner, 11)
                canvas.alpha_composite(gem, (1, 0))
                if card.enum_name != "none":
                    cost = str(card.costs[1] if upgraded else card.costs[0])
                    draw = ImageDraw.Draw(canvas)
                    for dy, row in enumerate(DIGITS[cost]):
                        for dx, ch in enumerate(row):
                            if ch == "#":
                                draw.point((5 + dx, 3 + dy), fill=(255, 255, 255, 255))
                    if upgraded:
                        # Small green "+" badge in the bottom-right corner.
                        for px, py in [(25, 26), (24, 27), (25, 27), (26, 27), (25, 28)]:
                            draw.point((px, py), fill=(96, 240, 96, 255))
                minis.append(canvas)
        # Shared palette weighted towards this hero's own cards.
        samples = []
        for i, img in enumerate(minis):
            arr = np.array(img)
            opaque = arr[:, :, :3][arr[:, :, 3] >= 110]
            weight = 3 if CARDS[i // 2].owner in ("hero", hero) else 1
            samples.extend([opaque] * weight)
        palette = quantize_opaque(np.vstack(samples), 15)
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


def build_ui() -> None:
    icons = [ICON_ATTACK, ICON_DEFEND, ICON_BUFF, ICON_DEBUFF, ICON_UNKNOWN, ICON_TARGET, ICON_HEART, ICON_GOLD]
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
