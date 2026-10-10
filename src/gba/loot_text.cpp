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
    { "PAELS_TEARS", "Si vous terminez votre tour avec de l'Énergie non dépensée, gagnez 2 ● supplémentaires au tour suivant." },
    { "ART_OF_WAR", "Si vous ne jouez aucune Attaque pendant votre tour, gagnez 1 ● supplémentaire au tour suivant." },
    { "BAG_OF_MARBLES", "Au début de chaque combat, appliquez Vulnérable (1) sur TOUS les ennemis." },
    { "BAG_OF_PREPARATION", "Au début de chaque combat, piochez 2 cartes supplémentaires." },
    { "BELLOWS", "La première Main que vous piochez chaque combat est Améliorée." },
    { "BIG_HAT", "Au début de chaque combat, ajoutez 2 cartes Éthérées aléatoires à votre Main." },
    { "BLOOD_VIAL", "Au début de chaque combat, récupérez 2 PV." },
    { "BONE_FLUTE", "Chaque fois que Titos attaque, gagnez 2 d'Armure." },
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
    { "CIRCLET", "C'est un diadème." },
    { "CLOAK_CLASP", "À la fin de votre tour, gagnez 1 d'Armure pour chaque carte dans votre Main." },
    { "DATA_DISK", "Commencez chaque combat avec 1 de Focalisation." },
    { "BEATING_REMNANT", "Vous ne pouvez perdre plus de 20 PV en un seul tour." },
    { "BELT_BUCKLE", "Sans Potion, vous avez 2 de Dextérité supplémentaire." },
    { "DELICATE_FROND", "Au début de chaque combat, remplissez vos emplacements de potion vides avec des Potions aléatoires." },
    { "DEMON_TONGUE", "La première fois que vous perdez des PV pendant votre tour, vous récupérez autant de PV." },
    { "EMOTION_CHIP", "Si vous avez perdu des PV lors du tour précédent, déclenchez la capacité passive de chacun de vos Orbes au début de votre tour." },
    { "FENCING_MANUAL", "Au début de chaque combat, Forgez 10." },
    { "FESTIVE_POPPER", "Au début de chaque combat, infligez 9 dégâts à TOUS les ennemis." },
    { "FUNERARY_MASK", "Au début de chaque combat, ajoutez 3 Âmes à votre Pioche." },
    { "JEWELED_MASK", "Au début de chaque combat, placez un Pouvoir aléatoire de votre Pioche dans votre Main. Il coûte 0 ce combat." },
    { "GALACTIC_DUST", "Chaque fois que vous dépensez 10 ★, gagnez 10 d'Armure." },
    { "GAMBLING_CHIP", "Au début de chaque combat, défaussez autant de cartes que vous souhaitez, puis piochez-en le même nombre." },
    { "REPTILE_TRINKET", "Quand vous utilisez une potion, gagnez 3 Force ce tour." },
    { "GAME_PIECE", "Chaque fois que vous jouez un Pouvoir, piochez 1 carte." },
    { "GHOST_SEED", "Les Frappes et les Défenses deviennent Éthérées." },
    { "GOLD_PLATED_CABLES", "Votre orbe le plus à droite déclenche sa capacité passive une fois de plus." },
    { "GORGET", "Au début de chaque combat, gagnez 4 de Blindage." },
    { "GREMLIN_HORN", "Chaque fois qu'un ennemi est tué, gagnez 1 ● et piochez 1 carte." },
    { "HAPPY_FLOWER", "Tous les 3 tours, gagnez 1 ●." },
    { "HELICAL_DART", "Chaque fois que vous jouez un Surin, gagnez 1 de Dextérité jusqu'à la fin du tour." },
    { "HORN_CLEAT", "Au début de votre 2ème tour, gagnez 14 d'Armure." },
    { "PUMPKIN_CANDLE", "Gagnez 1 ● par tour pendant 5 combats." },
    { "ICE_CREAM", "Vous conservez votre Énergie entre chaque tour." },
    { "INTIMIDATING_HELMET", "Lorsque vous jouez une carte qui coûte 2 ● ou plus, gagnez 4 d'Armure." },
    { "IVORY_TILE", "Lorsque vous jouez une carte qui coûte 3 ● ou plus, gagnez 1 ●." },
    { "JOSS_PAPER", "Chaque fois que vous avez Épuisé 5 cartes, piochez 1 carte." },
    { "KUNAI", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 1 de Dextérité." },
    { "KUSARIGAMA", "Chaque fois que vous jouez 3 Attaques en un seul tour, infligez 6 dégâts à un ennemi aléatoire." },
    { "LANTERN", "Démarrez chaque combat avec 1 ● supplémentaire." },
    { "LETTER_OPENER", "Chaque fois que vous jouez 3 Compétences en un seul tour, infligez 5 dégâts à TOUS les ennemis." },
    { "LIZARD_TAIL", "Lorsque vos PV sont réduits à 0, récupérez 50 % de vos PV max (ne fonctionne qu'une fois)." },
    { "LORDS_PARASOL", "Chez le Marchand, vous obtenez TOUT ce qu'il propose." },
    { "LUNAR_PASTRY", "À la fin de votre tour, gagnez 1 ★." },
    { "MEAT_ON_THE_BONE", "Si vous avez 50% ou moins de vos PV Max à la fin d'un combat, récupérez 12 PV." },
    { "MEMBERSHIP_CARD", "50% de remise sur tous les produits !" },
    { "MERCURY_HOURGLASS", "Au début de votre tour, infligez 3 dégâts à TOUS les ennemis." },
    { "METRONOME", "La première fois que vous Canalisez 7 Orbes dans un combat, infligez 30 dégâts à TOUS les ennemis." },
    { "MINI_REGENT", "La première fois que vous dépensez des ★ chaque tour, gagnez 1 de Force." },
    { "MUMMIFIED_HAND", "Chaque fois que vous jouez un Pouvoir, le coût d'une carte aléatoire de votre Main est réduit à 0 jusqu'à la fin du tour." },
    { "NINJA_SCROLL", "Au début de chaque combat, ajoutez 3 Surins à votre Main." },
    { "NUNCHAKU", "Toutes les 10 Attaques jouées, vous gagnez 1 ●." },
    { "ODDLY_SMOOTH_STONE", "Démarrez chaque combat avec 1 de Dextérité." },
    { "ORICHALCUM", "Si vous terminez votre tour sans Armure, gagnez 6 d'Armure." },
    { "ORNAMENTAL_FAN", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 4 d'Armure." },
    { "PEN_NIB", "Toutes les 10 Attaques jouées, la 10ème inflige le double de dégâts." },
    { "STRIKE_DUMMY", "Les cartes contenant « Frappe » infligent 3 dégâts supplémentaires." },
    { "TUNGSTEN_ROD", "Vous perdez 1 PV de moins chaque fois que vous perdriez des PV." },
    { "FIDDLE", "Au début de chaque tour, piochez 2 cartes supplémentaires. Vous ne pouvez plus piocher de cartes pendant votre tour." },
    { "PAELS_BLOOD", "Au début de votre tour, piochez 1 carte supplémentaire." },
    { "PANTOGRAPH", "Au début de chaque combat de Boss, récupérez 25 PV." },
    { "PAELS_LEGION", "Double l'Armure reçue d'une carte, puis s'endort pendant 2 tours." },
    { "PARRYING_SHIELD", "Si vous terminez votre tour avec au moins 10 d'Armure, infligez 6 dégâts à un ennemi aléatoire." },
    { "PENDULUM", "Tous les 3 tours, piochez 1 carte." },
    { "PERMAFROST", "La première fois que vous jouez un Pouvoir chaque combat, gagnez 7 d'Armure." },
    { "POCKETWATCH", "Si vous avez joué 3 cartes ou moins durant votre tour, piochez 3 cartes supplémentaires au début du tour suivant." },
    { "POWER_CELL", "Au début de chaque combat, ajoutez 2 cartes de coût 0 de votre Pioche à votre Main." },
    { "RINGING_TRIANGLE", "Retenez votre Main le premier tour de chaque combat." },
    { "RAINBOW_RING", "La première fois de chaque tour que vous jouez une Attaque, une Compétence et un Pouvoir, gagnez 1 de Force et 1 de Dextérité." },
    { "RAZOR_TOOTH", "Chaque fois que vous jouez une Attaque ou une Compétence, Améliorez-la pour le restant du combat." },
    { "RED_MASK", "Au début de chaque combat, appliquez Affaibli (1) à TOUS les ennemis." },
    { "RED_SKULL", "Si vous êtes à 50% de PV ou moins, vous avez 3 de Force supplémentaire." },
    { "REGALITE", "La première fois que vous créez une carte chaque tour, gagnez 4 d'Armure." },
    { "RIPPLE_BASIN", "Si vous n'avez pas joué d'Attaques pendant votre tour, gagnez 4 d'Armure." },
    { "RUNIC_CAPACITOR", "Démarrez chaque combat avec 3 Emplacements d'Orbe supplémentaires." },
    { "SAND_CASTLE", "Lorsque vous obtenez cette relique, Améliorez 6 cartes aléatoires." },
    { "SCREAMING_FLAGON", "Si votre Main est vide à la fin de votre tour, infligez 20 dégâts à TOUS les ennemis." },
    { "SELF_FORMING_CLAY", "Chaque fois que vous perdez des PV en combat, gagnez 3 d'Armure au tour suivant." },
    { "DISTINGUISHED_CAPE", "Lorsque vous obtenez cette relique, ajoutez 2 Malédictions aléatoires et 3 Apparitions à votre Deck." },
    { "BLOOD_SOAKED_ROSE", "Lorsque vous obtenez cette relique, ajoutez 1 Assuétude à votre Deck. Gagnez 1 ● au début de chaque tour." },
    { "GOLDEN_PEARL", "Lorsque vous obtenez cette relique, gagnez 150 Pièces d'or." },
    { "CURSED_PEARL", "Lorsque vous obtenez cette relique, recevez Cupidité. Obtenez 333 Pièces d'or." },
    { "ECTOPLASM", "Vous ne pouvez plus obtenir de Pièces d'or. Gagnez 1 ● au début de chaque tour." },
    { "DUSTY_TOME", "Lorsque vous obtenez cette relique, obtenez une Carte d'Ancien." },
    { "GLASS_EYE", "Lorsque vous obtenez cette relique, obtenez 2 cartes Communes, 2 cartes Inhabituelles, et 1 carte Rare." },
    { "PRISMATIC_GEM", "Gagnez 1 ● au début de chaque tour. Les récompenses de carte contiennent désormais des cartes d'autres couleurs." },
    { "WHISPERING_EARRING", "Gagnez 1 ● au début de chaque tour. Vakuu joue votre premier tour à votre place." },
    { "BOOMING_CONCH", "Au début des combats d'Élite, piochez 2 cartes supplémentaires et gagnez 1 ●." },
    { "WHISPERING_EARRING","Gagnez 1 ● au début de chaque tour. Vakuu joue votre premier tour à votre place." },
    { "ARCANE_SCROLL", "Lorsque vous obtenez cette relique, ajoutez une Carte Rare aléatoire à votre Deck." },
    { "LEAD_PAPERWEIGHT", "Lorsque vous obtenez cette relique, choisissez 1 carte Incolore parmi 2 à ajouter à votre Deck." },
    { "LAVA_ROCK", "Le Boss de l'Acte 1 octroie 2 Reliques lorsqu'il est vaincu." },
    { "RUNIC_PYRAMID", "À la fin de votre tour, vous ne défaussez pas votre Main." },
    { "PHILOSOPHERS_STONE", "Gagnez 1 ● au début de chaque tour. TOUS les ennemis démarrent le combat avec 1 de Force." },
    { "VELVET_CHOKER", "Gagnez 1 ● au début de chaque tour. Vous ne pouvez pas jouer plus de 6 cartes par tour." },
    { "RADIANT_PEARL", "Au début de chaque combat, ajoutez 1 Luminescence à votre Main." },
    { "PAELS_EYE", "Si vous terminez un tour sans jouer de cartes (1 fois par combat), Épuisez votre Main et jouez un tour supplémentaire." },
    { "JEWELRY_BOX", "Lorsque vous obtenez cette relique, ajoutez 1 Apothéose à votre Deck." },
    { "NEOWS_BONES", "Lorsque vous obtenez cette relique, gagnez 2 Reliques de Neow aléatoires. Ajoutez 1 Malédiction aléatoire à votre Deck." },
    { "TANXS_WHISTLE", "Lorsque vous obtenez cette relique, ajoutez 1 Sifflet à votre Deck." },
    { "STORYBOOK", "Lorsque vous obtenez cette relique, ajoutez 1 Flamme éclatante à votre Deck." },
    { "NEOWS_TORMENT", "Lorsque vous obtenez cette relique, ajoutez 1 Fureur de Neow à votre Deck." },
    { "SIGNET_RING", "Lorsque vous obtenez cette relique, obtenez 888 Pièces d'or." },
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
    { "PAELS_TOOTH", "Retirez 5 cartes de votre Deck. Après chaque combat, rajoutez-en 1 aléatoirement ; elle est Améliorée." },
    { "WHITE_BEAST_STATUE", "Vous obtenez toujours une Potion dans les récompenses de combat." },
    { "WHITE_STAR", "Les Élites donnent une récompense de carte Rare supplémentaire." },
    { "WAR_HAMMER", "Lorsque vous tuez un Élite, Améliorez 4 cartes aléatoires." },
    { "STRAWBERRY", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 7." },
    { "PEAR", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 10." },
    { "SMALL_CAPSULE", "Lorsque vous obtenez cette relique, obtenez une Relique aléatoire." },
    { "LOOMING_FRUIT", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 31." },
    { "NUTRITIOUS_OYSTER", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 11." },
    { "LEES_WAFFLE", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 7 et récupérez tous vos PV." },
    { "MANGO", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 14." },
    { "STONE_HUMIDIFIER", "Lorsque vous vous Reposez à un Site de repos, augmentez vos PV Max de 5." },
    { "TINY_MAILBOX", "Lorsque vous vous Reposez, obtenez 2 Potions aléatoires." },
    { "LARGE_CAPSULE", "Lorsque vous obtenez cette relique, obtenez 2 Reliques aléatoires. Ajoutez une Frappe et une Défense à votre Deck." },
    { "CALLING_BELL", "Lorsque vous obtenez cette relique, obtenez une Malédiction spéciale et 3 Reliques." },
    { "CAULDRON", "Lorsque vous obtenez cette relique, concoctez 5 Potions aléatoires." },
    { "OLD_COIN", "Lorsque vous obtenez cette relique, obtenez 300 Pièces d'or." },
    { "WAR_PAINT", "Lorsque vous obtenez cette relique, Améliorez 2 Compétences aléatoires." },
    { "STURDY_CLAMP", "Vous conservez jusqu'à 10 d'Armure entre chaque tour." },
    { "SAI", "Au début de votre tour, gagnez 7 d'Armure." },
    { "SEAL_OF_GOLD", "Au début de votre tour, dépensez 3 Pièces d'or pour gagner 1 ●." },
    { "SYMBIOTIC_VIRUS", "Au début de chaque combat, Canalisez 1 Orbe ténébreux." },
    { "THE_COURIER", "Le Marchand renouvelle ce que vous achetez. Prix -20%." },
    { "THE_ABACUS", "Chaque fois que la Pioche est mélangée, gagnez 6 d'Armure." },
    { "TINGSHA", "Pour chaque carte que vous défaussez pendant votre tour, infligez 3 dégâts à un ennemi aléatoire." },
    { "TOUGH_BANDAGES", "Chaque fois que vous défaussez une carte pendant votre tour, gagnez 3 d'Armure." },
    { "TUNING_FORK", "Toutes les 10 Compétences que vous jouez, vous gagnez 7 d'Armure." },
    { "TWISTED_FUNNEL", "Au début de chaque combat, appliquez 4 de Poison à TOUS les ennemis." },
    { "UNCEASING_TOP", "Chaque fois que votre Main est vide pendant votre tour, piochez une carte." },
    { "VAJRA", "Démarrez chaque combat avec 1 de Force." },
    { "VERY_HOT_COCOA", "Démarrez chaque combat avec 4 ● supplémentaires." },
    { "VEXING_PUZZLEBOX", "Au début de chaque combat, ajoutez une carte aléatoire à votre Main. Elle coûte 0 ce tour." },
    { "YUMMY_COOKIE", "Lorsque vous obtenez cette relique, Améliorez 4 cartes." },
    { "WHETSTONE", "Lorsque vous obtenez cette relique, Améliorez 2 Attaques aléatoires." },
    { "NEOWS_TALISMAN", "Lorsque vous obtenez cette relique, Améliorez 1 de vos Frappes et 1 de vos Défenses." },
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
