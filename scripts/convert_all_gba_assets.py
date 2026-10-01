#!/usr/bin/env python3
# Script de conversion de tous les assets du jeu Slay the Spire 2 vers Butano GBA (16 couleurs, BMP)
import os, sys
from PIL import Image

def convert_asset(src_path, dst_bmp, dst_json, size=(32, 32), sprite_type="sprite"):
    os.makedirs(os.path.dirname(dst_bmp), exist_ok=True)
    if not os.path.exists(src_path):
        return False
    with Image.open(src_path) as img:
        img = img.convert("RGBA")
        img = img.resize(size, Image.Resampling.LANCZOS)
        quantized = img.convert("RGB").quantize(colors=16, method=Image.Quantize.MEDIANCUT)
        quantized.save(dst_bmp, "BMP")
    with open(dst_json, "w") as f:
        f.write(f'{{\n    "type": "{sprite_type}",\n    "height": {size[1]}\n}}\n')
    return True

if __name__ == "__main__":
    print("Converting core GBA gameplay sprites...")
    # Convert starter relics
    starters = ["burning_blood", "ring_of_the_snake", "cracked_core", "pure_water"]
    # Relics acting when a combat starts
    combat_start = ["blood_vial", "sling_of_courage", "lantern", "stone_cracker", "red_mask", "pantograph",
                    "oddly_smooth_stone", "festive_popper", "bag_of_marbles"]
    for s in starters + combat_start:
        src = f"bible/images/relics/{s}.webp"
        convert_asset(src, f"graphics/relics/relic_{s}.bmp", f"graphics/relics/relic_{s}.json", (32, 32))
    
    # Convert core potions
    potions = ["fire_potion", "block_potion", "strength_potion", "speed_potion"]
    for p in potions:
        src = f"bible/images/potions/{p}.webp"
        convert_asset(src, f"graphics/potions/potion_{p}.bmp", f"graphics/potions/potion_{p}.json", (32, 32))

    print("Core GBA gameplay sprites successfully generated in graphics/!")
