#include "engine/ancient.h"

namespace engine {
namespace {

constexpr AncientRelic RELICS_NEOW_0[] = {
    { "ARCANE_SCROLL", "Parchemin arcanique", RelicTier::TIER_C, "Obtenez une Carte Rare aléatoire." },
    { "BOOMING_CONCH", "Conque retentissante", RelicTier::TIER_B, "Au début des combats d'Élite, piochez 2 cartes supplémentaires et gagnez 1." },
    { "FISHING_ROD", "Canne à pêche", RelicTier::UNTIERED, "Tous les 3 combats normaux, Améliorez une carte aléatoire de votre Deck." },
    { "GOLDEN_PEARL", "Perle dorée", RelicTier::TIER_A, "Lorsque vous obtenez cette relique, gagnez 150 Pièces d'or." },
    { "KALEIDOSCOPE", "Kaléidoscope", RelicTier::UNTIERED, "Recevez 2 récompenses de cartes d'autres personnages." },
    { "LEAD_PAPERWEIGHT", "Presse-papier en plomb", RelicTier::TIER_A, "Choisissez 1 carte Incolore parmi 2 à ajoutez à votre Deck." },
    { "LOST_COFFER", "Coffret perdu", RelicTier::TIER_S, "Gagnez 1 récompense de carte et obtenez 1 Potion aléatoire." },
    { "MASSIVE_SCROLL", "Parchemin énorme", RelicTier::TIER_A, "Choisissez 1 Carte Multijoueur parmi 3 à ajouter à votre Deck." },
    { "NEOWS_TORMENT", "Tourment de Neow", RelicTier::TIER_B, "Ajoutez 1 Fureur de Neow à votre Deck." },
    { "NEW_LEAF", "Nouvelle page", RelicTier::TIER_A, "Transformez 1 carte." },
    { "PHIAL_HOLSTER", "Étui à fioles", RelicTier::TIER_S, "Gagnez 1 nouvel emplacement de potion et obtenez 2 Potions aléatoires." },
    { "PRECISE_SCISSORS", "Ciseaux précis", RelicTier::TIER_B, "Retirez 1 carte de votre Deck." },
    { "SCROLL_BOXES", "Caisse de parchemins", RelicTier::TIER_D, "Choisissez 1 pack de cartes parmi 2 à ajouter à votre Deck." },
    { "WINGED_BOOTS", "Bottes ailées", RelicTier::TIER_D, "Vous pouvez ignorer les trajets indiqués sur la carte lorsque vous choisissez la prochaine salle 3 fois." },
    { "LAVA_ROCK", "Pierre volcanique", RelicTier::TIER_C, "Le Boss de l'Acte 1 octroie 2 Reliques lorsqu'il est vaincu." },
    { "SMALL_CAPSULE", "Petite capsule", RelicTier::TIER_A, "Obtenez une Relique aléatoire." },
    { "NUTRITIOUS_OYSTER", "Huître nourrissante", RelicTier::TIER_D, "Augmentez vos PV Max de 11." },
    { "STONE_HUMIDIFIER", "Humidificateur en pierre", RelicTier::TIER_D, "Lorsque vous vous Reposez à un Site de repos, augmentez vos PV Max de 5." },
    { "NEOWS_TALISMAN", "Talisman de Neow", RelicTier::TIER_A, "Améliorez 1 de vos Frappes et 1 de vos Défenses." },
    { "POMANDER", "Pommandre", RelicTier::TIER_B, "Améliorez une carte." },
};

constexpr AncientRelic RELICS_NEOW_1[] = {
    { "CURSED_PEARL", "Perle maudite", RelicTier::TIER_S, "Recevez Cupidité. Obtenez 333 Pièces d'or." },
    { "DOWSING_ROD", "Baguette de rhabdomancie", RelicTier::UNTIERED, "Ajoutez 1 Rhabdomancie à votre Deck." },
    { "HEFTY_TABLET", "Tablette lourde", RelicTier::TIER_B, "Choisissez 1 carte Rare parmi 3 à ajouter à votre Deck. Ajoutez 1 Blessure à votre Deck." },
    { "LARGE_CAPSULE", "Grande capsule", RelicTier::TIER_B, "Obtenez 2 Reliques aléatoires. Ajoutez une Frappe et une Défense à votre Deck." },
    { "LEAFY_POULTICE", "Cataplasme", RelicTier::TIER_S, "Transformez 1 Frappe et 1 Défense et perdez 12 PV Max." },
    { "NEOWS_BONES", "Osselets de Neow", RelicTier::TIER_C, "Gagnez 2 Reliques de Neow aléatoires. Ajoutez 1 Malédiction aléatoire à votre Deck." },
    { "NEOWS_SACRIFICE", "Sacrifice de Neow", RelicTier::UNTIERED, "Obtenez 1 Ambre gris et ajoutez 1 Culpabilité à votre Deck." },
    { "PRECARIOUS_SHEARS", "Forces précaires", RelicTier::TIER_C, "Retirez 2 cartes de votre Deck. Perdez 16 PV." },
    { "SILKEN_TRESS", "Tresse soyeuse", RelicTier::UNTIERED, "Perdez toutes vos Pièces d'or. Envoûtez toutes les cartes de votre première récompense de cartes avec Glamour." },
    { "SILVER_CRUCIBLE", "Creuset en argent", RelicTier::TIER_C, "Vos 3 premières récompenses de carte sont Améliorées. Le premier Coffre trésor que vous ouvrez est vide." },
};

constexpr AncientRelic RELICS_DARV_0[] = {
    { "ASTROLABE", "Astrolabe", RelicTier::TIER_A, "Transformez 3 cartes, puis Améliorez-les." },
    { "BLACK_STAR", "Étoile noire", RelicTier::TIER_B, "Les Élites donnent une Relique supplémentaire lorsqu'ils sont vaincus." },
    { "CALLING_BELL", "Cloche d'appel", RelicTier::TIER_C, "Obtenez une Malédiction spéciale et 3 Reliques." },
    { "EMPTY_CAGE", "Cage vide", RelicTier::TIER_A, "Retirez 2 cartes de votre Deck." },
    { "PANDORAS_BOX", "Boîte de Pandore", RelicTier::TIER_S, "Transformez TOUTES vos Frappes et vos Défenses." },
    { "RUNIC_PYRAMID", "Pyramide runique", RelicTier::TIER_S, "À la fin de votre tour, vous ne défaussez pas votre Main." },
    { "SNECKO_EYE", "Œil de geckobra", RelicTier::TIER_B, "Au début de votre tour, piochez 2 cartes supplémentaires. Démarrez chaque combat Désorienté." },
    { "ECTOPLASM", "Ectoplasme", RelicTier::TIER_D, "Vous ne pouvez plus obtenir de Pièces d'or. Gagnez 1 au début de chaque tour." },
    { "SOZU", "Sozu", RelicTier::TIER_D, "Gagnez 1 au début de chaque tour. Vous ne pouvez plus obtenir de Potions." },
    { "PHILOSOPHERS_STONE", "Pierre philosophale", RelicTier::TIER_D, "Gagnez 1 au début de chaque tour. TOUS les ennemis démarrent le combat avec 1 de Force." },
    { "VELVET_CHOKER", "Ras du cou en velours", RelicTier::TIER_B, "Gagnez 1 au début de chaque tour. Vous ne pouvez pas jouer plus de 6 cartes par tour." },
    { "DUSTY_TOME", "Grimoire poussiéreux", RelicTier::TIER_B, "Obtenez une Carte d'Ancien." },
};

constexpr AncientRelic RELICS_OROBAS_0[] = {
    { "ELECTRIC_SHRYMP", "Crevaite électrique", RelicTier::TIER_B, "Envoûtez une Compétence avec Imprégné." },
    { "GLASS_EYE", "Œil de verre", RelicTier::TIER_C, "Lorsque vous obtenez cette relique, obtenez 2 cartes Communes, 2 cartes Inhabituelles, et 1 carte Rare." },
    { "PRISMATIC_GEM", "Joyau prismatique", RelicTier::TIER_B, "Gagnez 1 au début de chaque tour. Les récompenses de carte contiennent désormais des cartes d'autres couleurs." },
    { "SEA_GLASS", "Verre marin", RelicTier::TIER_D, "Consultez une sélection de 15 cartes d'un autre personnage. Ajoutez-en autant que vous souhaitez à votre Deck." },
};

constexpr AncientRelic RELICS_OROBAS_1[] = {
    { "ALCHEMICAL_COFFER", "Caisse d'alchimie", RelicTier::TIER_S, "Gagnez 4 emplacements de potion avec des Potions aléatoires." },
    { "DRIFTWOOD", "Bois flotté", RelicTier::TIER_A, "Vous disposez d'une relance gratuite pour chaque récompense de carte." },
    { "RADIANT_PEARL", "Perle radieuse", RelicTier::TIER_S, "Au début de chaque combat, ajoutez 1 Luminescence à votre Main." },
    { "SAND_CASTLE", "Château de sable", RelicTier::TIER_D, "Lorsque vous obtenez cette relique, Améliorez 6 cartes aléatoires." },
};

constexpr AncientRelic RELICS_OROBAS_2[] = {
    { "TOUCH_OF_OROBAS", "Toucher d'Orobas", RelicTier::TIER_C, "Remplacez votre Relique de départ par une version Améliorée." },
    { "ARCHAIC_TOOTH", "Dent archaïque", RelicTier::TIER_A, "Transformez une carte de base en sa version d'Ancien." },
};

constexpr AncientRelic RELICS_PAEL_0[] = {
    { "PAELS_FLESH", "Chair de Paël", RelicTier::TIER_A, "Gagnez 1 supplémentaire au début de votre 3ème tour et au début de chaque tour suivant." },
    { "PAELS_HORN", "Corne de Paël", RelicTier::TIER_D, "Ajoutez 2 Relaxations à votre Deck." },
    { "PAELS_TEARS", "Larmes de Paël", RelicTier::TIER_S, "Si vous terminez votre tour sans avoir dépensé toute votre [E], gagnez 2 supplémentaires au prochain tour." },
};

constexpr AncientRelic RELICS_PAEL_1[] = {
    { "PAELS_WING", "Aile de Paël", RelicTier::TIER_B, "Vous pouvez offrir des récompenses de carte en sacrifice à Paël. Tous les 2 sacrifices, vous obtenez une Relique." },
    { "PAELS_CLAW", "Griffe de Paël", RelicTier::TIER_A, "Envoûtez toutes vos Défenses avec Viscosité." },
    { "PAELS_TOOTH", "Dent de Paël", RelicTier::TIER_D, "Retirez 5 cartes de votre Deck. Après chaque combat, récupérez-en 1 aléatoirement ; elle est Améliorée." },
    { "PAELS_GROWTH", "Excroissance de Paël", RelicTier::TIER_B, "Envoûtez une carte avec Clonage." },
};

constexpr AncientRelic RELICS_PAEL_2[] = {
    { "PAELS_EYE", "Œil de Paël", RelicTier::TIER_S, "Lorsque vous terminez votre tour sans avoir joué de cartes pour la première fois dans un combat, Épuisez votre Main, et jouez un tour supplémentaire." },
    { "PAELS_BLOOD", "Sang de Paël", RelicTier::TIER_S, "Au début de votre tour, piochez 1 carte supplémentaire." },
    { "PAELS_LEGION", "Légion de Paël", RelicTier::TIER_C, "Double l'Armure reçue d'une carte, puis s'endort pendant 2 tours." },
};

constexpr AncientRelic RELICS_TEZCATARA_0[] = {
    { "VERY_HOT_COCOA", "Chocolat très chaud", RelicTier::TIER_B, "Démarrez chaque combat avec 4 supplémentaires." },
    { "YUMMY_COOKIE", "Délicieux biscuit", RelicTier::TIER_A, "Améliorez 4 cartes." },
    { "NUTRITIOUS_SOUP", "Soupe nourrissante", RelicTier::TIER_B, "Envoûtez toutes les Frappes dans votre Deck avec Braise de Tezcatara." },
};

constexpr AncientRelic RELICS_TEZCATARA_1[] = {
    { "BIIIG_HUG", "Grooos câlin", RelicTier::TIER_A, "Retirez 4 cartes de votre Deck. Lorsque vous mélangez votre Pioche, ajoutez une Suie à votre Pioche." },
    { "STORYBOOK", "Livre d'histoires", RelicTier::TIER_S, "Ajoutez 1 Flamme éclatante à votre Deck." },
    { "TOASTY_MITTENS", "Mitaines réchauffantes", RelicTier::TIER_D, "Au début de votre tour, Épuisez 1 carte de votre Main et gagnez 1 de Force." },
};

constexpr AncientRelic RELICS_TEZCATARA_2[] = {
    { "GOLDEN_COMPASS", "Boussole dorée", RelicTier::TIER_A, "Remplacez la carte de l'Acte 2 par un chemin unique." },
    { "PUMPKIN_CANDLE", "Lanterne-citrouille", RelicTier::TIER_C, "Gagnez 1 au début de chaque tour. S'éteint après 5 combats. Peut être Ravivée aux Sites de repos." },
    { "TOY_BOX", "Caisse à jouets", RelicTier::TIER_D, "Obtenez 5 Reliques de cire. Tous les 3 combats, la Relique de cire le plus à gauche fondra." },
    { "SEAL_OF_GOLD", "Sceau doré", RelicTier::TIER_C, "Au début de votre tour, dépensez 3 Pièces d'or pour gagner 1." },
};

constexpr AncientRelic RELICS_NONUPEIPE_0[] = {
    { "BLESSED_ANTLER", "Bois bénis", RelicTier::TIER_B, "Gagnez 1 au début de chaque tour. Au début de chaque combat, ajoutez 3 Hébété à votre Pioche." },
    { "BRILLIANT_SCARF", "Écharpe étincelante", RelicTier::TIER_B, "La 5ème carte jouée depuis votre main chaque tour peut être jouée sans coût." },
    { "DELICATE_FROND", "Fougère délicate", RelicTier::TIER_A, "Au début de chaque combat, remplissez vos emplacements de potion vides avec des Potions aléatoires." },
    { "DIAMOND_DIADEM", "Diadème de diamant", RelicTier::TIER_S, "Démarrez chaque combat avec 20 d'Armure. Vous ne perdez pas votre Armure au début de votre 2ème tour." },
    { "FUR_COAT", "Manteau de fourrure", RelicTier::TIER_D, "Marquez 8 combats aléatoires sur la Carte. Les ennemis dans ces salles ont 1 PV." },
    { "GLITTER", "Paillettes", RelicTier::TIER_C, "Envoûtez toutes les récompenses de carte avec Glamour." },
    { "JEWELRY_BOX", "Coffre à bijoux", RelicTier::TIER_A, "Ajoutez 1 Apothéose à votre Deck." },
    { "LOOMING_FRUIT", "Fruits exotiques", RelicTier::TIER_C, "Augmentez vos PV Max de 31." },
    { "SIGNET_RING", "Anneau à signet", RelicTier::TIER_A, "Lorsque vous obtenez cette relique, obtenez 888 Pièces d'or." },
    { "BEAUTIFUL_BRACELET", "Bracelet brillant", RelicTier::TIER_B, "Choisissez 4 cartes dans votre Deck. Envoûtez-les avec Célérité 2." },
};

constexpr AncientRelic RELICS_TANX_0[] = {
    { "CLAWS", "Griffes", RelicTier::TIER_B, "Transformez jusqu'à 6 cartes en Mutilation." },
    { "CROSSBOW", "Arbalète", RelicTier::TIER_C, "Au début de votre tour, ajoutez une Attaque aléatoire à votre Main. Elle coûte 0 ce tour." },
    { "IRON_CLUB", "Gourdin de fer", RelicTier::TIER_A, "Toutes les 4 cartes jouées, vous piochez 1 carte." },
    { "MEAT_CLEAVER", "Couperet à viande", RelicTier::TIER_S, "Vous pouvez Cuisiner aux Sites de Repos." },
    { "SAI", "Saï", RelicTier::TIER_S, "Au début de votre tour, gagnez 7 d'Armure." },
    { "SPIKED_GAUNTLETS", "Gantelets à pointes", RelicTier::TIER_D, "Gagnez 1 au début de chaque tour. Les Pouvoirs coûtent 1 [E] supplémentaire." },
    { "TANXS_WHISTLE", "Sifflet de Tanx", RelicTier::TIER_A, "Ajoutez 1 Sifflet à votre Deck." },
    { "THROWING_AXE", "Hache de lancer", RelicTier::TIER_A, "La première carte que vous jouez chaque combat est jouée une fois de plus." },
    { "WAR_HAMMER", "Marteau de guerre", RelicTier::TIER_D, "Lorsque vous tuez un Élite, Améliorez 4 cartes aléatoires." },
    { "TRI_BOOMERANG", "Tri-boomerang", RelicTier::TIER_A, "Choisissez 3 Attaques de votre Deck. Envoûtez-les avec Instinct." },
};

constexpr AncientRelic RELICS_VAKUU_0[] = {
    { "BLOOD_SOAKED_ROSE", "Rose sanguinolente", RelicTier::TIER_C, "Ajoutez 1 Assuétude à votre Deck. Gagnez 1 au début de chaque tour." },
    { "WHISPERING_EARRING", "Boucle d'oreille susurrante", RelicTier::TIER_A, "Gagnez 1 au début de chaque tour. Vakuu joue votre premier tour à votre place." },
    { "FIDDLE", "Violon", RelicTier::TIER_D, "Au début de chaque tour, piochez 2 cartes supplémentaires. Vous ne pouvez plus piocher de cartes pendant votre tour." },
};

constexpr AncientRelic RELICS_VAKUU_1[] = {
    { "PRESERVED_FOG", "Brouillard en conserve", RelicTier::TIER_A, "Retirez 3 cartes de votre Deck. Ajoutez Hubris à votre Deck." },
    { "SERE_TALON", "Serre desséchée", RelicTier::TIER_B, "Perdez 9 PV Max. Ajoutez 3 Vœux à votre Deck." },
    { "DISTINGUISHED_CAPE", "Cape distinguée", RelicTier::TIER_S, "Ajoutez 2 Malédictions aléatoires et 3 Apparitions à votre Deck." },
};

constexpr AncientRelic RELICS_VAKUU_2[] = {
    { "CHOICES_PARADOX", "Embarras du choix", RelicTier::TIER_A, "Au début de chaque combat, ajoutez 1 carte parmi 5 cartes aléatoires à votre Main. Ajoutez Retenue à la carte choisie." },
    { "MUSIC_BOX", "Boîte à musique", RelicTier::TIER_B, "Vous créez une copie Éthérée de la première Attaque que vous jouez chaque tour." },
    { "LORDS_PARASOL", "Parasol seigneurial", RelicTier::TIER_D, "Lorsque vous passez chez le Marchand, vous obtenez immédiatement TOUT ce qu'il propose." },
    { "JEWELED_MASK", "Masque serti", RelicTier::TIER_S, "Au début de chaque combat, placez un Pouvoir aléatoire de votre Pioche dans votre Main. Il coûte 0 ce combat." },
};

constexpr AncientSlot SLOTS_NEOW[] = {
    { "blessing", 2, std::span<const AncientRelic>(RELICS_NEOW_0) },
    { "curse", 1, std::span<const AncientRelic>(RELICS_NEOW_1) },
};

constexpr AncientSlot SLOTS_DARV[] = {
    { "", 3, std::span<const AncientRelic>(RELICS_DARV_0) },
};

constexpr AncientSlot SLOTS_OROBAS[] = {
    { "", 1, std::span<const AncientRelic>(RELICS_OROBAS_0) },
    { "", 1, std::span<const AncientRelic>(RELICS_OROBAS_1) },
    { "", 1, std::span<const AncientRelic>(RELICS_OROBAS_2) },
};

constexpr AncientSlot SLOTS_PAEL[] = {
    { "", 1, std::span<const AncientRelic>(RELICS_PAEL_0) },
    { "", 1, std::span<const AncientRelic>(RELICS_PAEL_1) },
    { "", 1, std::span<const AncientRelic>(RELICS_PAEL_2) },
};

constexpr AncientSlot SLOTS_TEZCATARA[] = {
    { "", 1, std::span<const AncientRelic>(RELICS_TEZCATARA_0) },
    { "", 1, std::span<const AncientRelic>(RELICS_TEZCATARA_1) },
    { "", 1, std::span<const AncientRelic>(RELICS_TEZCATARA_2) },
};

constexpr AncientSlot SLOTS_NONUPEIPE[] = {
    { "", 3, std::span<const AncientRelic>(RELICS_NONUPEIPE_0) },
};

constexpr AncientSlot SLOTS_TANX[] = {
    { "", 3, std::span<const AncientRelic>(RELICS_TANX_0) },
};

constexpr AncientSlot SLOTS_VAKUU[] = {
    { "", 1, std::span<const AncientRelic>(RELICS_VAKUU_0) },
    { "", 1, std::span<const AncientRelic>(RELICS_VAKUU_1) },
    { "", 1, std::span<const AncientRelic>(RELICS_VAKUU_2) },
};

constexpr Ancient ALL_ANCIENTS[] = {
    { AncientId::NEOW, "neow", "Neow", "Mère de la Résurrection (exilée)", 1, std::span<const AncientSlot>(SLOTS_NEOW) },
    { AncientId::DARV, "darv", "Darv", "L'Amasseur", 7, std::span<const AncientSlot>(SLOTS_DARV) },
    { AncientId::OROBAS, "orobas", "Orobas", "Arc-en-ciel Vivant", 2, std::span<const AncientSlot>(SLOTS_OROBAS) },
    { AncientId::PAEL, "pael", "Paël", "Dragon de Cire", 2, std::span<const AncientSlot>(SLOTS_PAEL) },
    { AncientId::TEZCATARA, "tezcatara", "Tezcatara", "Nourricière de la Flamme", 2, std::span<const AncientSlot>(SLOTS_TEZCATARA) },
    { AncientId::NONUPEIPE, "nonupeipe", "Nonupeipée", "Incarnation de la Fortune", 4, std::span<const AncientSlot>(SLOTS_NONUPEIPE) },
    { AncientId::TANX, "tanx", "Tanx", "Roi Chimère", 4, std::span<const AncientSlot>(SLOTS_TANX) },
    { AncientId::VAKUU, "vakuu", "Vakuu", "Le Premier Démon", 4, std::span<const AncientSlot>(SLOTS_VAKUU) },
};

} // namespace

const Ancient& get_ancient(AncientId id) {
    return ALL_ANCIENTS[static_cast<size_t>(id)];
}

std::span<const Ancient> get_all_ancients() {
    return std::span<const Ancient>(ALL_ANCIENTS);
}

} // namespace engine