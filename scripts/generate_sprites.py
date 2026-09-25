#!/usr/bin/env python3
import os
from PIL import Image, ImageDraw

os.makedirs("graphics", exist_ok=True)

# 16-color palette (4bpp standard GBA)
# Color 0: Pure Magenta (0xFF00FF) -> Transparent
# Color 1: Black / Deep Shadow (0x101018)
# Color 2: Dark Gray / Slate (0x303440)
# Color 3: Mid Gray / Steel (0x606878)
# Color 4: Light Gray / Highlight (0xA0A8B8)
# Color 5: White / Pure Light (0xFFFFFF)
# Color 6: Dark Red (0x801820)
# Color 7: Crimson Red (0xC82030)
# Color 8: Bright Orange/Red (0xF05030)
# Color 9: Dark Purple (0x301848)
# Color 10: Cultist Blue/Purple (0x503880)
# Color 11: Mystic Cyan (0x40B0D0)
# Color 12: Beak Yellow/Bone (0xD8C080)
# Color 13: Ground Dark (0x282030)
# Color 14: Ground Mid (0x484058)
# Color 15: Ground Highlight (0x706888)

palette = [
    255, 0, 255,    # 0: Transparent
    16, 16, 24,     # 1: Deep Black
    48, 52, 64,     # 2: Slate
    96, 104, 120,   # 3: Steel
    160, 168, 184,  # 4: Highlight Steel
    255, 255, 255,  # 5: White
    128, 24, 32,    # 6: Dark Red
    200, 32, 48,    # 7: Crimson Red
    240, 80, 48,    # 8: Bright Orange
    48, 24, 72,     # 9: Dark Purple
    80, 56, 128,    # 10: Cultist Purple
    64, 176, 208,   # 11: Mystic Cyan
    216, 192, 128,  # 12: Bone/Beak
    40, 32, 48,     # 13: Ground Dark
    72, 64, 88,     # 14: Ground Mid
    112, 104, 136   # 15: Ground High
]
# Pad palette to 256 colors (768 bytes) for BMP compatibility
while len(palette) < 768:
    palette.extend([0, 0, 0])

def save_bmp(im, path):
    im.putpalette(palette)
    im.save(path, format="BMP")
    print(f"Generated {path}")

# ==========================================
# 1. PLAYER BACK SPRITE (64x64) - Ironclad
# ==========================================
im_p = Image.new("P", (64, 64), 0)
d_p = ImageDraw.Draw(im_p)

# Silhouette & Body (Ironclad seen from behind 3/4)
# Cape (Crimson red)
d_p.polygon([(26, 24), (16, 60), (44, 62), (38, 24)], fill=7)
d_p.polygon([(28, 26), (20, 58), (34, 60), (32, 26)], fill=6) # Shadow folds in cape

# Big Greatsword on back / side
d_p.line([(42, 10), (48, 56)], fill=3, width=3)
d_p.line([(41, 10), (47, 56)], fill=4, width=1) # blade sheen
d_p.rectangle([(39, 44), (51, 47)], fill=12) # gold crossguard
d_p.line([(48, 48), (52, 60)], fill=1, width=2) # hilt

# Shoulders & Armor (Heavy iron plates)
d_p.ellipse([(20, 16), (36, 32)], fill=2) # Left pauldron
d_p.ellipse([(32, 16), (46, 32)], fill=1) # Right pauldron / shadow
d_p.ellipse([(22, 18), (34, 30)], fill=3) # pauldron highlight

# Head / Helmet from behind
d_p.ellipse([(26, 10), (38, 24)], fill=2) # Iron helmet back
d_p.polygon([(24, 14), (20, 6), (28, 12)], fill=12) # Left horn
d_p.polygon([(36, 14), (42, 8), (34, 12)], fill=12) # Right horn
d_p.ellipse([(28, 12), (36, 20)], fill=3) # Helmet highlight

# Outline / Contrast
d_p.line([(15, 60), (45, 62)], fill=1, width=1)

save_bmp(im_p, "graphics/player_back.bmp")

with open("graphics/player_back.json", "w") as f:
    f.write('{\n    "type": "sprite",\n    "height": 64\n}\n')

# ==========================================
# 2. ENEMY CULTIST SPRITE (64x64) - Cultist
# ==========================================
im_e = Image.new("P", (64, 64), 0)
d_e = ImageDraw.Draw(im_e)

# Cultist Robe & Body (Seen from front)
d_e.polygon([(32, 20), (14, 58), (48, 58)], fill=9) # Robe base
d_e.polygon([(32, 22), (20, 56), (42, 56)], fill=10) # Robe highlights

# Feathered Cloak / Wings (Spreading outward)
d_e.polygon([(18, 28), (6, 42), (22, 46)], fill=10) # Left wing
d_e.polygon([(44, 28), (58, 38), (40, 46)], fill=9)  # Right wing
d_e.polygon([(8, 40), (14, 50), (20, 44)], fill=11) # Feather cyan tips

# Cultist Bird Mask (Raven Skull / Beak)
d_e.ellipse([(24, 12), (40, 28)], fill=12) # Bone mask
d_e.polygon([(32, 24), (27, 36), (37, 36)], fill=12) # Long pointed beak
d_e.line([(32, 24), (32, 36)], fill=1, width=1) # Beak split

# Glowing Eyes
d_e.ellipse([(26, 18), (29, 21)], fill=11) # Cyan glowing left eye
d_e.ellipse([(35, 18), (38, 21)], fill=11) # Cyan glowing right eye
d_e.point([(27, 19), (36, 19)], fill=5)   # White pupil glint

# Sacrificial Curved Dagger raised in right hand
d_e.line([(48, 22), (56, 10)], fill=4, width=2) # Blade
d_e.line([(55, 9), (57, 12)], fill=5, width=1) # Tip glint
d_e.rectangle([(45, 23), (50, 26)], fill=12) # Guard
d_e.rectangle([(47, 26), (50, 30)], fill=1) # Hand

save_bmp(im_e, "graphics/enemy_cultist.bmp")

with open("graphics/enemy_cultist.json", "w") as f:
    f.write('{\n    "type": "sprite",\n    "height": 64\n}\n')

# ==========================================
# 3. PEDESTAL SPRITE (64x32) - Combat Floor
# ==========================================
im_g = Image.new("P", (64, 32), 0)
d_g = ImageDraw.Draw(im_g)

# Elliptical stone battle ground platform
d_g.ellipse([(4, 6), (60, 26)], fill=13)
d_g.ellipse([(6, 8), (58, 24)], fill=14)
d_g.ellipse([(12, 10), (52, 20)], fill=15)

save_bmp(im_g, "graphics/pedestal.bmp")

with open("graphics/pedestal.json", "w") as f:
    f.write('{\n    "type": "sprite",\n    "height": 32\n}\n')

print("All Pokémon-style sprites successfully generated!")
