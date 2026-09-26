#!/usr/bin/env python3
"""
Générateur des assets Pixel Art Mario-style pour la carte de la Tour Slay the GBA.
Produit des BMP 16 couleurs (4 bpp) avec Magenta (255, 0, 255) comme couleur 0 transparente.
"""
from PIL import Image, ImageDraw
import os

GRAPHICS_DIR = "graphics"

# Palette de 16 couleurs optimisée pour GBA
# Index 0 : Transparent (Magenta)
PALETTE = [
    255, 0, 255,    # 0: Transparent
    24, 20, 36,     # 1: Noir / Contour sombre
    248, 248, 248,  # 2: Blanc
    136, 140, 156,  # 3: Gris pierre
    208, 70, 72,    # 4: Rouge vif (Mario / Monstre)
    133, 40, 48,    # 5: Rouge sombre
    238, 195, 60,   # 6: Jaune d'or / Bloc ?
    210, 125, 44,   # 7: Orange / Flamme
    109, 194, 76,   # 8: Vert feuille
    52, 101, 36,    # 9: Vert sombre
    75, 128, 202,   # 10: Bleu ciel / Épée
    38, 64, 122,    # 11: Bleu nuit
    142, 82, 52,    # 12: Brun bois
    89, 44, 28,     # 13: Brun sombre
    218, 148, 120,  # 14: Peau / Chair
    180, 80, 200,   # 15: Violet néon / Élite
]
# Remplir jusqu'à 256 entrées (768 octets) pour le format palette standard BMP
while len(PALETTE) < 768:
    PALETTE.extend([0, 0, 0])

def create_indexed_image(width, height):
    img = Image.new("P", (width, height), 0)
    img.putpalette(PALETTE)
    return img

def save_sprite(img, name, height=None):
    if height is None:
        height = img.height
    bmp_path = os.path.join(GRAPHICS_DIR, f"{name}.bmp")
    json_path = os.path.join(GRAPHICS_DIR, f"{name}.json")
    img.save(bmp_path, "BMP")
    with open(json_path, "w") as f:
        f.write(f'{{\n    "type": "sprite",\n    "height": {height}\n}}\n')
    print(f"Generated {bmp_path} ({img.width}x{img.height})")

def make_monster_node():
    # 16x16 : Badge rouge circulaire avec épées croisées argentées
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Cercle de fond
    d.ellipse([1, 1, 14, 14], fill=4, outline=1)
    # Reflet
    d.arc([2, 2, 13, 13], 180, 270, fill=2)
    # Épée 1 (diagonale)
    d.line([4, 4, 11, 11], fill=2, width=1)
    d.point([(3, 3), (4, 3), (3, 4)], fill=1) # Garde
    # Épée 2 (diagonale inverse)
    d.line([11, 4, 4, 11], fill=2, width=1)
    d.point([(12, 3), (11, 3), (12, 4)], fill=1)
    # Centre
    d.point([(7, 7), (8, 8), (7, 8), (8, 7)], fill=1)
    return img

def make_elite_node():
    # 16x16 : Badge violet/or avec flamme et crâne cornu
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    d.ellipse([1, 1, 14, 14], fill=15, outline=6)
    # Flammes sur le dessus
    d.point([(7, 0), (8, 0), (6, 1), (9, 1)], fill=7)
    # Cornes
    d.point([(3, 3), (12, 3), (4, 4), (11, 4)], fill=6)
    # Crâne
    d.rectangle([5, 5, 10, 10], fill=2)
    # Yeux rouges
    d.point([(6, 7), (9, 7)], fill=4)
    # Mâchoire
    d.line([6, 11, 9, 11], fill=1)
    return img

def make_rest_node():
    # 16x16 : Feu de camp chaleureux sur bûches de bois
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Bûches
    d.line([2, 13, 13, 13], fill=12, width=2)
    d.line([4, 11, 11, 15], fill=13, width=1)
    # Flamme principale (orange et jaune)
    d.polygon([(7, 2), (10, 6), (11, 10), (8, 12), (4, 10), (5, 6)], fill=7, outline=1)
    d.polygon([(7, 5), (9, 8), (8, 11), (6, 10), (6, 7)], fill=6)
    # Étincelles
    d.point([(4, 3), (11, 4), (8, 1)], fill=6)
    return img

