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
    { "ART_OF_WAR", "Si vous ne jouez aucune Attaque pendant votre tour, gagnez 1 ● supplémentaire au tour suivant." },
    { "BAG_OF_MARBLES", "Au début de chaque combat, appliquez Vulnérable (1) sur TOUS les ennemis." },
    { "BAG_OF_PREPARATION", "Au début de chaque combat, piochez 2 cartes supplémentaires." },
    { "BELLOWS", "La première Main que vous piochez chaque combat est Améliorée." },
    { "BIG_HAT", "Au début de chaque combat, ajoutez 2 cartes Éthérées aléatoires à votre Main." },
    { "BLOOD_VIAL", "Au début de chaque combat, récupérez 2 PV." },
    { "BONE_FLUTE", "Chaque fois que Titos attaque, gagnez 2 d'Armure." },
    { "BOOK_REPAIR_KNIFE", "Chaque fois qu'un ennemi non-Serviteur succombe à Fatalité, récupérez 3 PV." },
    { "BREAD", "Au début de votre premier tour, perdez 2 ●. Au début de chaque tour suivant, gagnez 1 ●." },
    { "BRIMSTONE", "Au début de votre tour, vous gagnez 2 de Force et TOUS les ennemis gagnent 1 de Force." },
    { "BRONZE_SCALES", "Démarrez chaque combat avec 3 d'Épines." },
    { "BURNING_STICKS", "La première fois que vous Épuisez une Compétence chaque combat, ajoutez-en une copie à votre Main." },
    { "CANDELABRA", "Au début de votre 2ème tour, gagnez 2 ●." },
    { "CAPTAINS_WHEEL", "Au début de votre 3ème tour, gagnez 18 d'Armure." },
    { "CENTENNIAL_PUZZLE", "La première fois que vous perdez des PV chaque combat, piochez 3 cartes." },
    { "CHANDELIER", "Au début de votre 3ème tour, gagnez 3 ●." },
    { "CHARONS_ASHES", "Chaque fois que vous Épuisez une carte, infligez 3 dégâts à TOUS les ennemis." },
    { "CLOAK_CLASP", "À la fin de votre tour, gagnez 1 d'Armure pour chaque carte dans votre Main." },
    { "DATA_DISK", "Commencez chaque combat avec 1 de Focalisation." },
    { "DEMON_TONGUE", "La première fois que vous perdez des PV pendant votre tour, vous récupérez autant de PV." },
    { "EMOTION_CHIP", "Si vous avez perdu des PV lors du tour précédent, déclenchez la capacité passive de chacun de vos Orbes au début de votre tour." },
    { "FENCING_MANUAL", "Au début de chaque combat, Forgez 10." },
    { "FESTIVE_POPPER", "Au début de chaque combat, infligez 9 dégâts à TOUS les ennemis." },
    { "FUNERARY_MASK", "Au début de chaque combat, ajoutez 3 Âmes à votre Pioche." },
    { "GALACTIC_DUST", "Chaque fois que vous dépensez 10 ★, gagnez 10 d'Armure." },
    { "GAMBLING_CHIP", "Au début de chaque combat, défaussez autant de cartes que vous souhaitez, puis piochez-en le même nombre." },
    { "GAME_PIECE", "Chaque fois que vous jouez un Pouvoir, piochez 1 carte." },
    { "GOLD_PLATED_CABLES", "Votre orbe le plus à droite déclenche sa capacité passive une fois de plus." },
    { "GORGET", "Au début de chaque combat, gagnez 4 de Blindage." },
    { "GREMLIN_HORN", "Chaque fois qu'un ennemi est tué, gagnez 1 ● et piochez 1 carte." },
    { "HAPPY_FLOWER", "Tous les 3 tours, gagnez 1 ●." },
    { "HELICAL_DART", "Chaque fois que vous jouez un Surin, gagnez 1 de Dextérité jusqu'à la fin du tour." },
    { "HORN_CLEAT", "Au début de votre 2ème tour, gagnez 14 d'Armure." },
    { "ICE_CREAM", "Vous conservez votre Énergie entre chaque tour." },
    { "INTIMIDATING_HELMET", "Lorsque vous jouez une carte qui coûte 2 ● ou plus, gagnez 4 d'Armure." },
    { "IVORY_TILE", "Lorsque vous jouez une carte qui coûte 3 ● ou plus, gagnez 1 ●." },
    { "JOSS_PAPER", "Chaque fois que vous avez Épuisé 5 cartes, piochez 1 carte." },
    { "KUNAI", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 1 de Dextérité." },
    { "KUSARIGAMA", "Chaque fois que vous jouez 3 Attaques en un seul tour, infligez 6 dégâts à un ennemi aléatoire." },
    { "LANTERN", "Démarrez chaque combat avec 1 ● supplémentaire." },
    { "LETTER_OPENER", "Chaque fois que vous jouez 3 Compétences en un seul tour, infligez 5 dégâts à TOUS les ennemis." },
    { "LIZARD_TAIL", "Lorsque vos PV sont réduits à 0, récupérez 50 % de vos PV max (ne fonctionne qu'une fois)." },
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
    { "PANTOGRAPH", "Au début de chaque combat de Boss, récupérez 25 PV." },
    { "PARRYING_SHIELD", "Si vous terminez votre tour avec au moins 10 d'Armure, infligez 6 dégâts à un ennemi aléatoire." },
    { "PENDULUM", "Tous les 3 tours, piochez 1 carte." },
    { "PERMAFROST", "La première fois que vous jouez un Pouvoir chaque combat, gagnez 7 d'Armure." },
    { "POCKETWATCH", "Si vous avez joué 3 cartes ou moins durant votre tour, piochez 3 cartes supplémentaires au début du tour suivant." },
    { "POWER_CELL", "Au début de chaque combat, ajoutez 2 cartes de coût 0 de votre Pioche à votre Main." },
    { "RAINBOW_RING", "La première fois de chaque tour que vous jouez une Attaque, une Compétence et un Pouvoir, gagnez 1 de Force et 1 de Dextérité." },
    { "RAZOR_TOOTH", "Chaque fois que vous jouez une Attaque ou une Compétence, Améliorez-la pour le restant du combat." },
    { "RED_MASK", "Au début de chaque combat, appliquez Affaibli (1) à TOUS les ennemis." },
    { "RED_SKULL", "Si vous êtes à 50% de PV ou moins, vous avez 3 de Force supplémentaire." },
    { "REGALITE", "La première fois que vous créez une carte chaque tour, gagnez 4 d'Armure." },
    { "RIPPLE_BASIN", "Si vous n'avez pas joué d'Attaques pendant votre tour, gagnez 4 d'Armure." },
    { "RUNIC_CAPACITOR", "Démarrez chaque combat avec 3 Emplacements d'Orbe supplémentaires." },
    { "SCREAMING_FLAGON", "Si votre Main est vide à la fin de votre tour, infligez 20 dégâts à TOUS les ennemis." },
    { "SELF_FORMING_CLAY", "Chaque fois que vous perdez des PV en combat, gagnez 3 d'Armure au tour suivant." },
    { "SHURIKEN", "Chaque fois que vous jouez 3 Attaques en un seul tour, gagnez 1 de Force." },
    { "SLING_OF_COURAGE", "Démarrez chaque combat d'Élite avec 2 de Force." },
    { "SNECKO_SKULL", "Vous appliquez 1 de Poison en plus à chaque fois que vous appliquez du Poison." },
    { "SPARKLING_ROUGE", "Au début de votre 3ème tour, gagnez 1 de Force et 1 de Dextérité." },
    { "STONE_CALENDAR", "À la fin du tour 7, infligez 52 dégâts à TOUS les ennemis." },
    { "STONE_CRACKER", "Au début de chaque combat, Améliorez 2 cartes aléatoires de votre Pioche pour le restant du combat." },
    { "STRAWBERRY", "Lorsque vous obtenez cette relique, augmentez vos PV Max de 7." },
    { "STURDY_CLAMP", "Vous conservez jusqu'à 10 d'Armure entre chaque tour." },
    { "SYMBIOTIC_VIRUS", "Au début de chaque combat, Canalisez 1 Orbe ténébreux." },
    { "THE_ABACUS", "Chaque fois que la Pioche est mélangée, gagnez 6 d'Armure." },
    { "TINGSHA", "Pour chaque carte que vous défaussez pendant votre tour, infligez 3 dégâts à un ennemi aléatoire." },
    { "TOUGH_BANDAGES", "Chaque fois que vous défaussez une carte pendant votre tour, gagnez 3 d'Armure." },
    { "TUNING_FORK", "Toutes les 10 Compétences que vous jouez, vous gagnez 7 d'Armure." },
    { "TWISTED_FUNNEL", "Au début de chaque combat, appliquez 4 de Poison à TOUS les ennemis." },
    { "UNCEASING_TOP", "Chaque fois que votre Main est vide pendant votre tour, piochez une carte." },
    { "VAJRA", "Démarrez chaque combat avec 1 de Force." },
    { "VEXING_PUZZLEBOX", "Au début de chaque combat, ajoutez une carte aléatoire à votre Main. Elle coûte 0 ce tour." },
    { "YUMMY_COOKIE", "Lorsque vous obtenez cette relique, Améliorez 4 cartes." },
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
