#include "engine/entity.h"
#include <algorithm>

namespace engine {

void Enemy::init(EnemyType enemy_type) {
    type = enemy_type;
    statuses.clear();
    block = 0;
    damage_percent = 100;
    damage_down_turns = 0;
    stolen = {};
    ascended_damage = false;
    escaped = false;
    last_heal = 0;
    hp_bonus = 0;
    for (IntentDebuff& debuff : last_debuffs) debuff = {};
    reset_pattern(1);

    if (type == EnemyType::CULTIST) {
        name = "Cultist";
        max_hp = 50;
        hp = 50;
    } else if (type == EnemyType::JAW_WORM) {
        name = "Jaw Worm";
        max_hp = 42;
        hp = 42;
    } else if (type == EnemyType::LOUSE) {
        name = "Morpion";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::GREMLIN_NOB) {
        name = "Gremlin Nob";
        max_hp = 82;
        hp = 82;
    } else if (type == EnemyType::CEREMONIAL_BEAST) {
        name = "Cerf Auguste";
        max_hp = 150;
        hp = 150;
    } else if (type == EnemyType::FUZZY_WURM_CRAWLER) {
        name = "Rampeluche";
        max_hp = 55;
        hp = 55;
    } else if (type == EnemyType::CROSSBOW_RUBY_RAIDER) {
        name = "Rôdeur Arbalétrier";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::BRUTE_RUBY_RAIDER) {
        name = "Rôdeur Brute";
        max_hp = 30;
        hp = 30;
    } else if (type == EnemyType::ASSASSIN_RUBY_RAIDER) {
        name = "Rôdeur assassin";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::AXE_RUBY_RAIDER) {
        name = "Rôdeur à la hache";
        max_hp = 20;
        hp = 20;
    } else if (type == EnemyType::CALCIFIED_CULTIST) {
        name = "Adepte calcifié";
        max_hp = 38;
        hp = 38;
    } else if (type == EnemyType::DAMP_CULTIST) {
        name = "Adepte moisi";
        max_hp = 51;
        hp = 51;
    } else if (type == EnemyType::SEWER_CLAM) {
        name = "Palourde d'égout";
        max_hp = 56;
        hp = 56;
    } else if (type == EnemyType::THE_FORGOTTEN) {
        name = "L'Oublié";
        max_hp = 106;
        hp = 106;
    } else if (type == EnemyType::THE_LOST) {
        name = "L'Égaré";
        max_hp = 93;
        hp = 93;
    } else if (type == EnemyType::LIVING_SHIELD) {
        name = "Bouclier vivant";
        max_hp = 55;
        hp = 55;
    } else if (type == EnemyType::FAT_GREMLIN) {
        name = "Gros diablotin";
        max_hp = 13;
        hp = 13;
    } else if (type == EnemyType::SNEAKY_GREMLIN) {
        name = "Diablotin sournois";
        max_hp = 10;
        hp = 10;
    } else if (type == EnemyType::VINE_SHAMBLER) {
        name = "Bourbelianes";
        max_hp = 61;
        hp = 61;
    } else if (type == EnemyType::EYE_WITH_TEETH) {
        name = "Œil denté";
        max_hp = 6;
        hp = 6;
    } else if (type == EnemyType::STABBOT) {
        name = "Poignard-bot";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::PARAFRIGHT) {
        name = "Pareffroi";
        max_hp = 21;
        hp = 21;
    } else if (type == EnemyType::ZAPBOT) {
        name = "Déchargebot";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::NOISEBOT) {
        name = "Sonobot";
        max_hp = 18;
        hp = 18;
    } else if (type == EnemyType::CUBEX_CONSTRUCT) {
        name = "Assemblage Cubex";
        max_hp = 65;
        hp = 65;
    } else if (type == EnemyType::PUNCH_CONSTRUCT) {
        name = "Assemblage de pugilat";
        max_hp = 55;
        hp = 55;
    } else if (type == EnemyType::BOWLBUG_NECTAR) {
        name = "Sphéropode (Nectar)";
        max_hp = 35;
        hp = 35;
    } else if (type == EnemyType::AXEBOT) {
        name = "Hachebots";
        max_hp = 70;
        hp = 70;
    } else if (type == EnemyType::TURRET_OPERATOR) {
        name = "Opérateur de tourelle";
        max_hp = 41;
        hp = 41;
    } else if (type == EnemyType::BOWLBUG_ROCK) {
        name = "Sphéropode (Pierre)";
        max_hp = 45;
        hp = 45;
    } else if (type == EnemyType::SKULKING_COLONY) {
        name = "Colonie de rôdeurs";
        max_hp = 75;
        hp = 75;
    } else if (type == EnemyType::MAGI_KNIGHT) {
        name = "Chevalier mage";
        max_hp = 82;
        hp = 82;
    } else if (type == EnemyType::BYGONE_EFFIGY) {
        name = "Idole Oubliée";
        max_hp = 127;
        hp = 127;
    } else if (type == EnemyType::INKLET) {
        name = "Encrelin";
        max_hp = 11;
        hp = 11;
    } else if (type == EnemyType::NIBBIT) {
        name = "Grignotin";
        max_hp = 42;
        hp = 42;
    } else if (type == EnemyType::FROG_KNIGHT) {
        name = "Chevalier rainette";
        max_hp = 191;
        hp = 191;
    } else if (type == EnemyType::BOWLBUG_EGG) {
        name = "Sphéropode (Œuf)";
        max_hp = 21;
        hp = 21;
    } else if (type == EnemyType::TOADPOLE) {
        name = "Crapautard";
        max_hp = 21;
        hp = 21;
    } else if (type == EnemyType::MAWLER) {
        name = "Écharneur";
        max_hp = 72;
        hp = 72;
    } else if (type == EnemyType::GUARDBOT) {
        name = "Gardebot";
        max_hp = 16;
        hp = 16;
    } else if (type == EnemyType::DEVOTED_SCULPTOR) {
        name = "Sculpteur dévoué";
        max_hp = 162;
        hp = 162;
    } else if (type == EnemyType::BOWLBUG_SILK) {
        name = "Sphéropode (Soie)";
        max_hp = 40;
        hp = 40;
    } else if (type == EnemyType::INFESTED_PRISM) {
        name = "Prisme infesté";
        max_hp = 161;
        hp = 161;
    } else if (type == EnemyType::FLAIL_KNIGHT) {
        name = "Chevalier au fléau";
        max_hp = 101;
        hp = 101;
    } else if (type == EnemyType::FLYCONID) {
        name = "Mycoptère";
        max_hp = 47;
        hp = 47;
    } else if (type == EnemyType::ENTOMANCER) {
        name = "Entomancien";
        max_hp = 145;
        hp = 145;
    } else if (type == EnemyType::CORPSE_SLUG) {
        name = "Putrilimace";
        max_hp = 25;
        hp = 25;
    } else if (type == EnemyType::GAS_BOMB) {
        name = "Bombe à gaz";
        max_hp = 7;
        hp = 7;
    } else if (type == EnemyType::GREMLIN_MERC) {
        name = "Diablotin mercenaire";
        max_hp = 47;
        hp = 47;
    } else if (type == EnemyType::GLOBE_HEAD) {
        name = "Globocéphale";
        max_hp = 148;
        hp = 148;
    } else if (type == EnemyType::LOUSE_PROGENITOR) {
        name = "Progéniteur de poux";
        max_hp = 134;
        hp = 134;
    } else if (type == EnemyType::HAUNTED_SHIP) {
        name = "Navire hanté";
        max_hp = 63;
        hp = 63;
    } else if (type == EnemyType::SLUDGE_SPINNER) {
        name = "Tisse-boue";
        max_hp = 37;
        hp = 37;
    } else if (type == EnemyType::SPECTRAL_KNIGHT) {
        name = "Chevalier spectral";
        max_hp = 93;
        hp = 93;
    } else if (type == EnemyType::HUNTER_KILLER) {
        name = "Chasseur-tueur";
        max_hp = 121;
        hp = 121;
    } else if (type == EnemyType::MECHA_KNIGHT) {
        name = "Méca-Chevalier";
        max_hp = 300;
        hp = 300;
    } else if (type == EnemyType::FOSSIL_STALKER) {
        name = "Traqueur fossilisé";
        max_hp = 51;
        hp = 51;
    } else if (type == EnemyType::SPINY_TOAD) {
        name = "Crapineux";
        max_hp = 116;
        hp = 116;
    } else if (type == EnemyType::SEAPUNK) {
        name = "Voyou marin";
        max_hp = 44;
        hp = 44;
    } else if (type == EnemyType::SPECTRAL_KNIGHT) {
        name = "Chevalier spectral";
        max_hp = 93;
        hp = 93;
    } else if (type == EnemyType::TRACKER_RUBY_RAIDER) {
        name = "Rôdeur Traqueur";
        max_hp = 21;
        hp = 21;
    } else if (type == EnemyType::OSTY) {
        name = "Titos";
        max_hp = 1;
        hp = 1;
    } else if (type == EnemyType::CHOMPER) {
        name = "Croquebot";
        max_hp = 60;
        hp = 60;
    } else if (type == EnemyType::SLUMBERING_BEETLE) {
        name = "Scarabée dormeur";
        max_hp = 86;
        hp = 86;
    } else if (type == EnemyType::TUNNELER) {
        name = "Tunnelier";
        max_hp = 87;
        hp = 87;
    } else if (type == EnemyType::SLIMED_BERSERKER) {
        name = "Berserker de vase";
        max_hp = 261;
        hp = 261;
    } else if (type == EnemyType::TERROR_EEL) {
        name = "Murène de l'effroi";
        max_hp = 140;
        hp = 140;
    } else if (type == EnemyType::SOUL_NEXUS) {
        name = "Amalg'âmes";
        max_hp = 234;
        hp = 234;
    } else {
        name = "Monstre";
        max_hp = 30;
        hp = 30;
    }

    plan_turn(1);
}

namespace {
enum TunnelerMove : uint8_t { TUNNELER_BITE, TUNNELER_BURROW, TUNNELER_BELOW, TUNNELER_DIZZY };
}

uint8_t Enemy::hp_spread() const {
    switch (type) {
        case EnemyType::BOWLBUG_SILK: return 3; // 40 to 43, 41 to 44 at Ascension
        case EnemyType::SLUDGE_SPINNER: return 2; // 37 to 39, 41 to 42 at Ascension
        case EnemyType::GUARDBOT: return 4;    // 16 to 20, 17 to 21 at Ascension
        case EnemyType::BOWLBUG_EGG: return 1; // 21 to 22, 23 to 24 at Ascension
        case EnemyType::TOADPOLE: return 4;    // 21 to 25, 22 to 26 at Ascension
        case EnemyType::SPINY_TOAD: return 3;  // 116 to 119, 121 to 124 at Ascension
        case EnemyType::BOWLBUG_NECTAR: return 3; // 35 to 38, 36 to 39 at Ascension
        case EnemyType::FLYCONID: return 2;    // 47 to 49, 51 to 53 at Ascension
        case EnemyType::AXEBOT: return 8;      // 70 to 78, 76 to 86 at Ascension
        case EnemyType::CORPSE_SLUG: return 2; // 25 to 27, 27 to 29 at Ascension
        case EnemyType::LOUSE_PROGENITOR: return 2; // 134 to 136, 138 to 140 at Ascension
        case EnemyType::GREMLIN_MERC: return 2; // 47 to 49, 51 to 53 at Ascension
        case EnemyType::FOSSIL_STALKER: return 2; // 51 to 53, 54 to 56 at Ascension
        case EnemyType::SEAPUNK: return 2;     // 44 to 46, 47 to 49 at Ascension
        case EnemyType::FAT_GREMLIN: return 4; // 13 to 17, 14 to 18 at Ascension
        case EnemyType::SNEAKY_GREMLIN: return 4; // 10 to 14, 11 to 15 at Ascension
        case EnemyType::STABBOT: return 5;     // 18 to 23, 19 to 24 at Ascension
        case EnemyType::ZAPBOT: return 5;      // 18 to 23, 19 to 24 at Ascension
        case EnemyType::NOISEBOT: return 5;    // 18 to 23, 19 to 24 at Ascension
        case EnemyType::INKLET: return 6; // 11 to 17, 12 to 18 at Ascension
        case EnemyType::NIBBIT: return 4;      // 42 to 46, 44 to 48 at Ascension
        case EnemyType::TRACKER_RUBY_RAIDER: return 4; // 21 to 25, 22 to 26 at Ascension
        case EnemyType::CHOMPER: return 4;     // 60 to 64, 63 to 67 at Ascension
        default: return 0;
    }
}

void Enemy::roll_hp(uint32_t seed) {
    if (hp_spread() == 0) return;
    // A few rounds of mixing: seeds that differ by little must not give the same HP.
    seed ^= seed >> 16;
    seed *= 0x7FEB352Du;
    seed ^= seed >> 15;
    max_hp = static_cast<int16_t>(max_hp - hp_bonus);
    hp_bonus = static_cast<uint8_t>(seed % (hp_spread() + 1u));
    max_hp = static_cast<int16_t>(max_hp + hp_bonus);
    hp = max_hp;
}

void Enemy::stun() {
    current_intent = EnemyIntent{};
    current_intent.type = EnemyIntentType::STUN;
    current_intent.description = "Étourdi";
}

int16_t Enemy::ascension_hp() const {
    switch (type) {
        case EnemyType::THE_FORGOTTEN: return 111;
        case EnemyType::THE_LOST: return 99;
        case EnemyType::LIVING_SHIELD: return 65;
        case EnemyType::FAT_GREMLIN: return 14;
        case EnemyType::SNEAKY_GREMLIN: return 11;
        case EnemyType::VINE_SHAMBLER: return 64;
        case EnemyType::STABBOT: return 19;
        case EnemyType::ZAPBOT: return 19;
        case EnemyType::NOISEBOT: return 19;
        case EnemyType::CUBEX_CONSTRUCT: return 70;
        case EnemyType::PUNCH_CONSTRUCT: return 60;
        case EnemyType::BOWLBUG_NECTAR: return 36;
        case EnemyType::AXEBOT: return 76;
        case EnemyType::BYGONE_EFFIGY: return 132;
        case EnemyType::TURRET_OPERATOR: return 51;
        case EnemyType::BOWLBUG_ROCK: return 46;
        case EnemyType::SKULKING_COLONY: return 80;
        case EnemyType::MAGI_KNIGHT: return 89;
        case EnemyType::INKLET: return 12;
        case EnemyType::NIBBIT: return 44;
        case EnemyType::FLYCONID: return 51;
        case EnemyType::INFESTED_PRISM: return 171;
        case EnemyType::FLAIL_KNIGHT: return 108;
        case EnemyType::CORPSE_SLUG: return 27;
        case EnemyType::GAS_BOMB: return 8;
        case EnemyType::GREMLIN_MERC: return 51;
        case EnemyType::HAUNTED_SHIP: return 67;
        case EnemyType::GLOBE_HEAD: return 158;
        case EnemyType::LOUSE_PROGENITOR: return 138;
        case EnemyType::SLUDGE_SPINNER: return 41;
        case EnemyType::BOWLBUG_EGG: return 23;
        case EnemyType::TOADPOLE: return 22;
        case EnemyType::HUNTER_KILLER: return 126;
        case EnemyType::MAWLER: return 76;
        case EnemyType::FOSSIL_STALKER: return 54;
        case EnemyType::SPINY_TOAD: return 121;
        case EnemyType::SEAPUNK: return 47;
        case EnemyType::TRACKER_RUBY_RAIDER: return 22;
        case EnemyType::CHOMPER: return 63;
        case EnemyType::SLUMBERING_BEETLE: return 89;
        case EnemyType::TUNNELER: return 92;
        case EnemyType::SLIMED_BERSERKER: return 281;
        case EnemyType::FROG_KNIGHT: return 199;
        case EnemyType::GUARDBOT: return 17;
        case EnemyType::DEVOTED_SCULPTOR: return 172;
        case EnemyType::BOWLBUG_SILK: return 41;
        case EnemyType::TERROR_EEL: return 150;
        case EnemyType::ENTOMANCER: return 165;
        case EnemyType::SPECTRAL_KNIGHT: return 97;
        case EnemyType::SOUL_NEXUS: return 254;
        case EnemyType::MECHA_KNIGHT: return 320;
        default: return 0;
    }
}

void Enemy::ascend(bool more_hp, bool more_damage) {
    if (more_hp && ascension_hp() > 0) {
        max_hp = static_cast<int16_t>(ascension_hp() + hp_bonus);
        hp = max_hp;
    }
    ascended_damage = more_damage;
    reset_pattern(move_seed);
    plan_turn(1);
}

void Enemy::reset_pattern(uint32_t seed) {
    phase = 0;
    phase_turn = 0;
    last_move = NO_MOVE;
    move_seed = seed ? seed : 1;
}

bool Enemy::enter_phase(uint8_t next) {
    if (next <= phase) return false;
    phase = next;
    phase_turn = 0;
    return true;
}

uint8_t Enemy::draw_move(std::span<const MoveOption> options) {
    if (options.empty()) return NO_MOVE;
    auto allowed = [this](const MoveOption& option) { return option.can_repeat || option.move != last_move; };
    uint32_t total = 0;
    for (const MoveOption& option : options) {
        if (allowed(option)) total += option.weight;
    }
    // The only move left is the one just played: it is played again, and no number is drawn.
    if (total == 0) return last_move;

    move_seed ^= move_seed << 13;
    move_seed ^= move_seed >> 17;
    move_seed ^= move_seed << 5;
    uint32_t pick = move_seed % total;
    for (const MoveOption& option : options) {
        if (!allowed(option)) continue;
        if (pick < option.weight) return use_move(option.move);
        pick -= option.weight;
    }
    return last_move;
}

void Enemy::return_stolen(Player& player) {
    if (stolen.type != StatusType::NONE && stolen.amount != 0) {
        player.add_status(stolen.type, static_cast<int16_t>(-stolen.amount));
    }
    stolen = {};
}

void Enemy::break_burrow() {
    if (type != EnemyType::TUNNELER || !enter_phase(1)) return;
    phase_turn = 1;
    use_move(TUNNELER_DIZZY);
    stun();
}

void Enemy::plan_turn(int turn_number, const PlanContext& context) {
    // Start from a blank intent: a field set by last turn's move (a Strength buff) must not leak
    // into this one, since execute_turn() applies whatever the intent holds.
    current_intent = EnemyIntent{};
    if (phase_turn < UINT8_MAX) ++phase_turn;
    if (type == EnemyType::CULTIST) {
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 0;
            current_intent.status_type = StatusType::RITUAL;
            current_intent.status_amount = 3;
            current_intent.description = "Incantation (+3 Ritual)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 6;
            current_intent.block = 0;
            current_intent.status_type = StatusType::NONE;
            current_intent.status_amount = 0;
            current_intent.description = "Dark Strike";
        }
    } else if (type == EnemyType::JAW_WORM) {
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 11;
            current_intent.block = 0;
            current_intent.description = "Chomp (11 dmg)";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = 7;
            current_intent.block = 5;
            current_intent.description = "Thrash (7 dmg, 5 blk)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 6;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Bellow (+3 Str, 6 blk)";
        }
    } else if (type == EnemyType::LOUSE) {
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 6;
            current_intent.block = 0;
            current_intent.description = "Bite (6 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = 5;
            current_intent.block = 5;
            current_intent.description = "Spite (5 dmg, 5 blk)";
        }
    } else if (type == EnemyType::GREMLIN_NOB) {
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Bellow (+2 Str)";
        } else if (turn_number == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 8;
            current_intent.block = 0;
            current_intent.description = "Skull Bash (8 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 14;
            current_intent.block = 0;
            current_intent.description = "Rush (14 dmg)";
        }
    } else if (type == EnemyType::CEREMONIAL_BEAST) {
        // Opens with a roar, then alternates a heavy trample with an antler guard.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Brame (+2 Force)";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 12;
            current_intent.block = 0;
            current_intent.description = "Pietinement (12 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = 7;
            current_intent.block = 12;
            current_intent.description = "Ramure (7 dmg, 12 blk)";
        }
    } else if (type == EnemyType::FUZZY_WURM_CRAWLER) {
        // Acid Goop, Inhale, Acid Goop, and again.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 7;
            current_intent.description = "Inspiration (+7 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 4;
            current_intent.description = "Acide visqueux (4 dmg)";
        }
    } else if (type == EnemyType::CROSSBOW_RUBY_RAIDER) {
        // Reloads, then fires.
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.base_damage = 0;
            current_intent.block = 3;
            current_intent.description = "Rechargement (3 blk)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 14;
            current_intent.block = 0;
            current_intent.description = "Feu ! (14 dmg)";
        }
    } else if (type == EnemyType::BRUTE_RUBY_RAIDER) {
        // Beats, then roars.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 7;
            current_intent.description = "Rouer (7 dmg)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Rugissement (+3 Force)";
        }
    } else if (type == EnemyType::ASSASSIN_RUBY_RAIDER) {
        // Killshot every turn.
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = 10;
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        current_intent.description = "Tir meurtrier (10 dmg)";
    } else if (type == EnemyType::AXE_RUBY_RAIDER) {
        // Two Swings, then a Big Swing.
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 3 == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 12;
            current_intent.block = 0;
            current_intent.description = "Balancier (12 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = 5;
            current_intent.block = 5;
            current_intent.description = "Swing (5 dmg, 5 blk)";
        }
    } else if (type == EnemyType::TRACKER_RUBY_RAIDER) {
        // Track once (2 Frail), then Unleash the Hounds for the rest of the fight.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.base_damage = 0;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Traque (2 Fragilisé)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 1;
            current_intent.hits = ascended_damage ? 9 : 8;
            current_intent.description = "Lâcher les chiens (1 dmg x8)";
        }
    } else if (type == EnemyType::CALCIFIED_CULTIST) {
        // Incantation once, then Dark Strike for the rest of the fight.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.status_type = StatusType::RITUAL;
            current_intent.status_amount = 2;
            current_intent.description = "Incantation (+2 Rituel)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 9;
            current_intent.description = "Frappe ténébreuse (9 dmg)";
        }
    } else if (type == EnemyType::DAMP_CULTIST) {
        // Incantation once, then Dark Strike for the rest of the fight.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.status_type = StatusType::RITUAL;
            current_intent.status_amount = 5;
            current_intent.description = "Incantation (+5 Rituel)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 1;
            current_intent.description = "Frappe ténébreuse (1 dmg)";
        }
    } else if (type == EnemyType::SEWER_CLAM) {
        // Jet, then Pressurize.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 10;
            current_intent.description = "Jet (10 dmg)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 4;
            current_intent.description = "Pressurisation (+4 Force)";
        }
    } else if (type == EnemyType::THE_FORGOTTEN) {
        // Miasma, then Dread, and again.
        if (turn_number % 2 == 1) {
            // Takes 2 Dexterity from the player and keeps it for itself.
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.block = 8;
            current_intent.status_type = StatusType::DEXTERITY;
            current_intent.status_amount = 2;
            current_intent.debuffs[0] = { StatusType::DEXTERITY, -2 };
            current_intent.steals = true;
            current_intent.description = "Miasme (8 blk, -2 Dex)";
        } else {
            // Hits harder with each point of Dexterity it holds.
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 15 : 13;
            current_intent.damage_status = StatusType::DEXTERITY;
            current_intent.description = "Angoisse (13 dmg + Dex)";
        }
    } else if (type == EnemyType::THE_LOST) {
        // Debilitating Smog, then Eye Lasers, and again.
        if (turn_number % 2 == 1) {
            // Takes 2 Strength from the player and keeps it for itself.
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.debuffs[0] = { StatusType::STRENGTH, -2 };
            current_intent.steals = true;
            current_intent.description = "Brouillard paralysant (-2 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 5 : 4;
            current_intent.hits = 2;
            current_intent.description = "Lasers oculaires (4x2 dmg)";
        }
    } else if (type == EnemyType::LIVING_SHIELD) {
        // Shield Slam while it has someone to protect; once alone, Smash until the end.
        if (turn_number > 1 && context.allies == 0 && enter_phase(1)) phase_turn = 1;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 6;
            current_intent.description = "Heurt de bouclier (6 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 18 : 16;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Fracas (16 dmg, +3 Force)";
        }
    } else if (type == EnemyType::FAT_GREMLIN) {
        // Wakes up dazed, then runs away.
        if (turn_number == 1) {
            stun();
            current_intent.description = "Éveil (étourdi)";
        } else {
            current_intent.type = EnemyIntentType::ESCAPE;
            current_intent.description = "Fuite";
        }
    } else if (type == EnemyType::SNEAKY_GREMLIN) {
        // Wakes up dazed, then Tackles every turn.
        if (turn_number == 1) {
            stun();
            current_intent.description = "Éveil (étourdi)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 10 : 9;
            current_intent.description = "Charge (9 dmg)";
        }
    } else if (type == EnemyType::VINE_SHAMBLER) {
        // Swipe, Grasping Vines, Chomp, and again.
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.hits = 2;
            current_intent.description = "Griffe (6 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.power = PowerId::TANGLED;
            current_intent.power_amount = 1;
            current_intent.power_on_player = true;
            current_intent.power_turn_only = true;
            current_intent.description = "Lianes paralysantes (8 dmg, Paralysie)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 18 : 16;
            current_intent.description = "Croc (16 dmg)";
        }
    } else if (type == EnemyType::EYE_WITH_TEETH) {
        // Distract, every turn.
        current_intent.type = EnemyIntentType::STATUS;
        current_intent.status_card = CardId::DAZED;
        current_intent.status_cards = 3;
        current_intent.status_pile = Pile::DISCARD;
        current_intent.description = "Distraction (3 Hébété)";
    } else if (type == EnemyType::GAS_BOMB) {
        // Explode every turn: 8 damage (9 at Ascension).
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = ascended_damage ? 9 : 8;
        current_intent.description = "Explosion (8 dmg)";
    } else if (type == EnemyType::SPINY_TOAD) {
        // Protruding Spikes (+5 Thorns), Spike Explosion (-5 Thorns), Tongue Lash, and again.
        current_intent.status_type = StatusType::THORNS;
        const int phase = (turn_number - 1) % 3;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_amount = 5;
            current_intent.description = "Épines saillantes (+5 Épines)";
        } else if (phase == 1) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 25 : 23;
            current_intent.status_amount = -5;
            current_intent.description = "Explosion épineuse (23 dmg, -5 Épines)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.status_type = StatusType::NONE;
            current_intent.base_damage = ascended_damage ? 19 : 17;
            current_intent.description = "Fouet de langue (17 dmg)";
        }
    } else if (type == EnemyType::STABBOT) {
        // Stab every turn: 1 Frail.
        current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
        current_intent.base_damage = ascended_damage ? 12 : 11;
        current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
        current_intent.description = "Coup de poignard (11 dmg, Fragilisé)";
    } else if (type == EnemyType::PARAFRIGHT) {
        // Slam every turn.
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = ascended_damage ? 17 : 16;
        current_intent.description = "Heurt (16 dmg)";
    } else if (type == EnemyType::ZAPBOT) {
        // Zap every turn: 14 damage (15 at Ascension).
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = ascended_damage ? 15 : 14;
        current_intent.description = "Décharge (14 dmg)";
    } else if (type == EnemyType::NOISEBOT) {
        // Noise every turn: 2 Dazed into the discard pile.
        current_intent.type = EnemyIntentType::STATUS;
        current_intent.status_card = CardId::DAZED;
        current_intent.status_cards = 2;
        current_intent.status_pile = Pile::DISCARD;
        current_intent.description = "Ébruitement (2 Hébété)";
    } else if (type == EnemyType::CUBEX_CONSTRUCT) {
        // Charge Up, two Repeater Blasts, Expel, then the blasts and Expel again.
        current_intent.status_type = StatusType::STRENGTH;
        current_intent.status_amount = 2;
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.description = "Chargement (+2 Force)";
        } else if ((turn_number - 2) % 3 != 2) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 8 : 7;
            current_intent.description = "Tir répéteur (7 dmg, +2 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 6 : 5;
            current_intent.hits = 2;
            current_intent.status_type = StatusType::NONE;
            current_intent.status_amount = 0;
            current_intent.description = "Éjection (5 dmg x2)";
        }
    } else if (type == EnemyType::PUNCH_CONSTRUCT) {
        // Ready (10 Block), Fast Punch (2 hits, 1 Frail), Strong Punch, and again.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = 10;
            current_intent.description = "PRÊT (10 Armure)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 6 : 5;
            current_intent.hits = 2;
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Poing rapide (5 dmg x2, Fragilisé)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 16 : 14;
            current_intent.description = "Poing puissant (14 dmg)";
        }
    } else if (type == EnemyType::BOWLBUG_NECTAR) {
        // Thrash (3), Buff (+15 Strength, +16 at Ascension), then Thrash again for good.
        if (turn_number == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 16 : 15;
            current_intent.description = "Buff (+15 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 3;
            current_intent.description = "Furie (3 dmg)";
        }
    } else if (type == EnemyType::AXEBOT) {
        // Boot Up (10 Block, 1 Strength), then Hammer Uppercut and One-Two in turn.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.block = 10;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 1;
            current_intent.description = "Démarrage (10 Armure, +1 Force)";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 18 : 14;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.debuffs[1] = { StatusType::WEAK, 2 };
            current_intent.description = "Marteau Uppercut (14 dmg, Fragilisé, Affaibli)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 11 : 10;
            current_intent.hits = 2;
            current_intent.description = "Droite-gauche (10 dmg x2)";
        }
    } else if (type == EnemyType::TURRET_OPERATOR) {
        // Unload twice (5 hits of 3), Reload (+1 Strength), and again.
        if ((turn_number - 1) % 3 != 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.hits = 5;
            current_intent.description = "Déchargement ! (3 dmg x5)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 0;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 1;
            current_intent.description = "Rechargement (+1 Force)";
        }
    } else if (type == EnemyType::BOWLBUG_ROCK) {
        // Headbutt every turn; when the player's Block soaked it all, it is off balance (phase 1, set by the
        // fight) and loses the next turn (Dizzy).
        if (phase == 1) {
            stun();
            phase = 0;
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 16 : 15;
            current_intent.description = "Coup de boule (15 dmg)";
        }
    } else if (type == EnemyType::SKULKING_COLONY) {
        // Zoom twice, Inertia (+2 Strength, +4 at Ascension), Piercing Stabs, and again.
        const int step = (turn_number - 1) % 4;
        current_intent.type = EnemyIntentType::ATTACK;
        if (step <= 1) {
            current_intent.base_damage = ascended_damage ? 16 : 14;
            current_intent.description = "Bourdonnement (14 dmg)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 11 : 9;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 4 : 2;
            current_intent.description = "Inertie (9 dmg, +2 Force)";
        } else {
            current_intent.base_damage = ascended_damage ? 8 : 7;
            current_intent.hits = 2;
            current_intent.description = "Coups perforants (7 dmg x2)";
        }
    } else if (type == EnemyType::MAGI_KNIGHT) {
        // Power Shield, Dampen, then Ram, Prep and Magic Bomb in a loop.
        enum Move : uint8_t { POWER_SHIELD, DAMPEN, RAM, PREP, MAGIC_BOMB };
        const Move move = turn_number == 1 ? POWER_SHIELD : turn_number == 2 ? DAMPEN
                          : static_cast<Move>(RAM + (turn_number - 3) % 3);
        if (move == POWER_SHIELD) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.block = 5;
            current_intent.description = "Bouclier de puissance (6 dmg, 5 blk)";
        } else if (move == DAMPEN) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.description = "Atténuation";
        } else if (move == RAM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 11 : 10;
            current_intent.description = "Bélier (10 dmg)";
        } else if (move == PREP) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = 5;
            current_intent.description = "Prep (5 Armure)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 40 : 35;
            current_intent.description = "Bombe magique (35 dmg)";
        }
    } else if (type == EnemyType::BYGONE_EFFIGY) {
        // Sleep (a lost turn), Wake (+10 Strength), then Slashes every turn. Hurt while it sleeps, it skips
        // the Wake: one more lost turn, then Slashes without the Strength.
        if (phase == 0 && turn_number == 2 && hp < max_hp && enter_phase(1)) phase_turn = 1;
        if (phase == 0 && turn_number == 1) {
            current_intent.type = EnemyIntentType::STUN;
            current_intent.description = "Sommeil";
        } else if (phase == 0 && turn_number == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 10;
            current_intent.description = "Éveil (+10 Force)";
        } else if (phase == 1 && phase_turn == 1) {
            current_intent.type = EnemyIntentType::STUN;
            current_intent.description = "Sommeil";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 15 : 13;
            current_intent.description = "Bourrasque (13 dmg)";
        }
    } else if (type == EnemyType::INKLET) {
        // Jab on the odd turns; on the even ones, Whirlwind or Piercing Gaze at random.
        constexpr uint8_t JAB = 0;
        constexpr uint8_t WHIRLWIND = 1;
        constexpr uint8_t PIERCING_GAZE = 2;
        uint8_t move = JAB;
        if (turn_number % 2 == 0) {
            const MoveOption options[] = { { WHIRLWIND }, { PIERCING_GAZE } };
            move = draw_move(options);
        } else {
            use_move(JAB);
        }
        current_intent.type = EnemyIntentType::ATTACK;
        if (move == WHIRLWIND) {
            current_intent.base_damage = ascended_damage ? 3 : 2;
            current_intent.hits = 3;
            current_intent.description = "Tourbillon (2 dmg x3)";
        } else if (move == PIERCING_GAZE) {
            current_intent.base_damage = ascended_damage ? 11 : 10;
            current_intent.description = "Regard perçant (10 dmg)";
        } else {
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.description = "Coup sec (3 dmg)";
        }
    } else if (type == EnemyType::MAWLER) {
        // Claw first, then a draw among Rip and Tear, Roar and Claw: never the move just played, and Roar only
        // once (`phase` becomes 1 once it was played).
        enum Move : uint8_t { RIP_AND_TEAR, ROAR, CLAW };
        if (phase_turn <= 1) {
            use_move(CLAW);
        } else {
            static constexpr MoveOption ALL[] = { { RIP_AND_TEAR }, { ROAR }, { CLAW } };
            static constexpr MoveOption NO_ROAR[] = { { RIP_AND_TEAR }, { CLAW } };
            use_move(phase == 0 ? draw_move(ALL) : draw_move(NO_ROAR));
            if (last_move == ROAR) phase = 1;
        }
        if (last_move == RIP_AND_TEAR) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 16 : 14;
            current_intent.description = "Déchirure (14 dmg)";
        } else if (last_move == ROAR) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 3 };
            current_intent.description = "Rugissement (Vulnérable 3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 5 : 4;
            current_intent.hits = 2;
            current_intent.description = "Griffe (4 dmg x2)";
        }
    } else if (type == EnemyType::NIBBIT) {
        // Butt, Slice, Hiss, and again. Where it starts depends on its place at the start of the fight: alone it
        // begins with Butt, in front with Slice, behind with Hiss. `phase` holds that offset.
        if (turn_number == 1) phase = context.allies == 0 ? 0 : (context.slot == 0 ? 1 : 2);
        uint8_t move = static_cast<uint8_t>((phase + phase_turn - 1) % 3);
        if (move == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 13 : 12;
            current_intent.description = "Derrière (12 dmg)";
        } else if (move == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.block = ascended_damage ? 6 : 5;
            current_intent.description = "Slice (6 dmg, 5 blk)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 3 : 2;
            current_intent.description = "Chuintement (+2 Force)";
        }
    } else if (type == EnemyType::CORPSE_SLUG) {
        // Whip Slap, Glomp, Goop, and again.
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 3;
            current_intent.hits = 2;
            current_intent.description = "Fouet (3 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.description = "Câlin brutal (8 dmg)";
        } else {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Mucus (Fragilisé 2)";
        }
    } else if (type == EnemyType::GREMLIN_MERC) {
        // Gimme, Double Smash, Hehe, and again.
        current_intent.hits = 2;
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 8 : 7;
            current_intent.description = "Donne-moi ça ! (7 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.debuffs[0] = { StatusType::WEAK, 2 };
            current_intent.description = "Fracas double (6 dmg x2, Affaibli 2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.hits = 1;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Héhé (8 dmg, +2 Force)";
        }
    } else if (type == EnemyType::GLOBE_HEAD) {
        // Shocking Slap (2 Frail), Thunder Strike (3 hits), Galvanic Burst (+2 Strength), and again.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 14 : 13;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Claque de choc (13 dmg, Fragilisé 2)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.hits = 3;
            current_intent.description = "Canalisation kéraunique (6 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 17 : 16;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Galvanic Burst (16 dmg, +2 Force)";
        }
    } else if (type == EnemyType::LOUSE_PROGENITOR) {
        // Web Cannon, Curl and Grow, Pounce, and again.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 10 : 9;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Canon de toile (9 dmg, Fragilisé 2)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = ascended_damage ? 18 : 14;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 7 : 5;
            current_intent.description = "Repli fortifiant (14 Armure, +5 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 16 : 14;
            current_intent.description = "Bond (14 dmg)";
        }
    } else if (type == EnemyType::HAUNTED_SHIP) {
        // Haunt, then Swipe and Stomp, and again from Swipe.
        const int step = turn_number == 1 ? 0 : 1 + (turn_number - 2) % 2;
        if (step == 0) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::WEAK, 3 };
            current_intent.status_card = CardId::DAZED;
            current_intent.status_cards = 5;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Hanter (Affaibli 3, 5 Hébété)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 14 : 13;
            current_intent.description = "Griffe (13 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 5 : 4;
            current_intent.hits = 3;
            current_intent.description = "Piétinement (4 dmg x3)";
        }
    } else if (type == EnemyType::GUARDBOT) {
        // Guard (15 Block), every turn.
        current_intent.type = EnemyIntentType::DEFEND;
        current_intent.block = 15;
        current_intent.description = "Garde (15 Armure)";
    } else if (type == EnemyType::BOWLBUG_SILK) {
        // Toxic Spit (1 Weak), Thrash 4x2, and again.
        if ((turn_number - 1) % 2 == 0) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::WEAK, 1 };
            current_intent.description = "Crachat toxique (Affaibli 1)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 5 : 4;
            current_intent.hits = 2;
            current_intent.description = "Furie (4 dmg x2)";
        }
    } else if (type == EnemyType::BOWLBUG_EGG) {
        // Bite (7 damage, 7 Block) every turn.
        current_intent.type = EnemyIntentType::ATTACK_DEFEND;
        current_intent.base_damage = ascended_damage ? 8 : 7;
        current_intent.block = 7;
        current_intent.description = "Morsure (7 dmg, 7 blk)";
    } else if (type == EnemyType::TOADPOLE) {
        // Three moves in a loop, started by its place: in front Spiken, Spike Spit, Whirl; behind Whirl, Spiken,
        // Spike Spit. `phase` holds the offset in the cycle SPIKEN, SPIKE_SPIT, WHIRL.
        if (turn_number == 1) phase = context.slot == 0 ? 0 : 2;
        uint8_t move = static_cast<uint8_t>((phase + phase_turn - 1) % 3);
        if (move == 0) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::THORNS;
            current_intent.status_amount = 2;
            current_intent.description = "Épines (+2 Épines)";
        } else if (move == 1) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.hits = 3;
            current_intent.status_type = StatusType::THORNS;
            current_intent.status_amount = 2;
            current_intent.description = "Jet d'épines (3x3 dmg, +2 Épines)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 8 : 7;
            current_intent.description = "Tourbillon (7 dmg)";
        }
    } else if (type == EnemyType::TERROR_EEL) {
        // Crash, Thrash (+6 Vigor), and again. Once at half its HP or less (triggered), it is stunned, then
        // casts Terror (Vulnerable 99) and goes back to Crash and Thrash for good.
        enum Move : uint8_t { CRASH, THRASH, STUNNED, TERROR };
        if (phase == 0 && hp * 2 <= max_hp && enter_phase(1)) phase_turn = 1;
        uint8_t move;
        if (phase == 1 && phase_turn == 1) move = STUNNED;
        else if (phase == 1 && phase_turn == 2) move = TERROR;
        else if (phase == 1) move = (phase_turn % 2 == 1) ? CRASH : THRASH;
        else move = (phase_turn % 2 == 1) ? CRASH : THRASH;
        use_move(move);
        if (move == STUNNED) {
            stun();
        } else if (move == TERROR) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 99 };
            current_intent.description = "Aura de terreur (Vulnérable 99)";
        } else if (move == CRASH) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 18 : 16;
            current_intent.description = "Fracas (16 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.hits = 3;
            current_intent.status_type = StatusType::VIGOR;
            current_intent.status_amount = 6;
            current_intent.description = "Furie (3x3 dmg, +6 Vigueur)";
        }
    } else if (type == EnemyType::SEAPUNK) {
        // Sea Kick, Spinning Kick, Bubble Burp, and again.
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 13 : 11;
            current_intent.description = "Coup de pied marin (11 dmg)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 2;
            current_intent.hits = 4;
            current_intent.description = "Coup de pied tournoyant (2 dmg x4)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.block = ascended_damage ? 8 : 7;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 2 : 1;
            current_intent.description = "Jet de bulles (7 blk, +1 Force)";
        }
    } else if (type == EnemyType::OSTY) {
        // Titos, summoned by the Entomancer, has no move: it does nothing, turn after turn.
        current_intent.type = EnemyIntentType::STUN;
        current_intent.description = "Ne fait rien";
    } else if (type == EnemyType::CHOMPER) {
        // Clamp (two hits), Screech (3 Dazed into the discard pile), and again.
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.hits = 2;
            current_intent.description = "Pince (8 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::DAZED;
            current_intent.status_cards = 3;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Crissement (3 Hébété)";
        }
    } else if (type == EnemyType::DEVOTED_SCULPTOR) {
        // Forbidden Incantation (+9 Ritual) once, then Savage for the rest of the fight.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::RITUAL;
            current_intent.status_amount = 9;
            current_intent.description = "Incantation interdite (+9 Rituel)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 15 : 12;
            current_intent.description = "Sauvagerie (12 dmg)";
        }
    } else if (type == EnemyType::INFESTED_PRISM) {
        // Jab, Radiate, Whirlwind, Pulsate, and again.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 17 : 15;
            current_intent.description = "Coup sec (15 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = ascended_damage ? 13 : 11;
            current_intent.block = ascended_damage ? 13 : 11;
            current_intent.description = "Irradiation (11 dmg, 11 blk)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 6 : 5;
            current_intent.hits = 3;
            current_intent.description = "Tourbillon (5 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = ascended_damage ? 10 : 8;
            current_intent.block = ascended_damage ? 22 : 20;
            current_intent.power = PowerId::VITAL_SPARK;
            current_intent.power_amount = ascended_damage ? 3 : 2;
            current_intent.description = "Pulsation (8 dmg, 20 blk, Étincelle)";
        }
    } else if (type == EnemyType::FLAIL_KNIGHT) {
        // Ram first, then a draw among War Chant, Flail and Ram: War Chant never twice in a row.
        enum Move : uint8_t { WAR_CHANT, FLAIL, RAM };
        if (turn_number <= 1) {
            use_move(RAM);
        } else {
            static constexpr MoveOption OPTIONS[] = { { WAR_CHANT }, { FLAIL, 1, true }, { RAM, 1, true } };
            draw_move(OPTIONS);
        }
        if (last_move == WAR_CHANT) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Chant de guerre (+3 Force)";
        } else if (last_move == FLAIL) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 10 : 9;
            current_intent.hits = 2;
            current_intent.description = "Fléau (9 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 17 : 15;
            current_intent.description = "Bélier (15 dmg)";
        }
    } else if (type == EnemyType::FLYCONID) {
        // Phase 0: Frail Spores or Smash, never the same twice in a row. Once hurt (triggered), it opens the
        // next phase with Vulnerable Spores, then draws among the three moves, never the same twice in a row.
        enum Move : uint8_t { VULNERABLE_SPORES, FRAIL_SPORES, SMASH };
        if (phase == 0 && hp < max_hp && enter_phase(1)) {
            phase_turn = 1;
            use_move(VULNERABLE_SPORES);
        } else if (phase == 0) {
            static constexpr MoveOption OPENING[] = { { FRAIL_SPORES }, { SMASH } };
            use_move(draw_move(OPENING));
        } else if (phase_turn > 1) {
            static constexpr MoveOption ANY[] = { { VULNERABLE_SPORES }, { FRAIL_SPORES }, { SMASH } };
            use_move(draw_move(ANY));
        }
        if (last_move == VULNERABLE_SPORES) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 2 };
            current_intent.description = "Spores fragilisantes (Vulnérable 2)";
        } else if (last_move == FRAIL_SPORES) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Spores fragilisantes (8 dmg, Fragilisé 2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 12 : 11;
            current_intent.description = "Fracas (11 dmg)";
        }
    } else if (type == EnemyType::FROG_KNIGHT) {
        // Tongue Lash, Strike Down Evil, For the Queen, and again. Below half HP it opens a new phase, once,
        // with Beetle Charge, then goes back to the cycle.
        enum Move : uint8_t { TONGUE_LASH, STRIKE_DOWN_EVIL, FOR_THE_QUEEN, BEETLE_CHARGE };
        if (phase == 0 && hp < max_hp / 2 && enter_phase(1)) {
            phase_turn = 1;
            use_move(BEETLE_CHARGE);
        } else if (phase == 0 || phase_turn > 1) {
            use_move(static_cast<uint8_t>((phase_turn - (phase == 0 ? 1 : 2)) % 3));
        }
        if (last_move == TONGUE_LASH) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 14 : 13;
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Fouet de langue (13 dmg, Fragilisé 2)";
        } else if (last_move == STRIKE_DOWN_EVIL) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 23 : 21;
            current_intent.description = "Expier le mal (21 dmg)";
        } else if (last_move == FOR_THE_QUEEN) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 5;
            current_intent.description = "Pour la Reine (Force 5)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 40 : 35;
            current_intent.description = "Charge scarabée (35 dmg)";
        }
    } else if (type == EnemyType::SLUDGE_SPINNER) {
        // Calm: Oil Spray every turn. Once hurt (triggered), it opens the next phase with Rage, then draws among
        // the three moves, never the same twice in a row.
        enum Move : uint8_t { OIL_SPRAY, SLAM, RAGE };
        if (phase == 0 && hp < max_hp && enter_phase(1)) {
            phase_turn = 1;
            use_move(RAGE);
        } else if (phase == 0) {
            use_move(OIL_SPRAY);
        } else if (phase_turn > 1) {
            static constexpr MoveOption ANY[] = { { OIL_SPRAY }, { SLAM }, { RAGE } };
            use_move(draw_move(ANY));
        }
        if (last_move == OIL_SPRAY) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 9 : 8;
            current_intent.debuffs[0] = { StatusType::WEAK, 1 };
            current_intent.description = "Spray huileux (8 dmg, Affaibli 1)";
        } else if (last_move == SLAM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 12 : 11;
            current_intent.description = "Heurt (11 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Rage (6 dmg, Force 3)";
        }
    } else if (type == EnemyType::ENTOMANCER) {
        // Bees, Spear, Pheromone Spit (Personal Hive 1, Strength 2 then 1), and again.
        const int phase = (turn_number - 1) % 3;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = 3;
            current_intent.hits = ascended_damage ? 8 : 7;
            current_intent.description = "Bzzzzzz !! (3 dmg x7)";
        } else if (phase == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 20 : 18;
            current_intent.description = "Javelot ! (18 dmg)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.power = PowerId::PERSONAL_HIVE;
            current_intent.power_amount = 1;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Crachat de phéromones (Ruche 1, Force 3)";
        }
    } else if (type == EnemyType::SPECTRAL_KNIGHT) {
        // Hex, Soul Slash, then a draw between Soul Slash and Soul Flame (Soul Flame never twice in a row).
        enum Move : uint8_t { HEX, SOUL_SLASH, SOUL_FLAME };
        if (turn_number == 1) {
            use_move(HEX);
        } else if (turn_number == 2) {
            use_move(SOUL_SLASH);
        } else {
            static constexpr MoveOption AFTER[] = { { SOUL_SLASH, 1, true }, { SOUL_FLAME } };
            use_move(draw_move(AFTER));
        }
        if (last_move == HEX) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power = PowerId::HEX;
            current_intent.power_amount = 2;
            current_intent.description = "Sortilège (Éthéré)";
        } else if (last_move == SOUL_SLASH) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 17 : 15;
            current_intent.description = "Frappe spirituelle (15 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.hits = 3;
            current_intent.description = "Flamme spirituelle (3 dmg x3)";
        }
    } else if (type == EnemyType::MECHA_KNIGHT) {
        // Charge, Flamethrower, Windup, Heavy Cleave, then Flamethrower, Windup, Heavy Cleave again.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        const int step = turn_number == 1 ? 0 : 1 + (turn_number - 2) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 30 : 25;
            current_intent.description = "Charge (25 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 12 : 8;
            current_intent.status_card = CardId::BURN;
            current_intent.status_cards = 4;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Lance-flammes (8 dmg, 4 Brûlure)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.base_damage = 0;
            current_intent.block = 15;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 5;
            current_intent.description = "Chargement (15 Armure, Force 5)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 40 : 35;
            current_intent.description = "Balayage lourd (35 dmg)";
        }
    } else if (type == EnemyType::HUNTER_KILLER) {
        // Tenderizing Goop, Bite, Puncture, then a draw between Bite (never twice in a row) and Puncture.
        enum Move : uint8_t { TENDERIZING_GOOP, BITE, PUNCTURE };
        if (turn_number == 1) {
            use_move(TENDERIZING_GOOP);
        } else if (turn_number == 2) {
            use_move(BITE);
        } else if (turn_number == 3) {
            use_move(PUNCTURE);
        } else {
            static constexpr MoveOption AFTER[] = { { BITE }, { PUNCTURE, 1, true } };
            use_move(draw_move(AFTER));
        }
        if (last_move == TENDERIZING_GOOP) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power = PowerId::TENDER;
            current_intent.power_amount = 1;
            current_intent.power_on_player = true;
            current_intent.description = "Suc de décomposition (Décomposition 1)";
        } else if (last_move == BITE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 19 : 17;
            current_intent.description = "Morsure (17 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 8 : 7;
            current_intent.hits = 3;
            current_intent.description = "Perforation (7 dmg x3)";
        }
    } else if (type == EnemyType::FOSSIL_STALKER) {
        // Phase 0: Latch every turn. Once hurt (triggered), it opens the next phase with Lash, then draws among
        // Latch, Tackle and Lash, a move being allowed to repeat.
        enum Move : uint8_t { TACKLE, LATCH, LASH };
        if (phase == 0 && hp < max_hp && enter_phase(1)) {
            phase_turn = 1;
            use_move(LASH);
        } else if (phase == 0) {
            use_move(LATCH);
        } else if (phase_turn > 1) {
            static constexpr MoveOption ANY[] = { { LATCH, 1, true }, { TACKLE, 1, true }, { LASH, 1, true } };
            use_move(draw_move(ANY));
        }
        if (last_move == TACKLE) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 11 : 9;
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Charge (9 dmg, Fragilisé 1)";
        } else if (last_move == LATCH) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 14 : 12;
            current_intent.description = "Accrochage (12 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 4 : 3;
            current_intent.hits = 2;
            current_intent.description = "Fouet (3x2 dmg)";
        }
    } else if (type == EnemyType::SLUMBERING_BEETLE) {
        // Snores (loses its turn) while it sleeps; the first hit that costs it HP wakes it for good, and it
        // then rolls out every turn: 16 damage, +2 Strength.
        if (phase == 0 && hp < max_hp) enter_phase(1);
        if (phase == 0) {
            current_intent.type = EnemyIntentType::STUN;
            current_intent.description = "Ronflement";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = ascended_damage ? 18 : 16;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Roulade (16 dmg, +2 Force)";
        }
    } else if (type == EnemyType::TUNNELER) {
        // Phase 0: Bite, Burrow (Block and Burrowed), Below, and Below again. If the Block of the Burrow is broken
        // (triggered, see break_burrow), the next phase opens with Dizzy (a lost turn), then Bite, Burrow and
        // Below over and over.
        if (phase == 0) {
            use_move(phase_turn == 1 ? TUNNELER_BITE : (phase_turn == 2 ? TUNNELER_BURROW : TUNNELER_BELOW));
        } else {
            use_move(phase_turn == 1 ? TUNNELER_DIZZY : (phase_turn == 2 ? TUNNELER_BITE : (phase_turn == 3 ? TUNNELER_BURROW : TUNNELER_BELOW)));
        }
        if (last_move == TUNNELER_DIZZY) {
            stun();
        } else if (last_move == TUNNELER_BITE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 15 : 13;
            current_intent.description = "Morsure (13 dmg)";
        } else if (last_move == TUNNELER_BURROW) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = 32;
            current_intent.power = PowerId::BURROWED;
            current_intent.power_amount = 1;
            current_intent.description = "Creuser (32 blk, Enfoui)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 26 : 23;
            current_intent.description = "Attaque par dessous (23 dmg)";
        }
    } else if (type == EnemyType::SLIMED_BERSERKER) {
        // Vomit Ichor (10 Slimed), Furious Pummeling, Leeching Hug (+3 Strength, 3 Weak), Smother, and again.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::SLIMED;
            current_intent.status_cards = 10;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Régurgitation d'ichor (10 Englué)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 5 : 4;
            current_intent.hits = 4;
            current_intent.description = "Ruée de coups (4 dmg x4)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.debuffs[0] = { StatusType::WEAK, 3 };
            current_intent.description = "Câlin sangsue (Force 3, Affaibli 3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 33 : 30;
            current_intent.description = "Smother (30 dmg)";
        }
    } else if (type == EnemyType::SOUL_NEXUS) {
        // Phase 0: Soul Burn every turn. Once hurt (triggered), it opens the next phase with Life Drain, then
        // draws among Soul Burn, Maelstrom and Life Drain, never the move it just played.
        enum Move : uint8_t { SOUL_BURN, MAELSTROM, DRAIN_LIFE };
        if (phase == 0 && hp < max_hp && enter_phase(1)) {
            phase_turn = 1;
            use_move(DRAIN_LIFE);
        } else if (phase == 0) {
            use_move(SOUL_BURN);
        } else if (phase_turn > 1) {
            static constexpr MoveOption ANY[] = { { SOUL_BURN }, { MAELSTROM }, { DRAIN_LIFE } };
            use_move(draw_move(ANY));
        }
        if (last_move == SOUL_BURN) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 31 : 29;
            current_intent.description = "Brûlure d'âme (29 dmg)";
        } else if (last_move == MAELSTROM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = ascended_damage ? 7 : 6;
            current_intent.hits = 4;
            current_intent.description = "Maelstrom (6x4 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = ascended_damage ? 19 : 18;
            current_intent.debuffs[0] = { StatusType::WEAK, 2 };
            current_intent.debuffs[1] = { StatusType::VULNERABLE, 2 };
            current_intent.description = "Drain de vie (18 dmg, Affaibli 2, Vulnérable 2)";
        }
    }
}

