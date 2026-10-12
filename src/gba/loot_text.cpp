#include "gba/loot_text.h"

#include "engine/potion.h"
#include "engine/relic.h"

namespace gba
{

namespace
{

struct RelicText
{
    std::string_view id;
    bn::string_view description;
};

// French text of bible/data/relic (descI18n) for the relics of engine::RunManager::active_relics(), with the
// energy and Star icons written with the glyphs of the font. The engine keeps the English templates.
constexpr RelicText RELIC_TEXTS[] = {
    { "AKABEKO", "Au début de chaque combat, gagnez 8 de Vigueur." },
    { "ANCHOR", "Démarrez chaque combat avec 10 d'Armure." },
    { "FAKE_ANCHOR", "Démarrez chaque combat avec 4 d'Armure." },
    { "PAELS_FLESH", "Gagnez 1 ● supplémentaire au début de votre 3ème tour et de chaque tour suivant." },
    { "PAELS_TEARS", "Si vous terminez votre tour avec de l'Énergie non dépensée, gagnez 2 ● supplémentaires au tour suivant." },
    { "PAPER_PHROG", "Les ennemis Vulnérables subissent 75% de dégâts supplémentaires au lieu de 50%." },
    { "IRON_CLUB", "Toutes les 4 cartes jouées, vous piochez 1 carte." },
    { "ART_OF_WAR", "Si vous ne jouez aucune Attaque pendant votre tour, gagnez 1 ● supplémentaire au tour suivant." },
    { "BAG_OF_MARBLES", "Au début de chaque combat, appliquez Vulnérable (1) sur TOUS les ennemis." },
    { "BAG_OF_PREPARATION", "Au début de chaque combat, piochez 2 cartes supplémentaires." },
    { "BELLOWS", "La première Main que vous piochez chaque combat est Améliorée." },
    { "BIG_HAT", "Au début de chaque combat, ajoutez 2 cartes Éthérées aléatoires à votre Main." },
    { "BIG_MUSHROOM", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 20. Au début de chaque combat, piochez 2 cartes en moins." },
    { "BLOOD_VIAL", "Au début de chaque combat, récupérez 2 PV." },
    { "ROYAL_POISON", "Au début de chaque combat, perdez 4 PV." },
    { "BONE_FLUTE", "Chaque fois que Titos attaque, gagnez 2 d'Armure." },
    { "BONE_TEA", "Au début du prochain combat, améliorez votre main de départ." },
    { "BOOK_REPAIR_KNIFE", "Chaque fois qu'un ennemi non-Serviteur succombe à Fatalité, récupérez 3 PV." },
    { "BOOKMARK", "À la fin de chaque tour, une carte Retenue aléatoire coûte 1 de moins jusqu'à ce qu'elle soit jouée." },
    { "BREAD", "Au début de votre premier tour, perdez 2 ●. Au début de chaque tour suivant, gagnez 1 ●." },
    { "BRIMSTONE", "Au début de votre tour, vous gagnez 2 de Force et TOUS les ennemis gagnent 1 de Force." },
    { "BRONZE_SCALES", "Démarrez chaque combat avec 3 d'Épines." },
    { "BURNING_STICKS", "La première fois que vous Épuisez une Compétence chaque combat, ajoutez-en une copie à votre Main." },
    { "CANDELABRA", "Au début de votre 2ème tour, gagnez 2 ●." },
    { "CAPTAINS_WHEEL", "Au début de votre 3ème tour, gagnez 18 d'Armure." },
    { "CENTENNIAL_PUZZLE", "La première fois que vous perdez des PV chaque combat, piochez 3 cartes." },
    { "CHANDELIER", "Au début de votre 3ème tour, gagnez 3 ●." },
    { "CHARONS_ASHES", "Chaque fois que vous Épuisez une carte, infligez 3 dégâts à TOUS les ennemis." },
    { "FORGOTTEN_SOUL", "Chaque fois que vous Épuisez une carte, infligez 1 dégât à un ennemi aléatoire." },
    { "CIRCLET", "C'est un diadème." },
    { "FAKE_MERCHANTS_RUG", "Une piètre imitation." },
    { "DINGY_RUG", "Les récompenses de carte peuvent proposer des cartes Incolores." },
    { "CLOAK_CLASP", "À la fin de votre tour, gagnez 1 d'Armure pour chaque carte dans votre Main." },
    { "DAUGHTER_OF_THE_WIND", "Chaque fois que vous jouez une Attaque, gagnez 1 d'Armure." },
    { "DATA_DISK", "Commencez chaque combat avec 1 de Focalisation." },
    { "DRAGON_FRUIT", "Chaque fois que vous gagnez des Pièces d'or, augmentez vos PV Max de 1." },
    { "BEATING_REMNANT", "Vous ne pouvez perdre plus de 20 PV en un seul tour." },
    { "INFUSED_CORE", "Au début de chaque combat, Canalisez 3 Orbes kéraunique. Les Orbes kéraunique infligent 1 dégât supplémentaire." },
    { "TOASTY_MITTENS", "Au début de votre tour, Épuisez 1 carte de votre Main et gagnez 1 de Force." },
    { "UNDYING_SIGIL", "Les ennemis affligés d'au moins autant de Fatalité que de PV infligent 50% de dégâts en moins." },
    { "UNSETTLING_LAMP", "La première carte de chaque combat qui inflige des Effets négatifs à un ennemi voit son effet doublé." },
    { "BELT_BUCKLE", "Sans Potion, vous avez 2 de Dextérité supplémentaire." },
    { "BOWLER_HAT", "Recevez 25 % de Pièces d'or supplémentaires." },
    { "VAMBRACE", "La première fois de chaque combat qu'une carte vous confère de l'Armure, doublez-en la quantité gagnée." },
    { "RUINED_HELMET", "La première fois que vous gagnez de la Force chaque combat, doublez-en la quantité." },
    { "DELICATE_FROND", "Au début de chaque combat, remplissez vos emplacements de potion vides avec des Potions aléatoires." },
    { "DEMON_TONGUE", "La première fois que vous perdez des PV pendant votre tour, vous récupérez autant de PV." },
    { "EMOTION_CHIP", "Si vous avez perdu des PV lors du tour précédent, déclenchez la capacité passive de chacun de vos Orbes au début de votre tour." },
    { "FAKE_BLOOD_VIAL", "Au début de chaque combat, récupérez 1 PV." },
    { "FENCING_MANUAL", "Au début de chaque combat, Forgez 10." },
    { "FESTIVE_POPPER", "Au début de chaque combat, infligez 9 dégâts à TOUS les ennemis." },
    { "FUNERARY_MASK", "Au début de chaque combat, ajoutez 3 Âmes à votre Pioche." },
    { "JEWELED_MASK", "Au début de chaque combat, placez un Pouvoir aléatoire de votre Pioche dans votre Main. Il coûte 0 ce combat." },
    { "GALACTIC_DUST", "Chaque fois que vous dépensez 10 ★, gagnez 10 d'Armure." },
    { "EMBER_TEA", "Au début des 5 prochains combats, gagnez 2 de Force." },
    { "GAMBLING_CHIP", "Au début de chaque combat, défaussez autant de cartes que vous souhaitez, puis piochez-en le même nombre." },
    { "REPTILE_TRINKET", "Quand vous utilisez une potion, gagnez 3 Force ce tour." },
    { "GAME_PIECE", "Chaque fois que vous jouez un Pouvoir, piochez 1 carte." },
    { "HAND_DRILL", "Lorsque vous venez à bout de l'Armure d'un ennemi, appliquez-lui 2 Vulnérable." },
    { "GHOST_SEED", "Les Frappes et les Défenses deviennent Éthérées." },
    { "GOLD_PLATED_CABLES", "Votre orbe le plus à droite déclenche sa capacité passive une fois de plus." },
    { "GORGET", "Au début de chaque combat, gagnez 4 de Blindage." },
    { "GREMLIN_HORN", "Chaque fois qu'un ennemi est tué, gagnez 1 ● et piochez 1 carte." },
    { "FAKE_HAPPY_FLOWER", "Tous les 5 tours, gagnez 1 ●." },
    { "HAPPY_FLOWER", "Tous les 3 tours, gagnez 1 ●." },
    { "HELICAL_DART", "Chaque fois que vous jouez un Surin, gagnez 1 de Dextérité jusqu'à la fin du tour." },
    { "HORN_CLEAT", "Au début de votre 2ème tour, gagnez 14 d'Armure." },
    { "PUMPKIN_CANDLE", "Gagnez 1 ● par tour pendant 5 combats." },
    { "TEA_OF_DISCOURTESY", "Au début du prochain combat, mélangez 2 Hébétement dans votre Pioche." },
    { "ICE_CREAM", "Vous conservez votre Énergie entre chaque tour." },
    { "INTIMIDATING_HELMET", "Lorsque vous jouez une carte qui coûte 2 ● ou plus, gagnez 4 d'Armure." },
    { "IVORY_TILE", "Lorsque vous jouez une carte qui coûte 3 ● ou plus, gagnez 1 ●." },
    { "JOSS_PAPER", "Chaque fois que vous avez Épuisé 5 cartes, piochez 1 carte." },
    { "KUNAI", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 1 de Dextérité." },
    { "KUSARIGAMA", "Chaque fois que vous jouez 3 Attaques en un seul tour, infligez 6 dégâts à un ennemi aléatoire." },
    { "LANTERN", "Démarrez chaque combat avec 1 ● supplémentaire." },
    { "LETTER_OPENER", "Chaque fois que vous jouez 3 Compétences en un seul tour, infligez 5 dégâts à TOUS les ennemis." },
    { "LAVA_LAMP", "À la fin de chaque combat, si vous n'avez subi aucun dégât, Améliorez toutes les récompenses de carte." },
    { "LIZARD_TAIL", "Lorsque vos PV sont réduits à 0, récupérez 50 % de vos PV max (ne fonctionne qu'une fois)." },
    { "LORDS_PARASOL", "Chez le Marchand, vous obtenez TOUT ce qu'il propose." },
    { "LUNAR_PASTRY", "À la fin de votre tour, gagnez 1 ★." },
    { "MEAT_ON_THE_BONE", "Si vous avez 50% ou moins de vos PV Max à la fin d'un combat, récupérez 12 PV." },
    { "MEMBERSHIP_CARD", "50% de remise sur tous les produits !" },
    { "MERCURY_HOURGLASS", "Au début de votre tour, infligez 3 dégâts à TOUS les ennemis." },
    { "MR_STRUGGLES", "Au début de votre tour, infligez des dégâts égaux au numéro du tour à TOUS les ennemis." },
    { "METRONOME", "La première fois que vous Canalisez 7 Orbes dans un combat, infligez 30 dégâts à TOUS les ennemis." },
    { "MINI_REGENT", "La première fois que vous dépensez des ★ chaque tour, gagnez 1 de Force." },
    { "MUMMIFIED_HAND", "Chaque fois que vous jouez un Pouvoir, le coût d'une carte aléatoire de votre Main est réduit à 0 jusqu'à la fin du tour." },
    { "NINJA_SCROLL", "Au début de chaque combat, ajoutez 3 Surins à votre Main." },
    { "DIAMOND_DIADEM", "Début de combat : 20 d'Armure. Vous gardez votre Armure au début de votre 2ème tour." },
    { "ORANGE_DOUGH", "Au début de chaque combat, ajoutez 2 cartes Incolores aléatoires à votre Main." },
    { "NUNCHAKU", "Toutes les 10 Attaques jouées, vous gagnez 1 ●." },
    { "ODDLY_SMOOTH_STONE", "Démarrez chaque combat avec 1 de Dextérité." },
    { "ORICHALCUM", "Si vous terminez votre tour sans Armure, gagnez 6 d'Armure." },
    { "ORNAMENTAL_FAN", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 4 d'Armure." },
    { "VITRUVIAN_MINION", "Les cartes Sbire infligent le double de dégâts et donnent le double d'Armure." },
    { "PEN_NIB", "Toutes les 10 Attaques jouées, la 10ème inflige le double de dégâts." },
    { "STRIKE_DUMMY", "Les cartes contenant « Frappe » infligent 3 dégâts supplémentaires." },
    { "FAKE_STRIKE_DUMMY", "Les cartes contenant « Frappe » infligent 1 dégât supplémentaire." },
    { "MINIATURE_CANNON", "Les Attaques améliorées infligent 3 dégâts supplémentaires." },
    { "MYSTIC_LIGHTER", "Les Attaques Envoûtées infligent 9 dégâts supplémentaires." },
    { "TUNGSTEN_ROD", "Vous perdez 1 PV de moins chaque fois que vous perdriez des PV." },
    { "PAPER_KRANE", "Les ennemis Affaiblis vous infligent 40% de dégâts en moins plutôt que 25%." },
    { "AMETHYST_AUBERGINE", "Les ennemis donnent 15 Pièces d'or supplémentaires." },
    { "FIDDLE", "Au début de chaque tour, piochez 2 cartes supplémentaires. Vous ne pouvez plus piocher de cartes pendant votre tour." },
    { "PAELS_BLOOD", "Au début de votre tour, piochez 1 carte supplémentaire." },
    { "PANTOGRAPH", "Au début de chaque combat de Boss, récupérez 25 PV." },
    { "PAELS_LEGION", "Double l'Armure reçue d'une carte, puis s'endort pendant 2 tours." },
    { "PARRYING_SHIELD", "Si vous terminez votre tour avec au moins 10 d'Armure, infligez 6 dégâts à un ennemi aléatoire." },
    { "POLLINOUS_CORE", "Tous les 4 tours, piochez 2 cartes." },
    { "PENDULUM", "Tous les 3 tours, piochez 1 carte." },
    { "PERMAFROST", "La première fois que vous jouez un Pouvoir chaque combat, gagnez 7 d'Armure." },
    { "POCKETWATCH", "Si vous avez joué 3 cartes ou moins durant votre tour, piochez 3 cartes supplémentaires au début du tour suivant." },
    { "POWER_CELL", "Au début de chaque combat, ajoutez 2 cartes de coût 0 de votre Pioche à votre Main." },
    { "RINGING_TRIANGLE", "Retenez votre Main le premier tour de chaque combat." },
    { "PRAYER_WHEEL", "Les ennemis normaux donnent une récompense de carte supplémentaire." },
    { "RING_OF_THE_DRAKE", "Au début de vos 3 premiers tours, piochez 2 cartes supplémentaires." },
    { "RAINBOW_RING", "La première fois de chaque tour que vous jouez une Attaque, une Compétence et un Pouvoir, gagnez 1 de Force et 1 de Dextérité." },
    { "RAZOR_TOOTH", "Chaque fois que vous jouez une Attaque ou une Compétence, Améliorez-la pour le restant du combat." },
    { "RED_MASK", "Au début de chaque combat, appliquez Affaibli (1) à TOUS les ennemis." },
    { "RED_SKULL", "Si vous êtes à 50% de PV ou moins, vous avez 3 de Force supplémentaire." },
    { "REGALITE", "La première fois que vous créez une carte chaque tour, gagnez 4 d'Armure." },
    { "REGAL_PILLOW", "Lorsque vous vous Reposez, récupérez 15 PV supplémentaires." },
    { "RIPPLE_BASIN", "Si vous n'avez pas joué d'Attaques pendant votre tour, gagnez 4 d'Armure." },
    { "RUNIC_CAPACITOR", "Démarrez chaque combat avec 3 Emplacements d'Orbe supplémentaires." },
    { "SAND_CASTLE", "Lorsque vous obtenez cette relique, Améliorez 6 cartes aléatoires." },
    { "SCREAMING_FLAGON", "Si votre Main est vide à la fin de votre tour, infligez 20 dégâts à TOUS les ennemis." },
    { "SELF_FORMING_CLAY", "Chaque fois que vous perdez des PV en combat, gagnez 3 d'Armure au tour suivant." },
    { "DISTINGUISHED_CAPE", "Lorsque vous obtenez cette relique, ajoutez 2 Malédictions aléatoires et 3 Apparitions à votre Deck." },
    { "BLOOD_SOAKED_ROSE", "Lorsque vous obtenez cette relique, ajoutez 1 Assuétude à votre Deck. Gagnez 1 ● au début de chaque tour." },
    { "GOLDEN_PEARL", "Lorsque vous obtenez cette relique, gagnez 150 Pièces d'or." },
    { "CURSED_PEARL", "Lorsque vous obtenez cette relique, recevez Cupidité. Obtenez 333 Pièces d'or." },
    { "NEOWS_SACRIFICE", "Lorsque vous obtenez cette relique, obtenez 1 Ambre gris et ajoutez 1 Culpabilité à votre Deck." },
    { "ECTOPLASM", "Vous ne pouvez plus obtenir de Pièces d'or. Gagnez 1 ● au début de chaque tour." },
    { "DARKSTONE_PERIAPT", "Chaque fois que vous obtenez une Malédiction, augmentez vos PV Max de 6." },
    { "DUSTY_TOME", "Lorsque vous obtenez cette relique, obtenez une Carte d'Ancien." },
    { "GLASS_EYE", "Lorsque vous obtenez cette relique, obtenez 2 cartes Communes, 2 cartes Inhabituelles, et 1 carte Rare." },
    { "PRISMATIC_GEM", "Gagnez 1 ● au début de chaque tour. Les récompenses de carte contiennent désormais des cartes d'autres couleurs." },
    { "WHISPERING_EARRING", "Gagnez 1 ● au début de chaque tour. Vakuu joue votre premier tour à votre place." },
    { "BOOMING_CONCH", "Au début des combats d'Élite, piochez 2 cartes supplémentaires et gagnez 1 ●." },
    { "WHISPERING_EARRING","Gagnez 1 ● au début de chaque tour. Vakuu joue votre premier tour à votre place." },
    { "ARCANE_SCROLL", "Lorsque vous obtenez cette relique, ajoutez une Carte Rare aléatoire à votre Deck." },
    { "LOST_COFFER", "Lorsque vous obtenez cette relique, gagnez 1 récompense de carte et obtenez 1 Potion aléatoire." },
    { "LEAD_PAPERWEIGHT", "Lorsque vous obtenez cette relique, choisissez 1 carte Incolore parmi 2 à ajouter à votre Deck." },
    { "BLACK_BLOOD", "À la fin de chaque combat, récupérez 12 PV." },
    { "LAVA_ROCK", "Le Boss de l'Acte 1 octroie 2 Reliques lorsqu'il est vaincu." },
    { "RUNIC_PYRAMID", "À la fin de votre tour, vous ne défaussez pas votre Main." },
    { "SOZU", "Gagnez 1 ● au début de chaque tour. Vous ne pouvez plus obtenir de Potions." },
    { "SPIKED_GAUNTLETS", "Gagnez 1 ● au début de chaque tour. Les Pouvoirs coûtent 1 ● de plus." },
    { "PHILOSOPHERS_STONE", "Gagnez 1 ● au début de chaque tour. TOUS les ennemis démarrent le combat avec 1 de Force." },
    { "VELVET_CHOKER", "Gagnez 1 ● au début de chaque tour. Vous ne pouvez pas jouer plus de 6 cartes par tour." },
    { "RADIANT_PEARL", "Au début de chaque combat, ajoutez 1 Luminescence à votre Main." },
    { "THROWING_AXE", "La première carte que vous jouez chaque combat est jouée une fois de plus." },
    { "MUSIC_BOX", "Crée une copie Éthérée de la première Attaque jouée chaque tour." },
    { "BRILLIANT_SCARF", "La 5ème carte jouée chaque tour ne coûte rien.", },
    { "PAELS_EYE", "Si vous terminez un tour sans jouer de cartes (1 fois par combat), Épuisez votre Main et jouez un tour supplémentaire." },
    { "JEWELRY_BOX", "Lorsque vous obtenez cette relique, ajoutez 1 Apothéose à votre Deck." },
    { "NEOWS_BONES", "Lorsque vous obtenez cette relique, gagnez 2 Reliques de Neow aléatoires. Ajoutez 1 Malédiction aléatoire à votre Deck." },
    { "TANXS_WHISTLE", "Lorsque vous obtenez cette relique, ajoutez 1 Sifflet à votre Deck." },
    { "STORYBOOK", "Lorsque vous obtenez cette relique, ajoutez 1 Flamme éclatante à votre Deck." },
    { "NEOWS_TORMENT", "Lorsque vous obtenez cette relique, ajoutez 1 Fureur de Neow à votre Deck." },
    { "SIGNET_RING", "Lorsque vous obtenez cette relique, obtenez 888 Pièces d'or." },
    { "SILVER_CRUCIBLE", "Vos 3 premières récompenses de carte sont Améliorées. Le premier Coffre trésor que vous ouvrez est vide." },
    { "SERE_TALON", "Lorsque vous obtenez cette relique, perdez 9 PV Max. Ajoutez 3 Vœux à votre Deck." },
    { "SHURIKEN", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 1 de Force." },
    { "SLING_OF_COURAGE", "Démarrez chaque combat d'Élite avec 2 de Force." },
    { "SNECKO_SKULL", "Vous appliquez 1 de Poison en plus à chaque fois que vous appliquez du Poison." },
    { "SPARKLING_ROUGE", "Au début de votre 3ème tour, gagnez 1 de Force et 1 de Dextérité." },
    { "STONE_CALENDAR", "À la fin du tour 7, infligez 52 dégâts à TOUS les ennemis." },
    { "STONE_CRACKER", "Au début de chaque combat, Améliorez 2 cartes aléatoires de votre Pioche pour le restant du combat." },
    { "GOLDEN_COMPASS", "Lorsque vous obtenez cette relique, remplacez la carte de l'Acte 2 par un chemin unique." },
    { "CLAWS", "Lorsque vous obtenez cette relique, Transformez jusqu'à 6 cartes en Mutilation." },
    { "NEW_LEAF", "Lorsque vous obtenez cette relique, Transformez 1 carte." },
    { "POMANDER", "Lorsque vous obtenez cette relique, Améliorez une carte." },
    { "ASTROLABE", "Lorsque vous obtenez cette relique, Transformez 3 cartes, puis Améliorez-les." },
    { "PANDORAS_BOX", "Transformez TOUTES vos Frappes et vos Défenses." },
    { "PAELS_HORN", "Lorsque vous obtenez cette relique, ajoutez 2 Relaxations à votre Deck." },
    { "PRESERVED_FOG", "Retirez 3 cartes de votre Deck. Ajoutez Hubris à votre Deck." },
    { "PRECISE_SCISSORS", "Lorsque vous obtenez cette relique, retirez 1 carte de votre Deck." },
    { "EMPTY_CAGE", "Lorsque vous obtenez cette relique, retirez 2 cartes de votre Deck." },
    { "BIIIG_HUG", "Lorsque vous obtenez cette relique, retirez 4 cartes de votre Deck. Lorsque vous mélangez votre Pioche, ajoutez une Suie à votre Pioche." },
    { "BEAUTIFUL_BRACELET", "Lorsque vous obtenez cette relique, choisissez 4 cartes dans votre Deck. Envoûtez-les avec Célérité 2." },
    { "PRECARIOUS_SHEARS", "Lorsque vous obtenez cette relique, retirez 2 cartes de votre Deck et perdez 16 PV." },
    { "PAELS_TOOTH", "Retirez 5 cartes de votre Deck. Après chaque combat, rajoutez-en 1 aléatoirement ; elle est Améliorée." },
    { "WHITE_BEAST_STATUE", "Vous obtenez toujours une Potion dans les récompenses de combat." },
    { "WHITE_STAR", "Les Élites donnent une récompense de carte Rare supplémentaire." },
    { "FISHING_ROD", "Tous les 3 combats normaux, Améliorez une carte aléatoire de votre Deck." },
    { "WAR_HAMMER", "Lorsque vous tuez un Élite, Améliorez 4 cartes aléatoires." },
    { "STRAWBERRY", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 7." },
    { "PEAR", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 10." },
    { "SMALL_CAPSULE", "Lorsque vous obtenez cette relique, obtenez une Relique aléatoire." },
    { "LOOMING_FRUIT", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 31." },
    { "NUTRITIOUS_OYSTER", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 11." },
    { "LEES_WAFFLE", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 7 et récupérez tous vos PV." },
    { "MANGO", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 14." },
    { "FAKE_LEES_WAFFLE", "Lorsque vous obtenez cette relique, récupérez 10% de vos PV." },
    { "FAKE_MANGO", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 3." },
    { "CHOSEN_CHEESE", "À la fin de chaque combat, augmentez vos PV Max de 1." },
    { "STONE_HUMIDIFIER", "Lorsque vous vous Reposez à un Site de repos, augmentez vos PV Max de 5." },
    { "TINY_MAILBOX", "Lorsque vous vous Reposez, obtenez 2 Potions aléatoires." },
    { "DREAM_CATCHER", "Lorsque vous vous Reposez, vous pouvez ajouter une carte à votre Deck." },
    { "VENERABLE_TEA_SET", "Chaque fois que vous entrez dans un Site de repos, commencez le combat suivant avec 2 Énergie supplémentaires." },
    { "PLANISPHERE", "Lorsque vous entrez dans une salle ?, récupérez 5 PV." },
    { "MEAL_TICKET", "Lorsque vous entrez dans une boutique, récupérez 15 PV." },
    { "SHOVEL", "Vous pouvez Fouiller les Sites de repos pour obtenir une Relique aléatoire." },
    { "LARGE_CAPSULE", "Lorsque vous obtenez cette relique, obtenez 2 Reliques aléatoires. Ajoutez une Frappe et une Défense à votre Deck." },
    { "CALLING_BELL", "Lorsque vous obtenez cette relique, obtenez une Malédiction spéciale et 3 Reliques." },
    { "ALCHEMICAL_COFFER", "Lorsque vous obtenez cette relique, gagnez 4 nouveaux emplacements de potion avec des Potions aléatoires." },
    { "CAULDRON", "Lorsque vous obtenez cette relique, concoctez 5 Potions aléatoires." },
    { "ELECTRIC_SHRYMP", "Lorsque vous obtenez cette relique, Envoûtez une Compétence avec Imprégné." },
    { "PHIAL_HOLSTER", "Lorsque vous obtenez cette relique, gagnez 1 emplacement de potion et obtenez 2 Potions aléatoires." },
    { "CHEMICAL_X", "Les effets de vos cartes de coût X sont augmentés de 2." },
    { "LOST_WISP", "Chaque fois que vous jouez un Pouvoir, infligez 8 dégâts à TOUS les ennemis." },
    { "OLD_COIN", "Lorsque vous obtenez cette relique, obtenez 300 Pièces d'or." },
    { "BING_BONG", "Chaque fois que vous ajoutez une carte à votre Deck, ajoutez-en une copie supplémentaire." },
    { "LUCKY_FYSH", "Chaque fois que vous ajoutez une carte à votre Deck, obtenez 15 Pièces d'or." },
    { "BOOK_OF_FIVE_RINGS", "Chaque fois que vous ajoutez 5 cartes à votre Deck, récupérez 20 PV." },
    { "WAR_PAINT", "Lorsque vous obtenez cette relique, Améliorez 2 Compétences aléatoires." },
    { "FRAGRANT_MUSHROOM", "Lorsque vous obtenez cette relique, perdez 15 PV et Améliorez 2 cartes aléatoires." },
    { "DIVINE_DESTINY", "Au début de chaque combat, gagnez 7 Étoiles." },
    { "ETERNAL_FEATHER", "Pour toutes les 5 cartes dans votre Deck, récupérez 3 PV lorsque vous entrez dans un Site de repos." },
    { "FROZEN_EGG", "Lorsque vous ajoutez un Pouvoir à votre Deck, Améliorez-le." },
    { "GIRYA", "Aux Sites de repos, vous pouvez gagner 1 de Force. (3 fois maximum)" },
    { "MAW_BANK", "Chaque fois que vous gravissez un étage, obtenez 12 Pièces d'or. Cesse de fonctionner dès que vous dépensez des Pièces d'or chez le Marchand." },
    { "PETRIFIED_TOAD", "Au début de chaque combat, obtenez une Pierre en forme de potion." },
    { "STURDY_CLAMP", "Vous conservez jusqu'à 10 d'Armure entre chaque tour." },
    { "SAI", "Au début de votre tour, gagnez 7 d'Armure." },
    { "SEAL_OF_GOLD", "Au début de votre tour, dépensez 3 Pièces d'or pour gagner 1 ●." },
    { "SYMBIOTIC_VIRUS", "Au début de chaque combat, Canalisez 1 Orbe ténébreux." },
    { "ORRERY", "Lorsque vous obtenez cette relique, obtenez 5 récompenses de carte." },
    { "THE_BOOT", "Chaque fois que vous auriez infligé 4 dégâts d'attaque ou moins (sans armure), vous infligez 5 dégâts à la place." },
    { "THE_COURIER", "Le Marchand renouvelle ce que vous achetez. Prix -20%." },
    { "THE_ABACUS", "Chaque fois que la Pioche est mélangée, gagnez 6 d'Armure." },
    { "TRI_BOOMERANG", "Choisissez 3 Attaques de votre Deck. Envoûtez-les avec Instinct." },
    { "TINGSHA", "Pour chaque carte que vous défaussez pendant votre tour, infligez 3 dégâts à un ennemi aléatoire." },
    { "CHOICES_PARADOX", "Au début de chaque combat, choisissez 1 carte parmi 5 cartes aléatoires et ajoutez-la à votre Main avec Retenue." },
    { "TOOLBOX", "Au début de chaque combat, choisissez 1 carte Incolore aléatoire parmi 3 et ajoutez-la à votre Main." },
    { "TOUGH_BANDAGES", "Chaque fois que vous défaussez une carte pendant votre tour, gagnez 3 d'Armure." },
    { "TUNING_FORK", "Toutes les 10 Compétences que vous jouez, vous gagnez 7 d'Armure." },
    { "TWISTED_FUNNEL", "Au début de chaque combat, appliquez 4 de Poison à TOUS les ennemis." },
    { "CROSSBOW", "Au début de votre tour, ajoutez une Attaque aléatoire à votre Main. Elle coûte 0 ce tour." },
    { "HISTORY_COURSE", "Au début de votre tour, jouez une copie de la dernière Attaque que vous avez jouée." },
    { "UNCEASING_TOP", "Chaque fois que votre Main est vide pendant votre tour, piochez une carte." },
    { "SWORD_OF_JADE", "Démarrez chaque combat avec 3 de Force." },
    { "VAJRA", "Démarrez chaque combat avec 1 de Force." },
    { "VERY_HOT_COCOA", "Démarrez chaque combat avec 4 ● supplémentaires." },
    { "VEXING_PUZZLEBOX", "Au début de chaque combat, ajoutez une carte aléatoire à votre Main. Elle coûte 0 ce tour." },
    { "YUMMY_COOKIE", "Lorsque vous obtenez cette relique, Améliorez 4 cartes." },
    { "POTION_BELT", "Lorsque vous obtenez cette relique, gagnez 2 emplacements de potion." },
    { "WHETSTONE", "Lorsque vous obtenez cette relique, Améliorez 2 Attaques aléatoires." },
    { "MOLTEN_EGG", "Lorsque vous ajoutez une Attaque à votre Deck, Améliorez-la." },
    { "NEOWS_TALISMAN", "Lorsque vous obtenez cette relique, Améliorez 1 de vos Frappes et 1 de vos Défenses." },
    { "TOXIC_EGG", "Lorsque vous ajoutez une Compétence à votre Deck, Améliorez-la." },
};

bn::string_view to_bn(std::string_view text)
{
    return bn::string_view(text.data(), int(text.size()));
}

}

bn::string_view relic_name(std::string_view relic_id)
{
    const engine::RelicData* relic = engine::find_relic(relic_id);
    return to_bn(relic ? relic->name : relic_id);
}

bn::string_view relic_kind(std::string_view relic_id)
{
    const engine::RelicData* relic = engine::find_relic(relic_id);

    switch(relic ? relic->rarity : engine::RelicRarity::UNKNOWN)
    {
    case engine::RelicRarity::COMMON:
        return "Relique commune";
    case engine::RelicRarity::UNCOMMON:
        return "Relique peu commune";
    case engine::RelicRarity::RARE:
        return "Relique rare";
    case engine::RelicRarity::SHOP:
        return "Relique du marchand";
    default:
        return "Relique";
    }
}

bn::string_view potion_name(std::string_view potion_id)
{
    const engine::PotionData* potion = engine::find_potion(potion_id);
    return to_bn(potion ? potion->name : potion_id);
}

bn::string_view potion_kind(std::string_view potion_id)
{
    const engine::PotionData* potion = engine::find_potion(potion_id);

    if(potion && potion->rarity == "Common")
    {
        return "Potion commune";
    }

    if(potion && potion->rarity == "Uncommon")
    {
        return "Potion peu commune";
    }

    if(potion && potion->rarity == "Rare")
    {
        return "Potion rare";
    }

    return "Potion";
}

bn::string_view potion_description(std::string_view potion_id)
{
    const engine::PotionData* potion = engine::find_potion(potion_id);
    return potion ? to_bn(potion->description) : bn::string_view();
}

bn::string_view relic_description(std::string_view relic_id)
{
    for(const RelicText& text : RELIC_TEXTS)
    {
        if(text.id == relic_id)
        {
            return text.description;
        }
    }

    return bn::string_view();
}

}
