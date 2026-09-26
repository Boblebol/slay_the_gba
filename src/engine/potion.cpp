#include "engine/potion.h"

namespace engine {
namespace {

constexpr PotionData ALL_POTIONS[] = {
    { "AMBERGRIS", "Ambre gris", "Event", "AnyPlayer", "Récupérez 50% de vos PV Max. Si vous consommez la potion pendant un combat, jouez un tour supplémentaire." },
    { "ASHWATER", "Eau cendrée", "Uncommon", "AnyPlayer", "Épuisez autant de cartes de votre Main que vous souhaitez." },
    { "ATTACK_POTION", "Potion d'attaque", "Common", "AnyPlayer", "Choisissez 1 Attaque parmi 3 aléatoires à ajouter à votre Main. Elle coûte 0 ce tour." },
    { "BEETLE_JUICE", "Jus de coléoptère", "Rare", "AnyEnemy", "Les attaques de l'ennemi infligent 30% de dégâts en moins pendant les 4 prochains tours." },
    { "BLESSING_OF_THE_FORGE", "Bénédiction de la forge", "Uncommon", "AnyPlayer", "Améliorez toutes les cartes de votre Main pour le restant du combat." },
    { "BLOCK_POTION", "Potion d'armure", "Common", "AnyPlayer", "Gagnez 12 d'Armure." },
    { "BLOOD_POTION", "Potion sanguine", "Common", "AnyPlayer", "Récupérez 20% de vos PV Max." },
    { "BONE_BREW", "Infusion osseuse", "Uncommon", "AnyPlayer", "Invoquez pour 15." },
    { "BOTTLED_POTENTIAL", "Potentiel en bouteille", "Rare", "AnyPlayer", "Mélangez TOUTES vos cartes dans votre Pioche. Piochez 5 cartes." },
    { "CLARITY", "Extrait de lucidité", "Uncommon", "AnyPlayer", "Piochez 1 carte. Au début de vos 3 prochains tours, piochez 1 carte supplémentaire." },
    { "COLORLESS_POTION", "Potion incolore", "Common", "AnyPlayer", "Choisissez 1 carte Incolore parmi 3 aléatoires à ajouter à votre Main. Elle coûte 0 ce tour." },
    { "COSMIC_CONCOCTION", "Concoction cosmique", "Rare", "AnyPlayer", "Ajoutez 3 cartes Incolores Améliorées à votre Main." },
    { "CUNNING_POTION", "Potion de ruse", "Uncommon", "AnyPlayer", "Ajoutez 3 Surins Améliorés à votre Main." },
    { "CURE_ALL", "Remède universel", "Uncommon", "AnyPlayer", "Gagnez 1. Piochez 2 cartes." },
    { "DEXTERITY_POTION", "Potion de dextérité", "Common", "AnyPlayer", "Gagnez 2 de Dextérité." },
    { "DISTILLED_CHAOS", "Chaos distillé", "Rare", "AnyPlayer", "Jouez les 3 cartes du dessus de votre Pioche." },
    { "DROPLET_OF_PRECOGNITION", "Goutte de prescience", "Rare", "AnyPlayer", "Choisissez une carte de votre Pioche et ajoutez-la à votre Main." },
    { "DUPLICATOR", "Duplicateur", "Uncommon", "AnyPlayer", "La prochaine carte que vous jouez ce tour est jouée une fois de plus." },
    { "ENERGY_POTION", "Potion d'énergie", "Common", "AnyPlayer", "Gagnez 2." },
    { "ENTROPIC_BREW", "Décoction entropique", "Rare", "AnyPlayer", "Remplissez vos emplacements de potion libres avec des potions aléatoires." },
    { "ESSENCE_OF_DARKNESS", "Essence ténébreuse", "Rare", "AnyPlayer", "Canalisez un Orbe obscur pour chacun de vos emplacements d'Orbe." },
    { "EXPLOSIVE_AMPOULE", "Ampoule explosive", "Common", "AllEnemies", "Infligez 10 dégâts à TOUS les ennemis." },
    { "FAIRY_IN_A_BOTTLE", "Fée en bouteille", "Rare", "Self", "Si vos PV atteignent 0, défaussez cette potion et récupérez 30 % de vos PV Max." },
    { "FIRE_POTION", "Potion incendiaire", "Common", "AnyEnemy", "Infligez 20 dégâts." },
    { "FLEX_POTION", "Potion de vantardise", "Common", "AnyPlayer", "Gagnez 5 de Force. À la fin de votre tour, perdez 5 de Force." },
    { "FOCUS_POTION", "Potion de focalisation", "Common", "AnyPlayer", "Gagnez 2 de Focalisation." },
    { "FORTIFIER", "Fortifiant", "Uncommon", "AnyPlayer", "Triplez votre Armure." },
    { "FOUL_POTION", "Potion infecte", "Event", "Combat", "Infligez 12 dégâts à TOUS les joueurs et les ennemis. Peut être jetée sur le Marchand pour 100 Pièces d'or." },
    { "FRUIT_JUICE", "Jus de fruits", "Rare", "AnyPlayer", "Gagnez 5 PV Max." },
    { "FYSH_OIL", "Huile de poissâme", "Uncommon", "AnyPlayer", "Gagnez 1 de Force et 1 de Dextérité." },
    { "GAMBLERS_BREW", "Décoction du parieur", "Uncommon", "AnyPlayer", "Défaussez autant de cartes que vous le souhaitez, puis piochez-en le même nombre." },
    { "GHOST_IN_A_JAR", "Fantôme dans un bocal", "Rare", "AnyPlayer", "Gagnez Intangible (1)." },
    { "GIGANTIFICATION_POTION", "Potion de géantification", "Rare", "AnyPlayer", "La prochaine Attaque que vous jouez inflige le triple de dégâts." },
    { "GLOWWATER_POTION", "Potion luminescente", "Event", "AnyPlayer", "Épuisez votre Main. Piochez 10 cartes." },
    { "HEART_OF_IRON", "Cœur de fer", "Uncommon", "AnyPlayer", "Gagnez 7 de Blindage." },
    { "KINGS_COURAGE", "Courage du roi", "Uncommon", "AnyPlayer", "Forgez pour 15." },
    { "LIQUID_BRONZE", "Bronze liquide", "Uncommon", "AnyPlayer", "Gagnez 3 Épines." },
    { "LIQUID_MEMORIES", "Souvenirs liquides", "Rare", "AnyPlayer", "Ajoutez une carte de votre Défausse à votre Main. Elle coûte 0 ce tour." },
    { "LUCKY_TONIC", "Philtre chanceux", "Rare", "AnyPlayer", "Gagnez 1 Tampon." },
    { "MAZALETHS_GIFT", "Don de Mazaleth", "Rare", "AnyPlayer", "Gagnez 1 Rituel." },
    { "OROBIC_ACID", "Acide orobique", "Rare", "AnyPlayer", "Ajoutez une Attaque, une Compétence et un Pouvoir aléatoires à votre Main. Ils coûtent 0 ce tour." },
    { "POISON_POTION", "Potion de poison", "Common", "AnyEnemy", "Appliquez 6 de Poison." },
    { "POT_OF_GHOULS", "Goules en bouteille", "Rare", "AnyPlayer", "Ajoutez 2 Âmes à votre Main." },
    { "POTION_OF_BINDING", "Potion paralysante", "Uncommon", "AllEnemies", "Appliquez Affaibli (1) et Vulnérable (1) à TOUS les ennemis." },
    { "POTION_OF_CAPACITY", "Potion de stockage", "Uncommon", "AnyPlayer", "Gagnez 2 Emplacements d'Orbe." },
    { "POTION_OF_DOOM", "Potion macabre", "Common", "AnyEnemy", "Appliquez 33 de Fatalité." },
    { "POWDERED_DEMISE", "Poudre funeste", "Uncommon", "AnyEnemy", "L'ennemi perd 9 PV à la fin de chacun de ses tours." },
    { "POWER_POTION", "Potion de pouvoir", "Common", "AnyPlayer", "Choisissez 1 Pouvoir parmi 3 aléatoires à ajouter à votre Main. Il coûte 0 ce tour." },
    { "RADIANT_TINCTURE", "Lampée de radiance", "Uncommon", "AnyPlayer", "Gagnez 1. Gagnez  au début de vos 3 prochains tours." },
    { "REGEN_POTION", "Potion de régénération", "Uncommon", "AnyPlayer", "Gagnez 5 de Régénération." },
    { "SHACKLING_POTION", "Potion d'entrave", "Rare", "AllEnemies", "TOUS les ennemis perdent 7 de Force ce tour." },
    { "SHIP_IN_A_BOTTLE", "Navire en bouteille", "Rare", "AnyPlayer", "Gagnez 10 d'Armure. Au prochain tour, gagnez 10 d'Armure." },
    { "SKILL_POTION", "Potion de compétence", "Common", "AnyPlayer", "Choisissez 1 Compétence parmi 3 aléatoires à ajouter à votre Main. Elle coûte 0 ce tour." },
    { "SNECKO_OIL", "Huile de geckobra", "Rare", "AnyPlayer", "Piochez 7 cartes. Les cartes dans votre Main ont un coût aléatoire pendant ce tour." },
    { "SOLDIERS_STEW", "Ragoût militaire", "Rare", "AnyPlayer", "Toutes les cartes dont le nom contient Frappe gagnent Écho (1) pendant ce combat." },
    { "SPEED_POTION", "Potion de vitesse", "Common", "AnyPlayer", "Gagnez 5 de Dextérité. À la fin de votre tour, perdez 5 de Dextérité." },
    { "STABLE_SERUM", "Sérum de stabilité", "Uncommon", "AnyPlayer", "Retenez votre Main pendant les 2 prochains tours." },
    { "STAR_POTION", "Potion d'étoiles", "Common", "AnyPlayer", "Gagnez 3." },
    { "STRENGTH_POTION", "Potion de force", "Common", "AnyPlayer", "Gagnez 2 de Force." },
    { "SWIFT_POTION", "Potion de célérité", "Common", "AnyPlayer", "Piochez 3 cartes." },
    { "TOUCH_OF_INSANITY", "Goutte de folie", "Uncommon", "AnyPlayer", "Choisissez une carte de votre Main. Elle coûte 0 pour le restant du combat." },
    { "VULNERABLE_POTION", "Potion de vulnérabilité", "Common", "AnyEnemy", "Appliquez Vulnérable (3)." },
    { "WEAK_POTION", "Potion d'affaiblissement", "Common", "AnyEnemy", "Appliquez Affaibli (3)." },
};

} // namespace

const PotionData* find_potion(std::string_view id) {
    for (const auto& p : ALL_POTIONS) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

std::span<const PotionData> get_all_potions() {
    return std::span<const PotionData>(ALL_POTIONS);
}

} // namespace engine