def make_shop_node():
    # 16x16 : Sac d'or bombé avec le signe $
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Lien du sac
    d.rectangle([6, 3, 9, 4], fill=4, outline=1)
    # Corps du sac
    d.ellipse([2, 5, 13, 14], fill=6, outline=1)
    # Signe $
    d.line([8, 7, 8, 12], fill=13)
    d.line([7, 8, 9, 8], fill=13)
    d.line([7, 10, 9, 10], fill=13)
    d.line([7, 12, 9, 12], fill=13)
    return img

def make_treasure_node():
    # 16x16 : Coffre au trésor en bois avec serrures dorées
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Corps du coffre
    d.rectangle([2, 6, 13, 13], fill=12, outline=1)
    # Couvercle arrondi
    d.ellipse([2, 3, 13, 8], fill=13, outline=1)
    # Ferrures d'or
    d.line([2, 8, 13, 8], fill=6)
    d.line([4, 6, 4, 13], fill=6)
    d.line([11, 6, 11, 13], fill=6)
    # Serrure dorée
    d.rectangle([7, 8, 8, 10], fill=6, outline=1)
    return img

def make_event_node():
    # 16x16 : Bloc mystère Mario '?' orné de rivets
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Bloc carré jaune d'or
    d.rectangle([1, 1, 14, 14], fill=6, outline=1)
    # Rivets aux 4 coins
    d.point([(2, 2), (13, 2), (2, 13), (13, 13)], fill=13)
    # Point d'interrogation blanc avec contour
    # Boucle haute du ?
    d.line([5, 4, 10, 4], fill=2)
    d.point([(10, 5), (10, 6)], fill=2)
    d.line([7, 7, 9, 7], fill=2)
    d.line([7, 8, 7, 9], fill=2)
    # Point du bas
    d.point([(7, 11), (8, 11)], fill=2)
    # Ombre intérieure
    d.line([2, 13, 13, 13], fill=13)
    d.line([13, 2, 13, 13], fill=13)
    return img

def make_boss_node():
    # 32x32 : Château forteresse imposant avec créneaux et yeux rouges
    img = create_indexed_image(32, 32)
    d = ImageDraw.Draw(img)
    # Base du château
    d.rectangle([4, 12, 27, 29], fill=3, outline=1)
    # Créneaux
    d.rectangle([4, 8, 9, 12], fill=3, outline=1)
    d.rectangle([13, 6, 18, 12], fill=3, outline=1) # Tour centrale plus haute
    d.rectangle([22, 8, 27, 12], fill=3, outline=1)
    # Portail d'entrée sombre
    d.polygon([(11, 29), (11, 20), (16, 17), (20, 20), (20, 29)], fill=1)
    # Bannières rouges
    d.rectangle([6, 14, 8, 20], fill=4)
    d.rectangle([23, 14, 25, 20], fill=4)
    # Yeux rouges de menace au-dessus du portail
    d.point([(14, 19), (17, 19)], fill=4)
    # Toit pointu tour centrale
    d.polygon([(11, 6), (15, 1), (16, 1), (20, 6)], fill=5, outline=1)
    # Étoile dorée au sommet
    d.point([(15, 0), (16, 0)], fill=6)
    return img

def make_pawn_ironclad():
    # 16x16 : Chibi Soldat de Fer - Casque cornu, cape rouge sang, grande épée
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Cornes de démon sur le casque (or/brun)
    d.line([3, 1, 4, 3], fill=6)
    d.line([12, 1, 11, 3], fill=6)
    d.point([(2, 0), (13, 0)], fill=13)
    # Casque rouge sang
    d.rectangle([4, 2, 11, 6], fill=4, outline=1)
    # Fente visière rougeoyante / blanche
    d.line([6, 4, 9, 4], fill=2)
    d.point([(5, 4), (10, 4)], fill=7)
    # Buste armure de fer sombre
    d.rectangle([5, 7, 10, 11], fill=3, outline=1)
    # Cape rougeoyante derrière
    d.rectangle([3, 7, 4, 12], fill=5)
    d.point([(4, 13), (3, 12)], fill=4)
    # Épée massive tenue à droite
    d.line([12, 5, 12, 12], fill=2)
    d.line([13, 5, 13, 12], fill=3)
    d.point([(11, 7), (14, 7)], fill=6) # Garde
    d.point([(12, 13)], fill=13) # Pommeau
    # Jambes armurées
    d.rectangle([5, 12, 6, 14], fill=1)
    d.rectangle([9, 12, 10, 14], fill=1)
    d.point([(5, 15), (9, 15)], fill=3) # Bottes
    return img

