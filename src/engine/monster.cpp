#include "engine/monster.h"

namespace engine {
namespace {

constexpr MonsterMoveData MOVES_AEONGLASS[] = {
    { "EBB", "Régression", 22, 26, 33, 1 },
    { "EYE_LASERS", "Rayons oculaires", 11, 12, 0, 2 },
    { "INCREASING_INTENSITY", "Intensité croissante", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_ASSASSIN_RUBY_RAIDER[] = {
    { "KILLSHOT", "Tir meurtrier", 10, 11, 0, 1 },
};

constexpr MonsterMoveData MOVES_AXEBOT[] = {
    { "BOOT_UP", "Démarrage", 0, 0, 10, 1 },
    { "ONE_TWO", "Droite-gauche", 10, 11, 0, 2 },
    { "HAMMER_UPPERCUT", "Marteau Uppercut", 14, 18, 0, 1 },
};

constexpr MonsterMoveData MOVES_AXE_RUBY_RAIDER[] = {
    { "SWING_1", "Swing", 5, 6, 5, 1 },
    { "SWING_2", "Swing", 5, 6, 5, 1 },
    { "BIG_SWING", "Balancier", 12, 13, 0, 1 },
};

constexpr MonsterMoveData MOVES_BOWLBUG_EGG[] = {
    { "BITE", "Morsure", 7, 8, 7, 1 },
};

constexpr MonsterMoveData MOVES_BOWLBUG_NECTAR[] = {
    { "THRASH", "Furie", 3, 3, 0, 1 },
    { "BUFF", "Buff", 0, 0, 0, 1 },
    { "THRASH2", "Furie", 3, 3, 0, 1 },
};

constexpr MonsterMoveData MOVES_BOWLBUG_ROCK[] = {
    { "HEADBUTT", "Coup de boule", 15, 16, 0, 1 },
    { "DIZZY", "Étourdi", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_BOWLBUG_SILK[] = {
    { "THRASH", "Furie", 4, 5, 0, 2 },
    { "TOXIC_SPIT", "Crachat Toxique", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_BRUTE_RUBY_RAIDER[] = {
    { "BEAT", "Rouer", 7, 8, 0, 1 },
    { "ROAR", "Rugissement", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_BYGONE_EFFIGY[] = {
    { "SLEEP", "Sommeil", 0, 0, 0, 1 },
    { "WAKE", "Éveil", 0, 0, 0, 1 },
    { "SLEEP_MOVE_2", "Sommeil", 0, 0, 0, 1 },
    { "SLASHES", "Bourrasque", 13, 15, 0, 1 },
};

constexpr MonsterMoveData MOVES_BYRDONIS[] = {
    { "PECK", "Coup de bec", 3, 4, 0, 3 },
    { "SWOOP", "Piqué", 17, 19, 0, 1 },
};

constexpr MonsterMoveData MOVES_CALCIFIED_CULTIST[] = {
    { "INCANTATION", "Incantation", 0, 0, 0, 1 },
    { "DARK_STRIKE", "Frappe ténébreuse", 9, 11, 0, 1 },
};

constexpr MonsterMoveData MOVES_CEREMONIAL_BEAST[] = {
    { "STAMP", "Trépignation", 0, 0, 0, 1 },
    { "PLOW", "Cavalcade", 18, 20, 0, 1 },
    { "STUN", "Étourdi", 0, 0, 0, 1 },
    { "BEAST_CRY", "Cri bestial", 0, 0, 0, 1 },
    { "STOMP", "Piétinement", 15, 17, 0, 1 },
    { "CRUSH", "Écrasement", 17, 19, 0, 1 },
};

constexpr MonsterMoveData MOVES_CHOMPER[] = {
    { "CLAMP", "Pince", 8, 9, 0, 2 },
    { "SCREECH", "Crissement", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_CORPSE_SLUG[] = {
    { "WHIP_SLAP", "Fouet", 3, 3, 0, 2 },
    { "GLOMP", "Câlin brutal", 8, 9, 0, 1 },
    { "GOOP", "Mucus", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_CROSSBOW_RUBY_RAIDER[] = {
    { "FIRE", "Feu !", 14, 16, 0, 1 },
    { "RELOAD", "Rechargement", 0, 0, 3, 1 },
};

constexpr MonsterMoveData MOVES_CRUSHER[] = {
    { "THRASH", "Furie", 12, 14, 0, 1 },
    { "ENLARGING_STRIKE", "Frappe embrassante", 4, 4, 0, 1 },
    { "BUG_STING", "Piqûre d'insecte", 6, 7, 0, 2 },
    { "ADAPT", "Adaptation", 0, 0, 0, 1 },
    { "GUARDED_STRIKE", "Frappe prudente", 12, 14, 18, 1 },
};

constexpr MonsterMoveData MOVES_CUBEX_CONSTRUCT[] = {
    { "CHARGE_UP", "Chargement", 0, 0, 0, 1 },
    { "REPEATER_BLAST", "Tir répéteur", 7, 8, 0, 1 },
    { "REPEATER_BLAST_MOVE_2", "Tir répéteur", 7, 8, 0, 1 },
    { "EXPEL", "Éjection", 5, 6, 0, 2 },
};

constexpr MonsterMoveData MOVES_DAMP_CULTIST[] = {
    { "INCANTATION", "Incantation", 0, 0, 0, 1 },
    { "DARK_STRIKE", "Frappe ténébreuse", 1, 3, 0, 1 },
};

constexpr MonsterMoveData MOVES_DECIMILLIPEDE_SEGMENT_BACK[] = {
    { "WRITHE", "Torsion", 5, 6, 0, 2 },
    { "BULK", "Croissance", 6, 7, 0, 1 },
    { "CONSTRICT", "Constriction", 8, 9, 0, 1 },
    { "DEAD", "Mort", 0, 0, 0, 1 },
    { "REATTACH", "Reconnexion", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_DECIMILLIPEDE_SEGMENT_FRONT[] = {
    { "WRITHE", "Torsion", 5, 6, 0, 2 },
    { "BULK", "Croissance", 6, 7, 0, 1 },
    { "CONSTRICT", "Constriction", 8, 9, 0, 1 },
    { "DEAD", "Mort", 0, 0, 0, 1 },
    { "REATTACH", "Reconnexion", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_DECIMILLIPEDE_SEGMENT_MIDDLE[] = {
    { "WRITHE", "Torsion", 5, 6, 0, 2 },
    { "BULK", "Croissance", 6, 7, 0, 1 },
    { "CONSTRICT", "Constriction", 8, 9, 0, 1 },
    { "DEAD", "Mort", 0, 0, 0, 1 },
    { "REATTACH", "Reconnexion", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_DEVOTED_SCULPTOR[] = {
    { "FORBIDDEN_INCANTATION", "Incantation interdite", 0, 0, 0, 1 },
    { "SAVAGE", "Sauvagerie", 12, 15, 0, 1 },
};

constexpr MonsterMoveData MOVES_ENTOMANCER[] = {
    { "PHEROMONE_SPIT", "Crachat de phéromones", 0, 0, 0, 1 },
    { "BEES", "Bzzzzzz !!", 3, 3, 0, 7 },
    { "SPEAR", "Javelot !", 18, 20, 0, 1 },
};

constexpr MonsterMoveData MOVES_EXOSKELETON[] = {
    { "SKITTER", "Détalage", 1, 1, 0, 3 },
    { "MANDIBLES", "Mandibules", 8, 9, 0, 1 },
    { "ENRAGE", "Enrager", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_EYE_WITH_TEETH[] = {
    { "DISTRACT", "Distraction", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_FABRICATOR[] = {
    { "FABRICATE", "Fabrication", 0, 0, 0, 1 },
    { "FABRICATING_STRIKE", "Frappe du Machiniste", 18, 21, 0, 1 },
    { "DISINTEGRATE", "Désintégration", 11, 13, 0, 1 },
};

constexpr MonsterMoveData MOVES_FAKE_MERCHANT_MONSTER[] = {
    { "SWIPE", "Vol", 13, 15, 0, 1 },
    { "SPEW_COINS", "Projection de pièces", 2, 2, 0, 8 },
    { "THROW_RELIC", "Lancer de relique", 9, 10, 0, 1 },
    { "ENRAGE", "Enrager", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_FAT_GREMLIN[] = {
    { "SPAWNED", "Éveil", 0, 0, 0, 1 },
    { "FLEE", "Fuite", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_FLAIL_KNIGHT[] = {
    { "WAR_CHANT", "Chant de Guerre", 0, 0, 0, 1 },
    { "FLAIL", "Fléau", 9, 10, 0, 2 },
    { "RAM", "Bélier", 15, 17, 0, 1 },
};

constexpr MonsterMoveData MOVES_FLYCONID[] = {
    { "VULNERABLE_SPORES", "Spores fragilisantes", 0, 0, 0, 1 },
    { "FRAIL_SPORES", "Spores fragilisantes", 8, 9, 0, 1 },
    { "SMASH", "Fracas", 11, 12, 0, 1 },
};

constexpr MonsterMoveData MOVES_FOGMOG[] = {
    { "ILLUSION", "Spores illusoires", 0, 0, 0, 1 },
    { "SWIPE", "Mandale", 8, 9, 0, 1 },
    { "SWIPE_RANDOM", "Mandale", 8, 9, 0, 1 },
    { "HEADBUTT", "Headbutt", 14, 16, 0, 1 },
};

constexpr MonsterMoveData MOVES_FOSSIL_STALKER[] = {
    { "TACKLE", "Charge", 9, 11, 0, 1 },
    { "LATCH", "Accrochage", 12, 14, 0, 1 },
    { "LASH", "Fouet", 3, 4, 0, 2 },
};

constexpr MonsterMoveData MOVES_FROG_KNIGHT[] = {
    { "FOR_THE_QUEEN", "Pour la Reine", 0, 0, 0, 1 },
    { "STRIKE_DOWN_EVIL", "Expier le mal", 21, 23, 0, 1 },
    { "TONGUE_LASH", "Fouet de langue", 13, 14, 0, 1 },
    { "BEETLE_CHARGE", "Charge scarabée", 35, 40, 0, 1 },
};

constexpr MonsterMoveData MOVES_FUZZY_WURM_CRAWLER[] = {
    { "FIRST_ACID_GOOP", "Acide visqueux", 4, 6, 0, 1 },
    { "ACID_GOOP", "Acide visqueux", 4, 6, 0, 1 },
    { "INHALE", "Inspiration", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_GAS_BOMB[] = {
    { "EXPLODE", "Explosion", 8, 9, 0, 1 },
};

constexpr MonsterMoveData MOVES_GLOBE_HEAD[] = {
    { "THUNDER_STRIKE", "Canalisation kéraunique", 6, 7, 0, 3 },
    { "SHOCKING_SLAP", "Claque de choc", 13, 14, 0, 1 },
    { "GALVANIC_BURST", "Galvanic Burst", 16, 17, 0, 1 },
};

constexpr MonsterMoveData MOVES_GREMLIN_MERC[] = {
    { "GIMME", "Donne-moi ça !", 7, 8, 0, 2 },
    { "DOUBLE_SMASH", "Fracas double", 6, 7, 0, 2 },
    { "HEHE", "Héhé", 8, 9, 0, 1 },
};

constexpr MonsterMoveData MOVES_GUARDBOT[] = {
    { "GUARD", "Garde", 0, 0, 15, 1 },
};

constexpr MonsterMoveData MOVES_HAUNTED_SHIP[] = {
    { "SWIPE", "Griffe", 13, 14, 0, 1 },
    { "STOMP", "Piétinement", 4, 5, 0, 3 },
    { "HAUNT", "Hanter", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_HUNTER_KILLER[] = {
    { "TENDERIZING_GOOP", "Suc de décomposition", 0, 0, 0, 1 },
    { "BITE", "Morsure", 17, 19, 0, 1 },
    { "PUNCTURE", "Perforation", 7, 8, 0, 3 },
};

constexpr MonsterMoveData MOVES_INFESTED_PRISM[] = {
    { "JAB", "Coup sec", 15, 17, 0, 1 },
    { "RADIATE", "Irradiation", 11, 13, 11, 1 },
    { "WHIRLWIND", "Tourbillon", 5, 6, 0, 3 },
    { "PULSATE", "Pulsation", 8, 10, 20, 1 },
};

constexpr MonsterMoveData MOVES_INKLET[] = {
    { "JAB", "Coup sec", 3, 4, 0, 1 },
    { "WHIRLWIND", "Tourbillon", 2, 3, 0, 3 },
    { "PIERCING_GAZE", "Regard perçant", 10, 11, 0, 1 },
};

constexpr MonsterMoveData MOVES_KIN_FOLLOWER[] = {
    { "QUICK_SLASH", "Coup vif", 5, 5, 0, 1 },
    { "BOOMERANG", "Boomerang", 2, 2, 0, 2 },
    { "POWER_DANCE", "Transe Martiale", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_KIN_PRIEST[] = {
    { "ORB_OF_FRAILTY", "Orbe de fragilité", 8, 9, 0, 1 },
    { "ORB_OF_WEAKNESS", "Orbe de faiblesse", 8, 9, 0, 1 },
    { "BEAM", "Rayon animique", 3, 3, 0, 3 },
    { "RITUAL", "Sombre rituel", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_KNOWLEDGE_DEMON[] = {
    { "CURSE_OF_KNOWLEDGE", "Malédiction du savoir", 0, 0, 0, 1 },
    { "SLAP", "Claque", 17, 18, 0, 1 },
    { "KNOWLEDGE_OVERWHELMING", "Savoir accablant", 8, 9, 0, 3 },
    { "PONDER", "Contemplation", 11, 13, 0, 1 },
};

constexpr MonsterMoveData MOVES_LAGAVULIN_MATRIARCH[] = {
    { "SLEEP", "Sommeil", 0, 0, 0, 1 },
    { "SLASH", "Trancher", 19, 21, 0, 1 },
    { "SLASH2", "Trancher", 12, 14, 12, 1 },
    { "DISEMBOWEL", "Éventration", 9, 10, 0, 2 },
    { "SOUL_SIPHON", "Siphon d'âme", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_LEAF_SLIME_M[] = {
    { "CLUMP_SHOT", "Tir groupé", 8, 9, 0, 1 },
    { "STICKY_SHOT", "Tir adhésif", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_LEAF_SLIME_S[] = {
    { "TACKLE", "Charge", 3, 4, 0, 1 },
    { "GOOP", "Gelée", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_LIVING_FOG[] = {
    { "ADVANCED_GAS", "Gaz nocif", 8, 9, 0, 1 },
    { "BLOAT", "Boursouflure", 5, 6, 0, 1 },
    { "SUPER_GAS_BLAST", "Super rafale gazeuse", 8, 9, 0, 1 },
};

constexpr MonsterMoveData MOVES_LIVING_SHIELD[] = {
    { "SHIELD_SLAM", "Heurt de bouclier", 6, 6, 0, 1 },
    { "SMASH", "Heurt de bouclier", 16, 18, 0, 1 },
};

constexpr MonsterMoveData MOVES_LOUSE_PROGENITOR[] = {
    { "WEB_CANNON", "Canon de toile", 9, 10, 0, 1 },
    { "POUNCE", "Bond", 14, 16, 0, 1 },
    { "CURL_AND_GROW", "Repli fortifiant", 0, 0, 14, 1 },
};

constexpr MonsterMoveData MOVES_MAGI_KNIGHT[] = {
    { "POWER_SHIELD", "Bouclier de puissance", 6, 7, 5, 1 },
    { "DAMPEN", "Atténuation", 0, 0, 0, 1 },
    { "PREP", "Prep", 0, 0, 5, 1 },
    { "MAGIC_BOMB", "Bombe magique", 35, 40, 0, 1 },
    { "RAM", "Bélier", 10, 11, 0, 1 },
};

constexpr MonsterMoveData MOVES_MAWLER[] = {
    { "RIP_AND_TEAR", "Déchirure", 14, 16, 0, 1 },
    { "ROAR", "Rugissement", 0, 0, 0, 1 },
    { "CLAW", "Griffe", 4, 5, 0, 2 },
};

constexpr MonsterMoveData MOVES_MECHA_KNIGHT[] = {
    { "CHARGE", "Charge", 25, 30, 0, 1 },
    { "FLAMETHROWER", "Lance-flammes", 8, 12, 0, 1 },
    { "WINDUP", "Chargement", 0, 0, 15, 1 },
    { "HEAVY_CLEAVE", "Balayage lourd", 35, 40, 0, 1 },
};

constexpr MonsterMoveData MOVES_MYSTERIOUS_KNIGHT[] = {
    { "WAR_CHANT", "Chant de guerre", 0, 0, 0, 1 },
    { "FLAIL", "Fléau", 9, 10, 0, 2 },
    { "RAM", "Bélier", 15, 17, 0, 1 },
};

constexpr MonsterMoveData MOVES_MYTE[] = {
    { "TOXIC", "Abondance toxique", 0, 0, 0, 1 },
    { "BITE", "Morsure", 13, 15, 0, 1 },
    { "SUCK", "Succion", 4, 6, 0, 1 },
};

constexpr MonsterMoveData MOVES_NIBBIT[] = {
    { "BUTT", "Derrière", 12, 13, 0, 1 },
    { "SLICE", "Slice", 6, 7, 5, 1 },
    { "HISS", "Chuintement", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_NOISEBOT[] = {
    { "NOISE", "Ébruitement", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_OVICOPTER[] = {
    { "LAY_EGGS", "Ponte", 0, 0, 0, 1 },
    { "SMASH", "Fracas", 16, 17, 0, 1 },
    { "TENDERIZER", "Adoucir", 7, 8, 0, 1 },
    { "NUTRITIONAL_PASTE", "Pâte nutritive", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_OWL_MAGISTRATE[] = {
    { "MAGISTRATE_SCRUTINY", "Regard du magistrat", 16, 17, 0, 1 },
    { "PECK_ASSAULT", "Peck Assault", 4, 4, 0, 6 },
    { "JUDICIAL_FLIGHT", "Envol judiciaire", 0, 0, 0, 1 },
    { "VERDICT", "Verdict", 33, 36, 0, 1 },
};

constexpr MonsterMoveData MOVES_PARAFRIGHT[] = {
    { "SLAM", "Heurt", 16, 17, 0, 1 },
};

constexpr MonsterMoveData MOVES_PHANTASMAL_GARDENER[] = {
    { "BITE", "Morsure", 5, 5, 0, 1 },
    { "LASH", "Fouet", 7, 7, 0, 1 },
    { "FLAIL", "Furie", 1, 1, 0, 3 },
    { "ENLARGE", "Croissance", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_PHROG_PARASITE[] = {
    { "INFECT", "Infection", 0, 0, 0, 1 },
    { "LASH", "Fouet", 4, 5, 0, 4 },
};

constexpr MonsterMoveData MOVES_PUNCH_CONSTRUCT[] = {
    { "READY", "PRET", 0, 0, 10, 1 },
    { "STRONG_PUNCH", "Poing puissant", 14, 16, 0, 1 },
    { "FAST_PUNCH", "Poing rapide", 5, 6, 0, 2 },
};

constexpr MonsterMoveData MOVES_QUEEN[] = {
    { "PUPPET_STRINGS", "Puppet Strings", 0, 0, 0, 1 },
    { "YOU_ARE_MINE", "Tu es à moi !", 0, 0, 0, 1 },
    { "BURN_BRIGHT_FOR_ME", "De mille feux", 0, 0, 20, 1 },
    { "OFF_WITH_YOUR_HEAD", "À la guillotine", 3, 4, 0, 5 },
    { "EXECUTION", "Exécution", 15, 18, 0, 1 },
    { "ENRAGE", "Enrage", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_ROCKET[] = {
    { "TARGETING_RETICLE", "Réticule de ciblage", 3, 4, 0, 1 },
    { "PRECISION_BEAM", "Rayon précis", 18, 20, 0, 1 },
    { "CHARGE_UP", "Chargement", 0, 0, 0, 1 },
    { "LASER", "Laser", 31, 35, 0, 1 },
    { "RECHARGE", "Recharge", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_SCROLL_OF_BITING[] = {
    { "CHOMP", "Croc", 14, 16, 0, 1 },
    { "CHEW", "Mâchouille", 5, 6, 0, 2 },
    { "MORE_TEETH", "Plus de dents", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_SEAPUNK[] = {
    { "SEA_KICK", "Coup de pied marin", 11, 13, 0, 1 },
    { "SPINNING_KICK", "Coup de pied tournoyant", 2, 2, 0, 4 },
    { "BUBBLE_BURP", "Jet de bulles", 0, 0, 7, 1 },
};

constexpr MonsterMoveData MOVES_SEWER_CLAM[] = {
    { "PRESSURIZE", "Pressurisation", 0, 0, 0, 1 },
    { "JET", "Jet", 10, 11, 0, 1 },
};

constexpr MonsterMoveData MOVES_SHRINKER_BEETLE[] = {
    { "SHRINKER", "Rétrécissement", 0, 0, 0, 1 },
    { "CHOMP", "Croc", 7, 8, 0, 1 },
    { "STOMP", "Piétinement", 13, 14, 0, 1 },
};

constexpr MonsterMoveData MOVES_SKULKING_COLONY[] = {
    { "ZOOM", "Bourdonnement", 14, 16, 0, 1 },
    { "ZOOM_MOVE_2", "Bourdonnement", 14, 16, 0, 1 },
    { "INERTIA", "Inertie", 9, 11, 0, 1 },
    { "PIERCING_STABS", "Coups perforants", 7, 8, 0, 2 },
};

constexpr MonsterMoveData MOVES_SLIMED_BERSERKER[] = {
    { "VOMIT_ICHOR", "Régurgitation d'ichor", 0, 0, 0, 1 },
    { "LEECHING_HUG", "Câlin sangsue", 0, 0, 0, 1 },
    { "SMOTHER", "Smother", 30, 33, 0, 1 },
    { "FURIOUS_PUMMELING", "Ruée de coups", 4, 5, 0, 4 },
};

constexpr MonsterMoveData MOVES_SLITHERING_STRANGLER[] = {
    { "CONSTRICT", "Étranglement", 0, 0, 0, 1 },
    { "THWACK", "Frappe", 7, 8, 5, 1 },
    { "LASH", "Fouet", 12, 13, 0, 1 },
};

constexpr MonsterMoveData MOVES_SLUDGE_SPINNER[] = {
    { "OIL_SPRAY", "Spray huileux", 8, 9, 0, 1 },
    { "SLAM", "Heurt", 11, 12, 0, 1 },
    { "RAGE", "Rage", 6, 7, 0, 1 },
};

constexpr MonsterMoveData MOVES_SLUMBERING_BEETLE[] = {
    { "SNORE", "Ronflement", 0, 0, 0, 1 },
    { "ROLL_OUT", "Roulade", 16, 18, 0, 1 },
};

constexpr MonsterMoveData MOVES_SNAPPING_JAXFRUIT[] = {
    { "ENERGY_ORB", "Orbe d'énergie", 3, 4, 0, 1 },
};

constexpr MonsterMoveData MOVES_SNEAKY_GREMLIN[] = {
    { "SPAWNED", "Éveil", 0, 0, 0, 1 },
    { "TACKLE", "Charge", 9, 10, 0, 1 },
};

constexpr MonsterMoveData MOVES_SOUL_FYSH[] = {
    { "BECKON", "Appel", 0, 0, 0, 1 },
    { "DE_GAS", "Dégazage", 16, 18, 0, 1 },
    { "GAZE", "Fixation", 7, 8, 0, 1 },
    { "FADE", "Camouflage", 0, 0, 0, 1 },
    { "SCREAM", "Hurlement", 13, 15, 0, 1 },
};

constexpr MonsterMoveData MOVES_SOUL_NEXUS[] = {
    { "SOUL_BURN", "Brûlure d'âme", 29, 31, 0, 1 },
    { "MAELSTROM", "Maelstrom", 6, 7, 0, 4 },
    { "DRAIN_LIFE", "Drain de vie", 18, 19, 0, 1 },
};

constexpr MonsterMoveData MOVES_SPECTRAL_KNIGHT[] = {
    { "HEX", "Sortilège", 0, 0, 0, 1 },
    { "SOUL_SLASH", "Frappe spirituelle", 15, 17, 0, 1 },
    { "SOUL_FLAME", "Flamme spirituelle", 3, 4, 0, 3 },
};

constexpr MonsterMoveData MOVES_SPINY_TOAD[] = {
    { "PROTRUDING_SPIKES", "Épines saillantes", 0, 0, 0, 1 },
    { "SPIKE_EXPLOSION", "Explosion épineuse", 23, 25, 0, 1 },
    { "TONGUE_LASH", "Fouet de langue", 17, 19, 0, 1 },
};

constexpr MonsterMoveData MOVES_STABBOT[] = {
    { "STAB", "Coup de poignard", 11, 12, 0, 1 },
};

constexpr MonsterMoveData MOVES_TERROR_EEL[] = {
    { "CRASH", "Fracas", 16, 18, 0, 1 },
    { "THRASH", "Furie", 3, 4, 0, 3 },
    { "STUN", "Étourdi", 0, 0, 0, 1 },
    { "TERROR", "Aura de terreur", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TEST_SUBJECT[] = {
    { "RESPAWN", "Réanimation", 0, 0, 0, 1 },
    { "BITE", "Morsure", 20, 22, 0, 1 },
    { "SKULL_BASH", "Coup de boule", 14, 16, 0, 1 },
    { "MULTI_CLAW", "Multi-Griffes", 10, 11, 0, 1 },
    { "PHASE3_LACERATE", "Lacération", 10, 11, 0, 3 },
    { "BIG_POUNCE", "Ruée bestiale", 45, 45, 0, 1 },
    { "BURNING_GROWL", "Grognement ardent", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_THE_ADVERSARY_MK_ONE[] = {
    { "SMASH", "Fracas", 12, 12, 0, 1 },
    { "BEAM", "Rayon", 15, 15, 0, 1 },
    { "BARRAGE", "Barrage", 8, 8, 0, 2 },
};

constexpr MonsterMoveData MOVES_THE_ADVERSARY_MK_THREE[] = {
    { "CRASH", "Collision", 15, 15, 0, 1 },
    { "FLAME_BEAM", "Rayon de flammes", 18, 18, 0, 1 },
    { "BARRAGE", "Barrage", 10, 10, 0, 2 },
};

constexpr MonsterMoveData MOVES_THE_ADVERSARY_MK_TWO[] = {
    { "BASH", "Heurt", 13, 13, 0, 1 },
    { "FLAME_BEAM", "Rayon de flammes", 16, 16, 0, 1 },
    { "BARRAGE", "Barrage", 9, 9, 0, 2 },
};

constexpr MonsterMoveData MOVES_THE_FORGOTTEN[] = {
    { "MIASMA", "Miasme", 0, 0, 8, 1 },
    { "DREAD", "Angoisse", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_THE_INSATIABLE[] = {
    { "LIQUIFY_GROUND", "Liquéfaction du sol", 0, 0, 0, 1 },
    { "THRASH", "Furie", 8, 9, 0, 2 },
    { "THRASH_MOVE_2", "Furie", 8, 9, 0, 2 },
    { "LUNGING_BITE", "Morsure brusque", 28, 31, 0, 1 },
    { "SALIVATE", "Salivation", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_THE_LOST[] = {
    { "DEBILITATING_SMOG", "Brouillard paralysant", 0, 0, 0, 1 },
    { "EYE_LASERS", "Lasers oculaires", 4, 5, 0, 2 },
};

constexpr MonsterMoveData MOVES_THE_OBSCURA[] = {
    { "ILLUSION", "Illusion", 0, 0, 0, 1 },
    { "PIERCING_GAZE", "Regard perçant", 10, 11, 0, 1 },
    { "SAIL", "Voile", 0, 0, 0, 1 },
    { "HARDENING_STRIKE", "Frappe durcissante", 6, 7, 6, 1 },
};

constexpr MonsterMoveData MOVES_THIEVING_HOPPER[] = {
    { "THIEVERY", "Vol", 17, 19, 0, 1 },
    { "NAB", "Prendre", 14, 16, 0, 1 },
    { "HAT_TRICK", "Tour de chapeau", 21, 23, 0, 1 },
    { "FLUTTER", "Battement d'ailes", 0, 0, 0, 1 },
    { "ESCAPE", "Fuite", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TOADPOLE[] = {
    { "SPIKE_SPIT", "Jet d'épines", 3, 4, 0, 3 },
    { "WHIRL", "Tourbillonnement", 7, 8, 0, 1 },
    { "SPIKEN", "Épines", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TORCH_HEAD_AMALGAM[] = {
    { "STRONG_TACKLE", "Charge puissante", 26, 32, 0, 1 },
    { "TACKLE_2", "Tackle", 18, 22, 0, 1 },
    { "BEAM", "Rayon", 8, 8, 0, 3 },
    { "TACKLE_3", "Tackle", 14, 16, 0, 1 },
    { "TACKLE_4", "Tackle", 14, 16, 0, 1 },
};

constexpr MonsterMoveData MOVES_TOUGH_EGG[] = {
    { "HATCH", "Éclosion", 0, 0, 0, 1 },
    { "NIBBLE", "Grignotage", 4, 5, 0, 1 },
};

constexpr MonsterMoveData MOVES_TRACKER_RUBY_RAIDER[] = {
    { "TRACK", "Traque", 0, 0, 0, 1 },
    { "HOUNDS", "Lâcher les chiens", 1, 1, 0, 8 },
};

constexpr MonsterMoveData MOVES_TUNNELER[] = {
    { "BITE", "Morsure", 13, 15, 0, 1 },
    { "BURROW", "Creuser", 0, 0, 32, 1 },
    { "BELOW", "Attaque par dessous", 23, 26, 0, 1 },
    { "DIZZY", "Étourdi", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TURRET_OPERATOR[] = {
    { "UNLOAD", "Déchargement !", 3, 4, 0, 5 },
    { "UNLOAD_MOVE_2", "Déchargement !", 3, 4, 0, 5 },
    { "RELOAD", "Rechargement", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TWIG_SLIME_M[] = {
    { "POKEY_POUNCE", "Petit saut", 11, 12, 0, 1 },
    { "STICKY_SHOT", "Tir collant", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_TWIG_SLIME_S[] = {
    { "TACKLE", "Charge", 4, 5, 0, 1 },
};

constexpr MonsterMoveData MOVES_TWO_TAILED_RAT[] = {
    { "SCRATCH", "Griffure", 8, 9, 0, 1 },
    { "DISEASE_BITE", "Morsure infectée", 6, 7, 0, 1 },
    { "SCREECH", "Cri", 0, 0, 0, 1 },
    { "CALL_FOR_BACKUP", "Appel à l'aide", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_VANTOM[] = {
    { "INK_BLOT", "Tache d'encre", 7, 8, 0, 1 },
    { "INKY_LANCE", "Javelot d'encre", 6, 7, 0, 2 },
    { "DISMEMBER", "Démembrement", 26, 30, 0, 1 },
    { "PREPARE", "Préparation", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_VINE_SHAMBLER[] = {
    { "GRASPING_VINES", "Lianes paralysantes", 8, 9, 0, 1 },
    { "SWIPE", "Griffe", 6, 7, 0, 2 },
    { "CHOMP", "Croc", 16, 18, 0, 1 },
};

constexpr MonsterMoveData MOVES_WATERFALL_GIANT[] = {
    { "PRESSURIZE", "Pressurisation", 0, 0, 0, 1 },
    { "STOMP", "Piétinement", 15, 16, 0, 1 },
    { "RAM", "Bélier", 10, 11, 0, 1 },
    { "SIPHON", "Siphon", 0, 0, 0, 1 },
    { "PRESSURE_GUN", "Pistolet à pression", 0, 0, 0, 1 },
    { "PRESSURE_UP", "Montée en pression", 13, 14, 0, 1 },
    { "ABOUT_TO_BLOW", "Surchauffe", 0, 0, 0, 1 },
    { "EXPLODE", "Explosion", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_WRIGGLER[] = {
    { "NASTY_BITE", "Morsure vicieuse", 6, 7, 0, 1 },
    { "WRIGGLE", "Tortillement", 0, 0, 0, 1 },
    { "SPAWNED", "Invoqué", 0, 0, 0, 1 },
};

constexpr MonsterMoveData MOVES_ZAPBOT[] = {
    { "ZAP", "Décharge", 14, 15, 0, 1 },
};

constexpr MonsterData ALL_MONSTERS[] = {
    { "AEONGLASS", "Clepséon", 512, 512, 535, 535, "boss", std::span<const MonsterMoveData>(MOVES_AEONGLASS) },
    { "ARCHITECT", "L'Architecte", 9999, 9999, 9999, 9999, "normal", std::span<const MonsterMoveData>() },
    { "ASSASSIN_RUBY_RAIDER", "Rôdeur assassin", 18, 23, 19, 24, "normal", std::span<const MonsterMoveData>(MOVES_ASSASSIN_RUBY_RAIDER) },
    { "AXEBOT", "Hachebots", 70, 78, 76, 86, "normal", std::span<const MonsterMoveData>(MOVES_AXEBOT) },
    { "AXE_RUBY_RAIDER", "Rôdeur à la hache", 20, 22, 21, 23, "normal", std::span<const MonsterMoveData>(MOVES_AXE_RUBY_RAIDER) },
    { "BOWLBUG_EGG", "Sphéropode (Œuf)", 21, 22, 23, 24, "normal", std::span<const MonsterMoveData>(MOVES_BOWLBUG_EGG) },
    { "BOWLBUG_NECTAR", "Sphéropode (Nectar)", 35, 38, 36, 39, "normal", std::span<const MonsterMoveData>(MOVES_BOWLBUG_NECTAR) },
    { "BOWLBUG_ROCK", "Sphéropode (Pierre)", 45, 48, 46, 49, "normal", std::span<const MonsterMoveData>(MOVES_BOWLBUG_ROCK) },
    { "BOWLBUG_SILK", "Sphéropode (Soie)", 40, 43, 41, 44, "normal", std::span<const MonsterMoveData>(MOVES_BOWLBUG_SILK) },
    { "BRUTE_RUBY_RAIDER", "Rôdeur Brute", 30, 33, 31, 34, "normal", std::span<const MonsterMoveData>(MOVES_BRUTE_RUBY_RAIDER) },
    { "BYGONE_EFFIGY", "Idole Oubliée", 127, 127, 132, 132, "elite", std::span<const MonsterMoveData>(MOVES_BYGONE_EFFIGY) },
    { "BYRDONIS", "Oizodon", 81, 84, 90, 90, "elite", std::span<const MonsterMoveData>(MOVES_BYRDONIS) },
    { "BYRDPIP", "Oizillon", 9999, 9999, 9999, 9999, "summon", std::span<const MonsterMoveData>() },
    { "CALCIFIED_CULTIST", "Adepte calcifié", 38, 41, 39, 42, "normal", std::span<const MonsterMoveData>(MOVES_CALCIFIED_CULTIST) },
    { "CEREMONIAL_BEAST", "Cerf Auguste", 252, 252, 262, 262, "boss", std::span<const MonsterMoveData>(MOVES_CEREMONIAL_BEAST) },
    { "CHOMPER", "Croquebot", 60, 64, 63, 67, "normal", std::span<const MonsterMoveData>(MOVES_CHOMPER) },
    { "CORPSE_SLUG", "Putrilimace", 25, 27, 27, 29, "normal", std::span<const MonsterMoveData>(MOVES_CORPSE_SLUG) },
    { "CROSSBOW_RUBY_RAIDER", "Rôdeur Arbalétrier", 18, 21, 19, 22, "normal", std::span<const MonsterMoveData>(MOVES_CROSSBOW_RUBY_RAIDER) },
    { "CRUSHER", "Anéantisseur", 209, 209, 219, 219, "boss", std::span<const MonsterMoveData>(MOVES_CRUSHER) },
    { "CUBEX_CONSTRUCT", "Assemblage Cubex", 65, 65, 70, 70, "normal", std::span<const MonsterMoveData>(MOVES_CUBEX_CONSTRUCT) },
    { "DAMP_CULTIST", "Adepte moisi", 51, 53, 52, 54, "normal", std::span<const MonsterMoveData>(MOVES_DAMP_CULTIST) },
    { "DECIMILLIPEDE_SEGMENT_BACK", "Dix-Mille-Pattes", 40, 46, 46, 52, "elite", std::span<const MonsterMoveData>(MOVES_DECIMILLIPEDE_SEGMENT_BACK) },
    { "DECIMILLIPEDE_SEGMENT_FRONT", "Dix-Mille-Pattes", 40, 46, 46, 52, "elite", std::span<const MonsterMoveData>(MOVES_DECIMILLIPEDE_SEGMENT_FRONT) },
    { "DECIMILLIPEDE_SEGMENT_MIDDLE", "Dix-Mille-Pattes", 40, 46, 46, 52, "elite", std::span<const MonsterMoveData>(MOVES_DECIMILLIPEDE_SEGMENT_MIDDLE) },
    { "DEPRECATED_MONSTER", "Monstre obsolète", 0, 0, 0, 0, "normal", std::span<const MonsterMoveData>() },
    { "DEVOTED_SCULPTOR", "Sculpteur dévoué", 162, 162, 172, 172, "weak", std::span<const MonsterMoveData>(MOVES_DEVOTED_SCULPTOR) },
    { "ENTOMANCER", "Entomancien", 145, 145, 165, 165, "elite", std::span<const MonsterMoveData>(MOVES_ENTOMANCER) },
    { "EXOSKELETON", "Exosquelette", 24, 28, 26, 30, "normal", std::span<const MonsterMoveData>(MOVES_EXOSKELETON) },
    { "EYE_WITH_TEETH", "Œil denté", 6, 6, 6, 6, "normal", std::span<const MonsterMoveData>(MOVES_EYE_WITH_TEETH) },
    { "FABRICATOR", "Machiniste", 150, 150, 155, 155, "normal", std::span<const MonsterMoveData>(MOVES_FABRICATOR) },
    { "FAKE_MERCHANT_MONSTER", "Le Marchand ???", 165, 165, 175, 175, "normal", std::span<const MonsterMoveData>(MOVES_FAKE_MERCHANT_MONSTER) },
    { "FAT_GREMLIN", "Gros diablotin", 13, 17, 14, 18, "normal", std::span<const MonsterMoveData>(MOVES_FAT_GREMLIN) },
    { "FLAIL_KNIGHT", "Chevalier au fléau", 101, 101, 108, 108, "elite", std::span<const MonsterMoveData>(MOVES_FLAIL_KNIGHT) },
    { "FLYCONID", "Mycoptère", 47, 49, 51, 53, "normal", std::span<const MonsterMoveData>(MOVES_FLYCONID) },
    { "FOGMOG", "Fongibrume", 74, 74, 78, 78, "normal", std::span<const MonsterMoveData>(MOVES_FOGMOG) },
    { "FOSSIL_STALKER", "Traqueur fossilisé", 51, 53, 54, 56, "normal", std::span<const MonsterMoveData>(MOVES_FOSSIL_STALKER) },
    { "FROG_KNIGHT", "Chevalier rainette", 191, 191, 199, 199, "normal", std::span<const MonsterMoveData>(MOVES_FROG_KNIGHT) },
    { "FUZZY_WURM_CRAWLER", "Rampeluche", 55, 57, 58, 59, "normal", std::span<const MonsterMoveData>(MOVES_FUZZY_WURM_CRAWLER) },
    { "GAS_BOMB", "Bombe à gaz", 7, 7, 8, 8, "normal", std::span<const MonsterMoveData>(MOVES_GAS_BOMB) },
    { "GLOBE_HEAD", "Globocéphale", 148, 148, 158, 158, "normal", std::span<const MonsterMoveData>(MOVES_GLOBE_HEAD) },
    { "GREMLIN_MERC", "Diablotin mercenaire", 47, 49, 51, 53, "normal", std::span<const MonsterMoveData>(MOVES_GREMLIN_MERC) },
    { "GUARDBOT", "Gardebot", 16, 20, 17, 21, "summon", std::span<const MonsterMoveData>(MOVES_GUARDBOT) },
    { "HAUNTED_SHIP", "Navire hanté", 63, 63, 67, 67, "normal", std::span<const MonsterMoveData>(MOVES_HAUNTED_SHIP) },
    { "HUNTER_KILLER", "Chasseur-tueur", 121, 121, 126, 126, "normal", std::span<const MonsterMoveData>(MOVES_HUNTER_KILLER) },
    { "INFESTED_PRISM", "Prisme infesté", 161, 161, 171, 171, "elite", std::span<const MonsterMoveData>(MOVES_INFESTED_PRISM) },
    { "INKLET", "Encrelin", 11, 17, 12, 18, "normal", std::span<const MonsterMoveData>(MOVES_INKLET) },
    { "KIN_FOLLOWER", "Disciple de la Tribu", 58, 59, 62, 63, "boss", std::span<const MonsterMoveData>(MOVES_KIN_FOLLOWER) },
    { "KIN_PRIEST", "Prêtre de la Tribu", 190, 190, 199, 199, "boss", std::span<const MonsterMoveData>(MOVES_KIN_PRIEST) },
    { "KNOWLEDGE_DEMON", "Démon de la Connaissance", 379, 379, 399, 399, "boss", std::span<const MonsterMoveData>(MOVES_KNOWLEDGE_DEMON) },
    { "LAGAVULIN_MATRIARCH", "Matriarche Lagavulin", 222, 222, 233, 233, "boss", std::span<const MonsterMoveData>(MOVES_LAGAVULIN_MATRIARCH) },
    { "LEAF_SLIME_M", "Limon feuillu (M)", 32, 35, 33, 36, "normal", std::span<const MonsterMoveData>(MOVES_LEAF_SLIME_M) },
    { "LEAF_SLIME_S", "Limon feuillu (S)", 11, 15, 12, 16, "normal", std::span<const MonsterMoveData>(MOVES_LEAF_SLIME_S) },
    { "LIVING_FOG", "Brouillard vivant", 80, 80, 82, 82, "normal", std::span<const MonsterMoveData>(MOVES_LIVING_FOG) },
    { "LIVING_SHIELD", "Bouclier vivant", 55, 55, 65, 65, "weak", std::span<const MonsterMoveData>(MOVES_LIVING_SHIELD) },
    { "LOUSE_PROGENITOR", "Progéniteur de poux", 134, 136, 138, 141, "normal", std::span<const MonsterMoveData>(MOVES_LOUSE_PROGENITOR) },
    { "MAGI_KNIGHT", "Chevalier mage", 82, 82, 89, 89, "elite", std::span<const MonsterMoveData>(MOVES_MAGI_KNIGHT) },
    { "MAWLER", "Écharneur", 72, 72, 76, 76, "normal", std::span<const MonsterMoveData>(MOVES_MAWLER) },
    { "MECHA_KNIGHT", "Méca-Chevalier", 300, 300, 320, 320, "elite", std::span<const MonsterMoveData>(MOVES_MECHA_KNIGHT) },
    { "MYSTERIOUS_KNIGHT", "Chevalier mystérieux", 101, 101, 108, 108, "normal", std::span<const MonsterMoveData>(MOVES_MYSTERIOUS_KNIGHT) },
    { "MYTE", "Sarcopte", 61, 67, 64, 69, "normal", std::span<const MonsterMoveData>(MOVES_MYTE) },
    { "NIBBIT", "Grignotin", 42, 46, 44, 48, "normal", std::span<const MonsterMoveData>(MOVES_NIBBIT) },
    { "NOISEBOT", "Sonobot", 18, 23, 19, 24, "summon", std::span<const MonsterMoveData>(MOVES_NOISEBOT) },
    { "OSTY", "Titos", 1, 1, 1, 1, "summon", std::span<const MonsterMoveData>() },
    { "OVICOPTER", "Ovicoptère", 124, 130, 126, 132, "normal", std::span<const MonsterMoveData>(MOVES_OVICOPTER) },
    { "OWL_MAGISTRATE", "Magistrix", 231, 231, 247, 247, "normal", std::span<const MonsterMoveData>(MOVES_OWL_MAGISTRATE) },
    { "PAELS_LEGION", "Légion de Paël", 9999, 9999, 9999, 9999, "normal", std::span<const MonsterMoveData>() },
    { "PARAFRIGHT", "Pareffroi", 21, 21, 21, 21, "normal", std::span<const MonsterMoveData>(MOVES_PARAFRIGHT) },
    { "PHANTASMAL_GARDENER", "Jardinier spectral", 26, 31, 27, 32, "elite", std::span<const MonsterMoveData>(MOVES_PHANTASMAL_GARDENER) },
    { "PHROG_PARASITE", "Parasite Krapo", 61, 64, 66, 68, "elite", std::span<const MonsterMoveData>(MOVES_PHROG_PARASITE) },
    { "PUNCH_CONSTRUCT", "Assemblage de pugilat", 55, 55, 60, 60, "normal", std::span<const MonsterMoveData>(MOVES_PUNCH_CONSTRUCT) },
    { "QUEEN", "Reine", 400, 400, 419, 419, "boss", std::span<const MonsterMoveData>(MOVES_QUEEN) },
    { "ROCKET", "Roquette", 199, 199, 209, 209, "boss", std::span<const MonsterMoveData>(MOVES_ROCKET) },
    { "SCROLL_OF_BITING", "Parchemord", 30, 37, 33, 39, "normal", std::span<const MonsterMoveData>(MOVES_SCROLL_OF_BITING) },
    { "SEAPUNK", "Voyou marin", 44, 46, 47, 49, "normal", std::span<const MonsterMoveData>(MOVES_SEAPUNK) },
    { "SEWER_CLAM", "Palourde d'égout", 56, 56, 58, 58, "normal", std::span<const MonsterMoveData>(MOVES_SEWER_CLAM) },
    { "SHRINKER_BEETLE", "Scarabrideur", 38, 40, 40, 42, "normal", std::span<const MonsterMoveData>(MOVES_SHRINKER_BEETLE) },
    { "SKULKING_COLONY", "Colonie de rôdeurs", 75, 75, 80, 80, "elite", std::span<const MonsterMoveData>(MOVES_SKULKING_COLONY) },
    { "SLIMED_BERSERKER", "Berserker de vase", 261, 261, 281, 281, "normal", std::span<const MonsterMoveData>(MOVES_SLIMED_BERSERKER) },
    { "SLITHERING_STRANGLER", "Étrangleur", 53, 55, 54, 56, "normal", std::span<const MonsterMoveData>(MOVES_SLITHERING_STRANGLER) },
    { "SLUDGE_SPINNER", "Tisse-boue", 37, 39, 41, 42, "weak", std::span<const MonsterMoveData>(MOVES_SLUDGE_SPINNER) },
    { "SLUMBERING_BEETLE", "Scarabée dormeur", 86, 86, 89, 89, "normal", std::span<const MonsterMoveData>(MOVES_SLUMBERING_BEETLE) },
    { "SNAPPING_JAXFRUIT", "Jaxier à clapet", 31, 33, 34, 36, "normal", std::span<const MonsterMoveData>(MOVES_SNAPPING_JAXFRUIT) },
    { "SNEAKY_GREMLIN", "Diablotin sournois", 10, 14, 11, 15, "normal", std::span<const MonsterMoveData>(MOVES_SNEAKY_GREMLIN) },
    { "SOUL_FYSH", "Poissâme", 211, 211, 221, 221, "boss", std::span<const MonsterMoveData>(MOVES_SOUL_FYSH) },
    { "SOUL_NEXUS", "Amalg'âmes", 234, 234, 254, 254, "elite", std::span<const MonsterMoveData>(MOVES_SOUL_NEXUS) },
    { "SPECTRAL_KNIGHT", "Chevalier spectral", 93, 93, 97, 97, "elite", std::span<const MonsterMoveData>(MOVES_SPECTRAL_KNIGHT) },
    { "SPINY_TOAD", "Crapineux", 116, 119, 121, 124, "normal", std::span<const MonsterMoveData>(MOVES_SPINY_TOAD) },
    { "STABBOT", "Poignard-bot", 18, 23, 19, 24, "summon", std::span<const MonsterMoveData>(MOVES_STABBOT) },
    { "TERROR_EEL", "Murène de l'effroi", 140, 140, 150, 150, "elite", std::span<const MonsterMoveData>(MOVES_TERROR_EEL) },
    // No HP in the reference: those of its first life, guessed (TEST_SUBJECT_HP in include/engine/entity.h).
    { "TEST_SUBJECT", "Cobaye", 100, 100, 110, 110, "boss", std::span<const MonsterMoveData>(MOVES_TEST_SUBJECT) },
    { "THE_ADVERSARY_MK_ONE", "L'Adversaire Mk 1", 100, 100, 100, 100, "normal", std::span<const MonsterMoveData>(MOVES_THE_ADVERSARY_MK_ONE) },
    { "THE_ADVERSARY_MK_THREE", "L'Adversaire Mk 3", 300, 300, 300, 300, "normal", std::span<const MonsterMoveData>(MOVES_THE_ADVERSARY_MK_THREE) },
    { "THE_ADVERSARY_MK_TWO", "L'Adversaire Mk 2", 200, 200, 200, 200, "normal", std::span<const MonsterMoveData>(MOVES_THE_ADVERSARY_MK_TWO) },
    { "THE_FORGOTTEN", "L'Oublié", 106, 106, 111, 111, "normal", std::span<const MonsterMoveData>(MOVES_THE_FORGOTTEN) },
    { "THE_INSATIABLE", "L'Insatiable", 321, 321, 341, 341, "boss", std::span<const MonsterMoveData>(MOVES_THE_INSATIABLE) },
    { "THE_LOST", "L'Égaré", 93, 93, 99, 99, "normal", std::span<const MonsterMoveData>(MOVES_THE_LOST) },
    { "THE_OBSCURA", "L'Obscura", 123, 123, 129, 129, "normal", std::span<const MonsterMoveData>(MOVES_THE_OBSCURA) },
    { "THIEVING_HOPPER", "Mante chapardeuse", 79, 79, 84, 84, "weak", std::span<const MonsterMoveData>(MOVES_THIEVING_HOPPER) },
    { "TOADPOLE", "Crapautard", 21, 25, 22, 26, "weak", std::span<const MonsterMoveData>(MOVES_TOADPOLE) },
    { "TORCH_HEAD_AMALGAM", "Amalgame Torchetête", 199, 199, 211, 211, "boss", std::span<const MonsterMoveData>(MOVES_TORCH_HEAD_AMALGAM) },
    { "TOUGH_EGG", "Œuf robuste", 14, 18, 15, 19, "normal", std::span<const MonsterMoveData>(MOVES_TOUGH_EGG) },
    { "TRACKER_RUBY_RAIDER", "Rôdeur Traqueur", 21, 25, 22, 26, "normal", std::span<const MonsterMoveData>(MOVES_TRACKER_RUBY_RAIDER) },
    { "TUNNELER", "Tunnelier", 87, 87, 92, 92, "normal", std::span<const MonsterMoveData>(MOVES_TUNNELER) },
    { "TURRET_OPERATOR", "Opérateur de tourelle", 41, 41, 51, 51, "weak", std::span<const MonsterMoveData>(MOVES_TURRET_OPERATOR) },
    { "TWIG_SLIME_M", "Limon brindille (M)", 26, 28, 27, 29, "normal", std::span<const MonsterMoveData>(MOVES_TWIG_SLIME_M) },
    { "TWIG_SLIME_S", "Limon brindille (S)", 7, 11, 8, 12, "normal", std::span<const MonsterMoveData>(MOVES_TWIG_SLIME_S) },
    { "TWO_TAILED_RAT", "Rat bicaudé", 17, 21, 18, 22, "normal", std::span<const MonsterMoveData>(MOVES_TWO_TAILED_RAT) },
    { "VANTOM", "Vantôme", 173, 173, 183, 183, "boss", std::span<const MonsterMoveData>(MOVES_VANTOM) },
    { "VINE_SHAMBLER", "Bourbelianes", 61, 61, 64, 64, "normal", std::span<const MonsterMoveData>(MOVES_VINE_SHAMBLER) },
    { "WATERFALL_GIANT", "Géant de la Cascade", 240, 240, 250, 250, "boss", std::span<const MonsterMoveData>(MOVES_WATERFALL_GIANT) },
    { "WRIGGLER", "Larve", 17, 21, 18, 22, "elite", std::span<const MonsterMoveData>(MOVES_WRIGGLER) },
    { "ZAPBOT", "Déchargebot", 18, 23, 19, 24, "summon", std::span<const MonsterMoveData>(MOVES_ZAPBOT) },
};

} // namespace

const MonsterData* find_monster(std::string_view id) {
    for (const auto& m : ALL_MONSTERS) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

std::span<const MonsterData> get_all_monsters() {
    return std::span<const MonsterData>(ALL_MONSTERS);
}

} // namespace engine