void Enemy::execute_turn(Player& player, int16_t damage_back, bool reflect, bool halved) {
    clear_block();
    decay_plating();
    last_heal = 0;

    // Ritual gained this turn only starts granting Strength at the end of the next one.
    int16_t ritual = get_status(StatusType::RITUAL);

    // Gain block from intent if any
    if (current_intent.block > 0) {
        gain_block(current_intent.block);
    }

    // Apply status buff to self if any. A move that also attacks hits first (see below).
    const bool buff_after_attack = current_intent.type == EnemyIntentType::ATTACK_BUFF;
    if (!buff_after_attack && current_intent.status_type != StatusType::NONE && current_intent.status_amount != 0) {
        add_status(current_intent.status_type, current_intent.status_amount);
    }

    // Execute attack if any, one hit after the other
    if (current_intent.base_damage > 0) {
        for (uint8_t hit = 0; hit < current_intent.hits && is_alive() && player.is_alive(); ++hit) {
            const int16_t block_before = player.block;
            player.take_damage_with_minion(intent_damage(player, halved));
            // The attacker gets hurt back: the player's Thorns, Flame Barrier, and the damage Reflect sends back.
            int16_t back = static_cast<int16_t>(player.get_status(StatusType::THORNS) + damage_back);
            if (reflect) back = static_cast<int16_t>(back + block_before - player.block);
            take_damage(back);
        }
        clear_status(StatusType::VIGOR); // spent on this attack
    }

    if (buff_after_attack && is_alive() && current_intent.status_amount != 0) {
        add_status(current_intent.status_type, current_intent.status_amount);
    }

    if (current_intent.heal > 0 && is_alive()) {
        last_heal = static_cast<int16_t>(std::min<int>(current_intent.heal, max_hp - hp));
        hp = static_cast<int16_t>(hp + last_heal);
    }

    // Statuses given to the player. They reach a player the attack left alive.
    for (size_t i = 0; i < MAX_INTENT_DEBUFFS; ++i) {
        const IntentDebuff& debuff = current_intent.debuffs[i];
        last_debuffs[i] = {};
        if (debuff.type == StatusType::NONE || debuff.amount == 0 || !player.is_alive()) continue;
        if (!player.receive_status(debuff.type, debuff.amount)) continue; // cancelled by Artifact
        last_debuffs[i] = debuff;
        player.fresh_debuffs |= CombatEntity::fresh_bit(debuff.type);
        if (current_intent.steals) {
            stolen.type = debuff.type;
            stolen.amount = static_cast<int16_t>(stolen.amount + debuff.amount);
        }
    }

    // End of turn status triggers (Ritual -> Strength, Plating -> Block, Regen -> HP)
    if (ritual > 0) {
        add_status(StatusType::STRENGTH, ritual);
    }
    if (is_alive()) {
        apply_plating_tick();
        apply_regen_tick();
    }

    // Tick end-of-turn status counters (Vulnerable, Weak)
    tick_end_of_turn_statuses();

    // Gone: nothing can reach it any more, and it was not killed.
    if (current_intent.type == EnemyIntentType::ESCAPE && is_alive()) {
        escaped = true;
        hp = 0;
        block = 0;
    }
}