def make_pawn_silent():
    # 16x16 : Chibi Silencieuse - Masque crâne de bête cornu, cape verte émeraude, dagues de poison
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Cornes du crâne de bête
    d.point([(4, 1), (5, 2), (10, 2), (11, 1)], fill=1)
    d.point([(4, 0), (11, 0)], fill=2)
    # Capuche sombre autour de la tête
    d.rectangle([3, 2, 12, 6], fill=9)
    # Masque crâne blanc d'oiseau/bête
    d.polygon([(5, 2), (10, 2), (9, 6), (8, 7), (7, 7), (6, 6)], fill=2, outline=1)
    # Orbites noires
    d.point([(6, 4), (9, 4)], fill=1)
    # Bec / museau du crâne
    d.point([(7, 6), (8, 6)], fill=1)
    # Manteau et cape vert émeraude toxique
    d.polygon([(4, 7), (11, 7), (12, 12), (3, 12)], fill=8, outline=9)
    # Dague gauche (avec pointe de venin vert néon)
    d.line([2, 8, 3, 11], fill=3)
    d.point([(1, 7)], fill=8) # Goutte de poison
    # Dague droite
    d.line([13, 8, 12, 11], fill=3)
    d.point([(14, 7)], fill=8) # Goutte de poison
    # Bottes d'assassin furtif
    d.rectangle([5, 13, 6, 15], fill=1)
    d.rectangle([9, 13, 10, 15], fill=1)
    return img

def make_pawn_defect():
    # 16x16 : Chibi Défectueux - Châssis saphir, œil central doré, 2 orbes flottants (Éclair & Glace)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Orbe 1 Flottant (Éclair jaune/cyan, en haut à gauche)
    d.ellipse([1, 1, 4, 4], fill=6, outline=10)
    d.point([(2, 2)], fill=2)
    # Orbe 2 Flottant (Glace bleu ciel/blanc, en haut à droite)
    d.ellipse([11, 1, 14, 4], fill=10, outline=2)
    d.point([(12, 2)], fill=2)
    # Tête d'automate saphir
    d.rectangle([5, 2, 10, 6], fill=11, outline=1)
    # Œil optique central brillant jaune/cyan
    d.rectangle([7, 4, 8, 4], fill=6)
    d.point([(7, 3), (8, 3)], fill=2) # Reflet néon
    # Cou mécanique
    d.rectangle([7, 7, 8, 7], fill=3)
    # Torse robotique avec réacteur d'énergie cyan
    d.rectangle([5, 8, 10, 11], fill=11, outline=1)
    d.ellipse([6, 9, 9, 10], fill=10) # Cœur d'énergie
    d.point([(7, 9), (8, 10)], fill=2)
    # Bras métalliques articulés
    d.point([(4, 9), (4, 10), (11, 9), (11, 10)], fill=3)
    # Béquilles/jambes robotiques
    d.line([6, 12, 5, 15], fill=3, width=1)
    d.line([9, 12, 10, 15], fill=3, width=1)
    return img

def make_pawn_regent():
    # 16x16 : Chibi Régent - Couronne d'or avec rubis, manteau pourpre royal et hermine, sceptre d'étoiles
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Couronne royale à 3 pointes dorées avec rubis central
    d.polygon([(4, 3), (4, 1), (6, 3), (8, 0), (10, 3), (12, 1), (12, 3)], fill=6, outline=1)
    d.point([(8, 2)], fill=4) # Rubis étincelant
    # Visage noble
    d.rectangle([5, 4, 11, 6], fill=14)
    d.point([(6, 5), (10, 5)], fill=1) # Yeux
    # Col d'hermine blanche royale avec points
    d.rectangle([4, 7, 12, 8], fill=2, outline=1)
    d.point([(5, 8), (8, 8), (11, 8)], fill=1) # Hermine
    # Manteau royal pourpre
    d.rectangle([4, 9, 11, 12], fill=15, outline=1)
    # Sceptre astral tenu en main droite (étoile à 4 branches d'or)
    d.line([13, 3, 13, 13], fill=6)
    d.point([(12, 4), (14, 4), (13, 3), (13, 5)], fill=6) # Étoile
    d.point([(13, 4)], fill=2) # Brillance stellaire
    # Bottes impériales
    d.rectangle([5, 13, 6, 15], fill=1)
    d.rectangle([9, 13, 10, 15], fill=1)
    d.point([(5, 15), (9, 15)], fill=6) # Boucles d'or
    return img

def make_pawn_necrobinder():
    # 16x16 : Chibi Nécromancienne - Capuche sombre, yeux spectraux, bâton d'os et mini serviteur Osty
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Capuche sombre déchiquetée
    d.polygon([(4, 1), (11, 1), (12, 6), (3, 6)], fill=1)
    # Ombre du visage et lueur spectrale cyan
    d.rectangle([5, 3, 10, 5], fill=15)
    d.point([(6, 4), (9, 4)], fill=10) # Yeux spectraux cyan
    # Robe nécromantique lambeaux violets
    d.polygon([(4, 7), (10, 7), (11, 13), (3, 13)], fill=15, outline=1)
    d.line([4, 14, 10, 14], fill=1)
    # Bâton d'os en main gauche
    d.line([2, 4, 2, 13], fill=14)
    d.point([(1, 3), (3, 3), (2, 2)], fill=2) # Crâne pommeau
    # Mini squelette serviteur Osty à ses côtés à droite !
    d.rectangle([12, 9, 14, 11], fill=2, outline=1)
    d.point([(13, 10)], fill=1) # Œil d'Osty
    d.point([(15, 9), (14, 12), (13, 13)], fill=2) # Main d'Osty
    # Bottes d'outre-tombe
    d.point([(5, 15), (8, 15)], fill=1)
    return img

def make_cursor_ironclad():
    # 16x16 : Anneau flamme rouge & pointes de fer (Soldat de Fer)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Flèches flammes rouges et orangées
    d.polygon([(7, 0), (8, 0), (6, 3), (9, 3)], fill=4, outline=7)
    d.polygon([(7, 15), (8, 15), (6, 12), (9, 12)], fill=4, outline=7)
    d.polygon([(0, 7), (0, 8), (3, 6), (3, 9)], fill=4, outline=7)
    d.polygon([(15, 7), (15, 8), (12, 6), (12, 9)], fill=4, outline=7)
    d.point([(3, 3), (12, 3), (3, 12), (12, 12)], fill=3) # Rivets de fer
    return img

def make_cursor_silent():
    # 16x16 : Anneau venin émeraude & shuriken (Silencieuse)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Flèches vertes dagues toxiques
    d.polygon([(7, 0), (8, 0), (6, 3), (9, 3)], fill=8, outline=9)
    d.polygon([(7, 15), (8, 15), (6, 12), (9, 12)], fill=8, outline=9)
    d.polygon([(0, 7), (0, 8), (3, 6), (3, 9)], fill=8, outline=9)
    d.polygon([(15, 7), (15, 8), (12, 6), (12, 9)], fill=8, outline=9)
    d.point([(2, 2), (13, 2), (2, 13), (13, 13)], fill=8) # Gouttes de poison
    return img

def make_cursor_defect():
    # 16x16 : Viseur cybernétique cyan & saphir (Défectueux)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Coins de visée haute technologie
    d.line([1, 1, 4, 1], fill=10)
    d.line([1, 1, 1, 4], fill=10)
    d.line([14, 1, 11, 1], fill=10)
    d.line([14, 1, 14, 4], fill=10)
    d.line([1, 14, 4, 14], fill=10)
    d.line([1, 14, 1, 11], fill=10)
    d.line([14, 14, 11, 14], fill=10)
    d.line([14, 14, 14, 11], fill=10)
    # Réticules néon cyan et blanc
    d.point([(7, 1), (8, 1), (7, 14), (8, 14)], fill=2)
    d.point([(1, 7), (1, 8), (14, 7), (14, 8)], fill=2)
    return img