int16_t Enemy::intent_damage(const CombatEntity& target, bool halved) const {
    if (current_intent.base_damage <= 0) return 0;
    int16_t base = static_cast<int16_t>(current_intent.base_damage * damage_percent / 100);
    if (current_intent.damage_status != StatusType::NONE) {
        base = static_cast<int16_t>(std::max(0, base + get_status(current_intent.damage_status)));
    }
    int16_t damage = attack_damage(base, *this, target, damage_down_turns > 0 ? 70 : 100);
    if (target.halve_vulnerable_damage && get_status(StatusType::VULNERABLE) > 0) damage = static_cast<int16_t>(damage / 2);
    if (halved) damage = static_cast<int16_t>(damage / 2); // Undying Sigil: enemy with Doom at least its HP
    return damage;
}

void Player::add_stars(uint8_t count) {
    sovereign_stars = (sovereign_stars + count > 99) ? 99 : (sovereign_stars + count);
}

void Player::summon_minion(int16_t m_hp, int16_t m_block) {
    if (m_hp > 0) {
        minion_hp += m_hp;
        minion_max_hp += m_hp;
    }
    minion_block += m_block;
}

int16_t Player::heal_minion(int16_t amount) {
    if (amount <= 0 || !minion_alive()) return 0;
    int16_t before = minion_hp;
    minion_hp = std::min<int16_t>(minion_max_hp, static_cast<int16_t>(minion_hp + amount));
    return static_cast<int16_t>(minion_hp - before);
}

int16_t Player::kill_minion() {
    int16_t left = minion_hp;
    minion_hp = 0;
    minion_max_hp = 0;
    minion_block = 0;
    return left;
}

} // namespace engine