def make_cursor_regent():
    # 16x16 : Couronne royale d'or & étoiles pourpres (Régent)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Flèches couronnées dorées
    d.polygon([(7, 0), (8, 0), (6, 3), (9, 3)], fill=6, outline=15)
    d.polygon([(7, 15), (8, 15), (6, 12), (9, 12)], fill=6, outline=15)
    d.polygon([(0, 7), (0, 8), (3, 6), (3, 9)], fill=6, outline=15)
    d.polygon([(15, 7), (15, 8), (12, 6), (12, 9)], fill=6, outline=15)
    d.point([(3, 3), (12, 3), (3, 12), (12, 12)], fill=6) # Gemmes d'or
    d.point([(7, 2), (8, 2)], fill=4) # Rubis
    return img

def make_cursor_necrobinder():
    # 16x16 : Âmes spectrales violettes & cyan (Nécromancienne)
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    # Flammes spectrales violettes
    d.polygon([(7, 0), (8, 0), (6, 3), (9, 3)], fill=15, outline=10)
    d.polygon([(7, 15), (8, 15), (6, 12), (9, 12)], fill=15, outline=10)
    d.polygon([(15, 7), (15, 8), (12, 6), (12, 9)], fill=15, outline=10)
    d.point([(3, 3), (12, 3), (3, 12), (12, 12)], fill=10) # Feux follets cyan
    return img

def make_path_dot():
    # 8x8 : Pavé de chemin de pierre rond Mario
    img = create_indexed_image(8, 8)
    d = ImageDraw.Draw(img)
    d.ellipse([1, 1, 6, 6], fill=3, outline=1)
    d.point([(2, 2)], fill=2) # Reflet
    return img

def make_player_pawn():
    return make_pawn_ironclad()

def make_cursor_select():
    # 16x16 : Anneau sélecteur Mario clignotant
    img = create_indexed_image(16, 16)
    d = ImageDraw.Draw(img)
    d.polygon([(7, 0), (8, 0), (6, 2), (9, 2)], fill=6)
    d.polygon([(7, 15), (8, 15), (6, 13), (9, 13)], fill=6)
    d.polygon([(0, 7), (0, 8), (2, 6), (2, 9)], fill=6)
    d.polygon([(15, 7), (15, 8), (13, 6), (13, 9)], fill=6)
    d.point([(3, 3), (12, 3), (3, 12), (12, 12)], fill=6)
    return img

if __name__ == "__main__":
    os.makedirs(GRAPHICS_DIR, exist_ok=True)
    # Mario Map Nodes
    save_sprite(make_monster_node(), "map_node_monster", 16)
    save_sprite(make_elite_node(), "map_node_elite", 16)
    save_sprite(make_rest_node(), "map_node_rest", 16)
    save_sprite(make_shop_node(), "map_node_shop", 16)
    save_sprite(make_treasure_node(), "map_node_treasure", 16)
    save_sprite(make_event_node(), "map_node_event", 16)
    save_sprite(make_boss_node(), "map_node_boss", 32)
    save_sprite(make_path_dot(), "map_path_dot", 8)
    # Fallback / Generic
    save_sprite(make_player_pawn(), "map_player_pawn", 16)
    save_sprite(make_cursor_select(), "map_cursor_select", 16)
    # 5 Hero-Specific Pawns
    save_sprite(make_pawn_ironclad(), "map_pawn_ironclad", 16)
    save_sprite(make_pawn_silent(), "map_pawn_silent", 16)
    save_sprite(make_pawn_defect(), "map_pawn_defect", 16)
    save_sprite(make_pawn_regent(), "map_pawn_regent", 16)
    save_sprite(make_pawn_necrobinder(), "map_pawn_necrobinder", 16)
    # 5 Hero-Specific Cursors
    save_sprite(make_cursor_ironclad(), "map_cursor_ironclad", 16)
    save_sprite(make_cursor_silent(), "map_cursor_silent", 16)
    save_sprite(make_cursor_defect(), "map_cursor_defect", 16)
    save_sprite(make_cursor_regent(), "map_cursor_regent", 16)
    save_sprite(make_cursor_necrobinder(), "map_cursor_necrobinder", 16)
    print("Tous les sprites de carte et héros ont été créés avec succès !")
