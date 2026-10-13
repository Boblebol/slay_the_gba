#include "engine/entity.h"
#include "engine/monster.h"
#include <algorithm>
#include <iterator>

namespace engine {

namespace {
// How each EnemyType reads its sheet in the monster database (src/engine/monster.cpp), in EnemyType order.
struct EnemySheet {
    std::string_view id; // "" for the monsters that do not follow a sheet (see monster_sheet)
    bool hp_range;       // its HP are drawn between the lowest and the highest of the sheet (see hp_spread)
    bool ascension_hp;   // it takes the Ascension HP of the sheet, not the percentages of the run (see ascension_hp)
};

constexpr EnemySheet ENEMY_SHEETS[] = {
    { "", false, false }, // CULTIST
    { "", false, false }, // JAW_WORM
    { "", false, false }, // LOUSE
    { "", false, false }, // GREMLIN_NOB
    { "CEREMONIAL_BEAST", false, true },
    { "FUZZY_WURM_CRAWLER", false, false },
    { "CROSSBOW_RUBY_RAIDER", false, false },
    { "BRUTE_RUBY_RAIDER", false, false },
    { "ASSASSIN_RUBY_RAIDER", false, false },
    { "AXE_RUBY_RAIDER", false, false },
    { "CALCIFIED_CULTIST", false, false },
    { "DAMP_CULTIST", false, false },
    { "SEWER_CLAM", false, false },
    { "THE_FORGOTTEN", false, true },
    { "LIVING_SHIELD", false, true },
    { "FAT_GREMLIN", true, true },
    { "VINE_SHAMBLER", false, true },
    { "EYE_WITH_TEETH", false, false },
    { "STABBOT", true, true },
    { "NOISEBOT", true, true },
    { "CUBEX_CONSTRUCT", false, true },
    { "PUNCH_CONSTRUCT", false, true },
    { "INKLET", true, true },
    { "NIBBIT", true, true },
    { "FLYCONID", true, true },
    { "CORPSE_SLUG", true, true },
    { "GAS_BOMB", false, true },
    { "GREMLIN_MERC", true, true },
    { "SNEAKY_GREMLIN", true, true },
    { "HAUNTED_SHIP", false, true },
    { "SLUDGE_SPINNER", true, true },
    { "TOADPOLE", true, true },
    { "HUNTER_KILLER", false, true },
    { "MAWLER", false, true },
    { "FOSSIL_STALKER", true, true },
    { "SPINY_TOAD", true, true },
    { "SEAPUNK", true, true },
    { "TRACKER_RUBY_RAIDER", true, true },
    { "OSTY", false, false },
    { "CHOMPER", true, true },
    { "SLUMBERING_BEETLE", false, true },
    { "PARAFRIGHT", false, false },
    { "TUNNELER", false, true },
    { "ZAPBOT", true, true },
    { "SLIMED_BERSERKER", false, true },
    { "FROG_KNIGHT", false, true },
    { "GLOBE_HEAD", false, true },
    { "LOUSE_PROGENITOR", true, true },
    { "GUARDBOT", true, true },
    { "BOWLBUG_NECTAR", true, true },
    { "DEVOTED_SCULPTOR", false, true },
    { "THE_LOST", false, true },
    { "AXEBOT", true, true },
    { "TURRET_OPERATOR", false, true },
    { "BOWLBUG_SILK", true, true },
    { "BOWLBUG_ROCK", false, true },
    { "BOWLBUG_EGG", true, true },
    { "SKULKING_COLONY", false, true },
    { "TERROR_EEL", false, true },
    { "ENTOMANCER", false, true },
    { "INFESTED_PRISM", false, true },
    { "FLAIL_KNIGHT", false, true },
    { "SPECTRAL_KNIGHT", false, true },
    { "MAGI_KNIGHT", false, true },
    { "SOUL_NEXUS", false, true },
    { "MECHA_KNIGHT", false, true },
    { "BYGONE_EFFIGY", false, true },
    { "BYRDONIS", true, true },
    { "PHROG_PARASITE", true, true },
    { "WRIGGLER", true, true },
    { "THE_INSATIABLE", false, true },
    { "TOUGH_EGG", true, true },
    { "THE_OBSCURA", false, true },
    { "OVICOPTER", true, true },
    { "FABRICATOR", false, true },
    { "SHRINKER_BEETLE", true, true },
    { "OWL_MAGISTRATE", false, true },
    { "SCROLL_OF_BITING", true, true },
    { "FOGMOG", false, true },
    { "SNAPPING_JAXFRUIT", true, true },
    { "TWIG_SLIME_M", true, true },
    { "TWIG_SLIME_S", true, true },
    { "LEAF_SLIME_M", true, true },
    { "LEAF_SLIME_S", true, true },
    { "SLITHERING_STRANGLER", true, true },
    { "LIVING_FOG", false, true },
    { "TWO_TAILED_RAT", true, true },
    { "LAGAVULIN_MATRIARCH", false, true },
    { "VANTOM", false, true },
    { "TEST_SUBJECT", false, true },
    { "KNOWLEDGE_DEMON", false, true },
    { "SOUL_FYSH", false, true },
    { "WATERFALL_GIANT", false, true },
    { "AEONGLASS", false, true },
    { "KIN_FOLLOWER", true, true },
    { "KIN_PRIEST", false, true },
};
static_assert(std::size(ENEMY_SHEETS) == static_cast<size_t>(EnemyType::KIN_PRIEST) + 1,
              "one sheet per EnemyType");

const EnemySheet* sheet_entry(EnemyType type) {
    const size_t index = static_cast<size_t>(type);
    return index < std::size(ENEMY_SHEETS) ? &ENEMY_SHEETS[index] : nullptr;
}

constexpr MonsterMoveData NO_MOVE_DATA = { "", "", 0, 0, 0, 1 };
} // namespace

const MonsterData* monster_sheet(EnemyType type) {
    const EnemySheet* entry = sheet_entry(type);
    if (entry == nullptr || entry->id.empty()) return nullptr;
    return find_monster(entry->id);
}

const MonsterMoveData& Enemy::sheet_move(std::string_view move_id) const {
    if (const MonsterData* sheet = monster_sheet(type)) {
        for (const MonsterMoveData& move : sheet->moves) {
            if (move.id == move_id) return move;
        }
    }
    return NO_MOVE_DATA;
}

int16_t Enemy::move_damage(std::string_view move_id) const {
    const MonsterMoveData& move = sheet_move(move_id);
    return ascended_damage ? move.damage_asc : move.damage;
}

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
    wounding_hits = 0;
    explosion = 0;
    hp_loss_budget = -1;
    hp_bonus = 0;
    for (IntentDebuff& debuff : last_debuffs) debuff = {};
    reset_pattern(1);

    if (const MonsterData* sheet = monster_sheet(type)) {
        name = sheet->name;
        max_hp = sheet->min_hp;
        hp = max_hp;
    } else if (type == EnemyType::CULTIST) {
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
    } else {
        name = "Monstre";
        max_hp = 30;
        hp = 30;
    }
    if (type == EnemyType::LAGAVULIN_MATRIARCH) add_status(StatusType::PLATED_ARMOR, LAGAVULIN_PLATING);

    plan_turn(1);
}

namespace {
enum TunnelerMove : uint8_t { TUNNELER_BITE, TUNNELER_BURROW, TUNNELER_BELOW, TUNNELER_DIZZY };
enum CeremonialBeastMove : uint8_t { BEAST_STAMP, BEAST_PLOW, BEAST_STUN, BEAST_CRY, BEAST_STOMP, BEAST_CRUSH };
enum TestSubjectMove : uint8_t {
    SUBJECT_RESPAWN, SUBJECT_BITE, SUBJECT_SKULL_BASH, SUBJECT_MULTI_CLAW, SUBJECT_LACERATE, SUBJECT_POUNCE,
    SUBJECT_GROWL
};
}

uint8_t Enemy::hp_spread() const {
    const EnemySheet* entry = sheet_entry(type);
    if (entry == nullptr || !entry->hp_range) return 0;
    const MonsterData* sheet = monster_sheet(type);
    if (sheet == nullptr) return 0;
    return static_cast<uint8_t>(sheet->max_hp - sheet->min_hp);
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
    const EnemySheet* entry = sheet_entry(type);
    if (entry == nullptr || !entry->ascension_hp) return 0;
    const MonsterData* sheet = monster_sheet(type);
    if (sheet == nullptr) return 0;
    // The Byrdonis always has the same HP at Ascension: ascend() adds hp_bonus back.
    if (type == EnemyType::BYRDONIS) return static_cast<int16_t>(sheet->min_hp_asc - hp_bonus);
    return sheet->min_hp_asc;
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

void Enemy::apply_ascension(int16_t hp_pct, int16_t damage_pct) {
    if (ascension_hp() > 0) {
        ascend(hp_pct > 100, damage_pct > 100);
        return;
    }
    max_hp = static_cast<int16_t>(max_hp * hp_pct / 100);
    hp = max_hp;
    damage_percent = damage_pct;
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

void Enemy::break_plow() {
    if (type != EnemyType::CEREMONIAL_BEAST || !enter_phase(1)) return;
    phase_turn = 1;
    use_move(BEAST_STUN);
    stun();
}

bool Enemy::respawn() {
    if (type != EnemyType::TEST_SUBJECT || is_alive() || escaped || phase >= TEST_SUBJECT_RESPAWNS) return false;
    const bool ascended_hp = max_hp == TEST_SUBJECT_HP_ASC[phase];
    enter_phase(static_cast<uint8_t>(phase + 1));
    max_hp = ascended_hp ? TEST_SUBJECT_HP_ASC[phase] : TEST_SUBJECT_HP[phase];
    hp = max_hp;
    block = 0;
    statuses.clear();
    fresh_debuffs = 0;
    temp_strength_loss = 0;
    damage_down_turns = 0;
    plan_turn(0); // Respawn, the first move of its new life
    return true;
}

bool Enemy::about_to_blow(int16_t steam) {
    if (type != EnemyType::WATERFALL_GIANT || is_alive() || escaped || steam <= 0 || !enter_phase(1)) return false;
    hp = 1;
    hp_loss_budget = 0; // nothing hurts it any more: only its Explosion ends it
    block = 0;
    statuses.clear();
    fresh_debuffs = 0;
    temp_strength_loss = 0;
    damage_down_turns = 0;
    explosion = steam;
    phase_turn = 0;
    plan_turn(0); // About to Blow
    return true;
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
        // Phase 0: Stamp (Plow on itself) once, then Plow (attack, +2 Strength) every turn. Phase 1, entered when
        // its HP fall below the Plow (break_plow): stunned once, then Beast Cry (Ringing), Stomp, Crush, in a loop.
        uint8_t move = BEAST_STAMP;
        if (phase == 0) move = turn_number == 1 ? BEAST_STAMP : BEAST_PLOW;
        else if (phase_turn == 1) move = BEAST_STUN;
        else move = static_cast<uint8_t>(BEAST_CRY + (phase_turn - 2) % 3);
        use_move(move);
        if (move == BEAST_STAMP) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.power = PowerId::PLOW;
            current_intent.power_amount = ascended_damage ? CEREMONIAL_BEAST_PLOW_ASC : CEREMONIAL_BEAST_PLOW;
            current_intent.description = "Trépignation (Cavalcade)";
        } else if (move == BEAST_PLOW) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("PLOW");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Cavalcade (18 dmg, +2 Force)";
        } else if (move == BEAST_STUN) {
            stun();
        } else if (move == BEAST_CRY) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power = PowerId::RINGING;
            current_intent.power_amount = 1;
            current_intent.power_on_player = true;
            current_intent.power_turn_only = true;
            current_intent.description = "Cri bestial (Résonnant)";
        } else if (move == BEAST_STOMP) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("STOMP");
            current_intent.description = "Piétinement (15 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("CRUSH");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 4 : 3;
            current_intent.description = "Écrasement (17 dmg, +3 Force)";
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
            current_intent.base_damage = sheet_move("FIRST_ACID_GOOP").damage;
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
            current_intent.base_damage = sheet_move("FIRE").damage;
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
            current_intent.base_damage = sheet_move("BEAT").damage;
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
        current_intent.base_damage = sheet_move("KILLSHOT").damage;
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
            current_intent.base_damage = sheet_move("BIG_SWING").damage;
            current_intent.block = 0;
            current_intent.description = "Balancier (12 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = sheet_move("SWING_1").damage;
            current_intent.block = sheet_move("SWING_1").block;
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
            current_intent.base_damage = move_damage("HOUNDS");
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
            current_intent.base_damage = sheet_move("DARK_STRIKE").damage;
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
            current_intent.base_damage = sheet_move("DARK_STRIKE").damage;
            current_intent.description = "Frappe ténébreuse (1 dmg)";
        }
    } else if (type == EnemyType::SEWER_CLAM) {
        // Jet, then Pressurize.
        current_intent.block = 0;
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = sheet_move("JET").damage;
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
            current_intent.block = sheet_move("MIASMA").block;
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
            current_intent.base_damage = move_damage("EYE_LASERS");
            current_intent.hits = sheet_move("EYE_LASERS").hits;
            current_intent.description = "Lasers oculaires (4x2 dmg)";
        }
    } else if (type == EnemyType::LIVING_SHIELD) {
        // Shield Slam while it has someone to protect; once alone, Smash until the end.
        if (turn_number > 1 && context.allies == 0 && enter_phase(1)) phase_turn = 1;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SHIELD_SLAM");
            current_intent.description = "Heurt de bouclier (6 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("SMASH");
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
            current_intent.base_damage = move_damage("TACKLE");
            current_intent.description = "Charge (9 dmg)";
        }
    } else if (type == EnemyType::VINE_SHAMBLER) {
        // Swipe, Grasping Vines, Chomp, and again.
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SWIPE");
            current_intent.hits = sheet_move("SWIPE").hits;
            current_intent.description = "Griffe (6 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("GRASPING_VINES");
            current_intent.power = PowerId::TANGLED;
            current_intent.power_amount = 1;
            current_intent.power_on_player = true;
            current_intent.power_turn_only = true;
            current_intent.description = "Lianes paralysantes (8 dmg, Paralysie)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CHOMP");
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
        current_intent.base_damage = move_damage("EXPLODE");
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
            current_intent.base_damage = move_damage("SPIKE_EXPLOSION");
            current_intent.status_amount = -5;
            current_intent.description = "Explosion épineuse (23 dmg, -5 Épines)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.status_type = StatusType::NONE;
            current_intent.base_damage = move_damage("TONGUE_LASH");
            current_intent.description = "Fouet de langue (17 dmg)";
        }
    } else if (type == EnemyType::STABBOT) {
        // Stab every turn: 1 Frail.
        current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
        current_intent.base_damage = move_damage("STAB");
        current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
        current_intent.description = "Coup de poignard (11 dmg, Fragilisé)";
    } else if (type == EnemyType::PARAFRIGHT) {
        // Slam every turn.
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = move_damage("SLAM");
        current_intent.description = "Heurt (16 dmg)";
    } else if (type == EnemyType::ZAPBOT) {
        // Zap every turn: 14 damage (15 at Ascension).
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = move_damage("ZAP");
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
            current_intent.base_damage = move_damage("REPEATER_BLAST");
            current_intent.description = "Tir répéteur (7 dmg, +2 Force)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("EXPEL");
            current_intent.hits = sheet_move("EXPEL").hits;
            current_intent.status_type = StatusType::NONE;
            current_intent.status_amount = 0;
            current_intent.description = "Éjection (5 dmg x2)";
        }
    } else if (type == EnemyType::PUNCH_CONSTRUCT) {
        // Ready (10 Block), Fast Punch (2 hits, 1 Frail), Strong Punch, and again.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = sheet_move("READY").block;
            current_intent.description = "PRÊT (10 Armure)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("FAST_PUNCH");
            current_intent.hits = sheet_move("FAST_PUNCH").hits;
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Poing rapide (5 dmg x2, Fragilisé)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("STRONG_PUNCH");
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
            current_intent.base_damage = move_damage("THRASH");
            current_intent.description = "Furie (3 dmg)";
        }
    } else if (type == EnemyType::AXEBOT) {
        // Boot Up (10 Block, 1 Strength), then Hammer Uppercut and One-Two in turn.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.block = sheet_move("BOOT_UP").block;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 1;
            current_intent.description = "Démarrage (10 Armure, +1 Force)";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("HAMMER_UPPERCUT");
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.debuffs[1] = { StatusType::WEAK, 2 };
            current_intent.description = "Marteau Uppercut (14 dmg, Fragilisé, Affaibli)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("ONE_TWO");
            current_intent.hits = sheet_move("ONE_TWO").hits;
            current_intent.description = "Droite-gauche (10 dmg x2)";
        }
    } else if (type == EnemyType::TURRET_OPERATOR) {
        // Unload twice (5 hits of 3), Reload (+1 Strength), and again.
        if ((turn_number - 1) % 3 != 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("UNLOAD");
            current_intent.hits = sheet_move("UNLOAD").hits;
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
            current_intent.base_damage = move_damage("HEADBUTT");
            current_intent.description = "Coup de boule (15 dmg)";
        }
    } else if (type == EnemyType::SKULKING_COLONY) {
        // Zoom twice, Inertia (+2 Strength, +4 at Ascension), Piercing Stabs, and again.
        const int step = (turn_number - 1) % 4;
        current_intent.type = EnemyIntentType::ATTACK;
        if (step <= 1) {
            current_intent.base_damage = move_damage("ZOOM");
            current_intent.description = "Bourdonnement (14 dmg)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("INERTIA");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 4 : 2;
            current_intent.description = "Inertie (9 dmg, +2 Force)";
        } else {
            current_intent.base_damage = move_damage("PIERCING_STABS");
            current_intent.hits = sheet_move("PIERCING_STABS").hits;
            current_intent.description = "Coups perforants (7 dmg x2)";
        }
    } else if (type == EnemyType::MAGI_KNIGHT) {
        // Power Shield, Dampen, then Ram, Prep and Magic Bomb in a loop.
        enum Move : uint8_t { POWER_SHIELD, DAMPEN, RAM, PREP, MAGIC_BOMB };
        const Move move = turn_number == 1 ? POWER_SHIELD : turn_number == 2 ? DAMPEN
                          : static_cast<Move>(RAM + (turn_number - 3) % 3);
        if (move == POWER_SHIELD) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("POWER_SHIELD");
            current_intent.block = sheet_move("POWER_SHIELD").block;
            current_intent.description = "Bouclier de puissance (6 dmg, 5 blk)";
        } else if (move == DAMPEN) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.description = "Atténuation";
        } else if (move == RAM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("RAM");
            current_intent.description = "Bélier (10 dmg)";
        } else if (move == PREP) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = sheet_move("PREP").block;
            current_intent.description = "Prep (5 Armure)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("MAGIC_BOMB");
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
            current_intent.base_damage = move_damage("SLASHES");
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
            current_intent.base_damage = move_damage("WHIRLWIND");
            current_intent.hits = sheet_move("WHIRLWIND").hits;
            current_intent.description = "Tourbillon (2 dmg x3)";
        } else if (move == PIERCING_GAZE) {
            current_intent.base_damage = move_damage("PIERCING_GAZE");
            current_intent.description = "Regard perçant (10 dmg)";
        } else {
            current_intent.base_damage = move_damage("JAB");
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
            current_intent.base_damage = move_damage("RIP_AND_TEAR");
            current_intent.description = "Déchirure (14 dmg)";
        } else if (last_move == ROAR) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 3 };
            current_intent.description = "Rugissement (Vulnérable 3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CLAW");
            current_intent.hits = sheet_move("CLAW").hits;
            current_intent.description = "Griffe (4 dmg x2)";
        }
    } else if (type == EnemyType::NIBBIT) {
        // Butt, Slice, Hiss, and again. Where it starts depends on its place at the start of the fight: alone it
        // begins with Butt, in front with Slice, behind with Hiss. `phase` holds that offset.
        if (turn_number == 1) phase = context.allies == 0 ? 0 : (context.slot == 0 ? 1 : 2);
        uint8_t move = static_cast<uint8_t>((phase + phase_turn - 1) % 3);
        if (move == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BUTT");
            current_intent.description = "Derrière (12 dmg)";
        } else if (move == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("SLICE");
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
            current_intent.base_damage = move_damage("WHIP_SLAP");
            current_intent.hits = sheet_move("WHIP_SLAP").hits;
            current_intent.description = "Fouet (3 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("GLOMP");
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
            current_intent.base_damage = move_damage("GIMME");
            current_intent.description = "Donne-moi ça ! (7 dmg x2)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("DOUBLE_SMASH");
            current_intent.debuffs[0] = { StatusType::WEAK, 2 };
            current_intent.description = "Fracas double (6 dmg x2, Affaibli 2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.hits = sheet_move("HEHE").hits;
            current_intent.base_damage = move_damage("HEHE");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Héhé (8 dmg, +2 Force)";
        }
    } else if (type == EnemyType::GLOBE_HEAD) {
        // Shocking Slap (2 Frail), Thunder Strike (3 hits), Galvanic Burst (+2 Strength), and again.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("SHOCKING_SLAP");
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Claque de choc (13 dmg, Fragilisé 2)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("THUNDER_STRIKE");
            current_intent.hits = sheet_move("THUNDER_STRIKE").hits;
            current_intent.description = "Canalisation kéraunique (6 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("GALVANIC_BURST");
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
            current_intent.base_damage = move_damage("WEB_CANNON");
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
            current_intent.base_damage = move_damage("POUNCE");
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
            current_intent.base_damage = move_damage("SWIPE");
            current_intent.description = "Griffe (13 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("STOMP");
            current_intent.hits = sheet_move("STOMP").hits;
            current_intent.description = "Piétinement (4 dmg x3)";
        }
    } else if (type == EnemyType::GUARDBOT) {
        // Guard (15 Block), every turn.
        current_intent.type = EnemyIntentType::DEFEND;
        current_intent.block = sheet_move("GUARD").block;
        current_intent.description = "Garde (15 Armure)";
    } else if (type == EnemyType::BOWLBUG_SILK) {
        // Toxic Spit (1 Weak), Thrash 4x2, and again.
        if ((turn_number - 1) % 2 == 0) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::WEAK, 1 };
            current_intent.description = "Crachat toxique (Affaibli 1)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("THRASH");
            current_intent.hits = sheet_move("THRASH").hits;
            current_intent.description = "Furie (4 dmg x2)";
        }
    } else if (type == EnemyType::BOWLBUG_EGG) {
        // Bite (7 damage, 7 Block) every turn.
        current_intent.type = EnemyIntentType::ATTACK_DEFEND;
        current_intent.base_damage = move_damage("BITE");
        current_intent.block = sheet_move("BITE").block;
        current_intent.description = "Morsure (7 dmg, 7 blk)";
    } else if (type == EnemyType::FABRICATOR) {
        // While there is room for a bot (fewer than 3 enemies alive): Fabricate or Fabricating Strike, 1 in 2, either
        // one again as often as drawn. Without room: Disintegrate. Every bot summoned is one of the four, drawn at random.
        enum Move : uint8_t { FABRICATE, FABRICATING_STRIKE, DISINTEGRATE };
        if (context.allies >= 2) {
            use_move(DISINTEGRATE);
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("DISINTEGRATE");
            current_intent.description = "Désintégration (11 dmg)";
        } else {
            static constexpr MoveOption OPTIONS[] = { { FABRICATE, 1, true }, { FABRICATING_STRIKE, 1, true } };
            static constexpr MoveOption BOTS[] = { { 0, 1, true }, { 1, 1, true }, { 2, 1, true }, { 3, 1, true } };
            static constexpr EnemyType BOT_TYPES[] = { EnemyType::STABBOT, EnemyType::NOISEBOT, EnemyType::ZAPBOT,
                                                       EnemyType::GUARDBOT };
            const uint8_t move = draw_move(OPTIONS);
            const uint8_t played = last_move;
            current_intent.summon_type = BOT_TYPES[draw_move(BOTS)];
            last_move = played;
            current_intent.summons = 1;
            current_intent.summon_minions = true;
            if (move == FABRICATING_STRIKE) {
                current_intent.type = EnemyIntentType::ATTACK;
                current_intent.base_damage = move_damage("FABRICATING_STRIKE");
                current_intent.description = "Frappe du Machiniste (18 dmg, invoque)";
            } else {
                current_intent.type = EnemyIntentType::SUMMON;
                current_intent.description = "Fabrication";
            }
        }
    } else if (type == EnemyType::OWL_MAGISTRATE) {
        // Magistrate's Scrutiny 16, Peck Assault 4x6, Judicial Flight (it Soars: half the attack damage until it lands,
        // when its next turn starts), Verdict 33 with 4 Vulnerable, and again.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("MAGISTRATE_SCRUTINY");
            current_intent.description = "Regard du magistrat (16 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("PECK_ASSAULT");
            current_intent.hits = sheet_move("PECK_ASSAULT").hits;
            current_intent.description = "Assaut de becs (4 dmg x6)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.power = PowerId::SOAR;
            current_intent.power_amount = 1;
            current_intent.description = "Envol judiciaire";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("VERDICT");
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 4 };
            current_intent.description = "Verdict (33 dmg, Vulnérable 4)";
        }
    } else if (type == EnemyType::SCROLL_OF_BITING) {
        // Chomp 14, More Teeth (Strength 2), Chew 5x2, then a draw between Chomp (never twice in a row) and Chew.
        enum Move : uint8_t { CHOMP, CHEW, MORE_TEETH };
        if (turn_number <= 1) {
            use_move(CHOMP);
        } else if (turn_number == 2) {
            use_move(MORE_TEETH);
        } else if (turn_number == 3) {
            use_move(CHEW);
        } else {
            static constexpr MoveOption OPTIONS[] = { { CHOMP }, { CHEW, 1, true } };
            draw_move(OPTIONS);
        }
        if (last_move == CHOMP) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CHOMP");
            current_intent.description = "Croc (14 dmg)";
        } else if (last_move == CHEW) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CHEW");
            current_intent.hits = sheet_move("CHEW").hits;
            current_intent.description = "Mâchouille (5 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Plus de dents (+2 Force)";
        }
    } else if (type == EnemyType::FOGMOG) {
        // Illusion Spores (an Eye With Teeth joins the fight), Swipe (+1 Strength), then a draw between Swipe (+1
        // Strength) and Headbutt, neither twice in a row.
        enum Move : uint8_t { ILLUSION, SWIPE, SWIPE_RANDOM, HEADBUTT };
        if (turn_number <= 1) {
            use_move(ILLUSION);
        } else if (turn_number == 2) {
            use_move(SWIPE);
        } else {
            static constexpr MoveOption OPTIONS[] = { { SWIPE_RANDOM, 4 }, { HEADBUTT, 6 } };
            draw_move(OPTIONS);
        }
        if (last_move == ILLUSION) {
            current_intent.type = EnemyIntentType::SUMMON;
            current_intent.summon_type = EnemyType::EYE_WITH_TEETH;
            current_intent.summons = 1;
            current_intent.summon_minions = true;
            current_intent.description = "Spores illusoires";
        } else if (last_move == HEADBUTT) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("HEADBUTT");
            current_intent.description = "Headbutt (14 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("SWIPE");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 1;
            current_intent.description = "Mandale (8 dmg, +1 Force)";
        }
    } else if (type == EnemyType::SNAPPING_JAXFRUIT) {
        // Energy Orb every turn: 3 damage and +2 Strength.
        current_intent.type = EnemyIntentType::ATTACK_BUFF;
        current_intent.base_damage = move_damage("ENERGY_ORB");
        current_intent.status_type = StatusType::STRENGTH;
        current_intent.status_amount = 2;
        current_intent.description = "Orbe d'énergie (3 dmg, +2 Force)";
    } else if (type == EnemyType::TWIG_SLIME_S) {
        // Tackle every turn.
        current_intent.type = EnemyIntentType::ATTACK;
        current_intent.base_damage = move_damage("TACKLE");
        current_intent.description = "Charge (4 dmg)";
    } else if (type == EnemyType::TWIG_SLIME_M) {
        // Sticky Shot (1 Slimed), then a draw between Pokey Pounce (as often as drawn) and Sticky Shot (never twice).
        enum Move : uint8_t { STICKY_SHOT, POKEY_POUNCE };
        if (turn_number <= 1) {
            use_move(STICKY_SHOT);
        } else {
            static constexpr MoveOption OPTIONS[] = { { POKEY_POUNCE, 1, true }, { STICKY_SHOT } };
            draw_move(OPTIONS);
        }
        if (last_move == STICKY_SHOT) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::SLIMED;
            current_intent.status_cards = 1;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Tir collant (1 Englué)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("POKEY_POUNCE");
            current_intent.description = "Petit saut (11 dmg)";
        }
    } else if (type == EnemyType::LEAF_SLIME_S) {
        // A draw between Tackle and Goop (1 Slimed), never the same twice in a row.
        enum Move : uint8_t { TACKLE, GOOP };
        static constexpr MoveOption OPTIONS[] = { { TACKLE }, { GOOP } };
        draw_move(OPTIONS);
        if (last_move == GOOP) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::SLIMED;
            current_intent.status_cards = 1;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Gelée (1 Englué)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("TACKLE");
            current_intent.description = "Charge (3 dmg)";
        }
    } else if (type == EnemyType::LEAF_SLIME_M) {
        // Sticky Shot (2 Slimed), Clump Shot 8, and again.
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::SLIMED;
            current_intent.status_cards = 2;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Tir adhésif (2 Englué)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CLUMP_SHOT");
            current_intent.description = "Tir groupé (8 dmg)";
        }
    } else if (type == EnemyType::SLITHERING_STRANGLER) {
        // Constrict (the player takes 3 damage at the end of each of their turns while it lives), then a draw
        // between Thwack (7 dmg, 5 Block) and Lash 12, as often as drawn.
        enum Move : uint8_t { CONSTRICT, THWACK, LASH };
        if (turn_number <= 1) {
            use_move(CONSTRICT);
        } else {
            static constexpr MoveOption OPTIONS[] = { { THWACK, 1, true }, { LASH, 1, true } };
            draw_move(OPTIONS);
        }
        if (last_move == CONSTRICT) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power = PowerId::CONSTRICT;
            current_intent.power_amount = 3;
            current_intent.power_on_player = true;
            current_intent.description = "Étranglement (Étreinte 3)";
        } else if (last_move == THWACK) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("THWACK");
            current_intent.block = sheet_move("THWACK").block;
            current_intent.description = "Frappe (7 dmg, 5 blk)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("LASH");
            current_intent.description = "Fouet (12 dmg)";
        }
    } else if (type == EnemyType::LIVING_FOG) {
        // Advanced Gas (Smoggy: one Skill a turn), then Bloat (a Gas Bomb joins the fight) and Super Gas Blast in turn.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("ADVANCED_GAS");
            current_intent.power = PowerId::SMOGGY;
            current_intent.power_amount = 1;
            current_intent.power_on_player = true;
            current_intent.description = "Gaz nocif (8 dmg, Enfumé)";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BLOAT");
            current_intent.summon_type = EnemyType::GAS_BOMB;
            current_intent.summons = 1;
            current_intent.summon_minions = true;
            current_intent.description = "Boursouflure (5 dmg, invoque)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SUPER_GAS_BLAST");
            current_intent.description = "Super rafale gazeuse (8 dmg)";
        }
    } else if (type == EnemyType::TWO_TAILED_RAT) {
        // A draw among Scratch 8, Disease Bite 6, Screech (1 Frail) and Call for Backup (a Two-Tailed Rat joins the
        // fight, once), never the same twice in a row.
        enum Move : uint8_t { SCRATCH, DISEASE_BITE, SCREECH, CALL_FOR_BACKUP };
        static constexpr MoveOption ANY[] = { { SCRATCH }, { DISEASE_BITE }, { SCREECH }, { CALL_FOR_BACKUP } };
        static constexpr MoveOption NO_CALL[] = { { SCRATCH }, { DISEASE_BITE }, { SCREECH } };
        if (phase == 0) {
            if (draw_move(ANY) == CALL_FOR_BACKUP) enter_phase(1);
        } else {
            draw_move(NO_CALL);
        }
        if (last_move == SCRATCH) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SCRATCH");
            current_intent.description = "Griffure (8 dmg)";
        } else if (last_move == DISEASE_BITE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("DISEASE_BITE");
            current_intent.description = "Morsure infectée (6 dmg)";
        } else if (last_move == SCREECH) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Cri (Fragilisé 1)";
        } else {
            current_intent.type = EnemyIntentType::SUMMON;
            current_intent.summon_type = EnemyType::TWO_TAILED_RAT;
            current_intent.summons = 1;
            current_intent.summon_minions = false;
            current_intent.description = "Appel à l'aide";
        }
    } else if (type == EnemyType::SHRINKER_BEETLE) {
        // Shrinker (the player is Shrunk for 3 turns) on the first turn, then Chomp 7 and Stomp 13 in turn.
        if (turn_number == 1) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power = PowerId::SHRINK;
            current_intent.power_amount = SHRINK_TURNS;
            current_intent.power_on_player = true;
            current_intent.description = "Rétrécissement";
        } else if (turn_number % 2 == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("CHOMP");
            current_intent.description = "Croc (7 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("STOMP");
            current_intent.description = "Piétinement (13 dmg)";
        }
    } else if (type == EnemyType::TOUGH_EGG) {
        // Hatch on its first turn (it does nothing else), then Nibble 4 every turn.
        if (phase_turn == 1) {
            current_intent.type = EnemyIntentType::SUMMON;
            current_intent.description = "Éclosion";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("NIBBLE");
            current_intent.description = "Grignotage (4 dmg)";
        }
    } else if (type == EnemyType::THE_OBSCURA) {
        // Illusion (summons a Parafright, Minion of the Obscura) on the first turn, then Hardening Strike (triggered),
        // and a draw among Piercing Gaze, Sail and Hardening Strike that cannot repeat the move just played.
        enum Move : uint8_t { ILLUSION, PIERCING_GAZE, SAIL, HARDENING_STRIKE };
        if (turn_number == 1) {
            use_move(ILLUSION);
        } else if (enter_phase(1)) {
            phase_turn = 1;
            use_move(HARDENING_STRIKE);
        } else {
            static constexpr MoveOption OPTIONS[] = { { PIERCING_GAZE }, { SAIL }, { HARDENING_STRIKE } };
            draw_move(OPTIONS);
        }
        if (last_move == ILLUSION) {
            current_intent.type = EnemyIntentType::SUMMON;
            current_intent.summon_type = EnemyType::PARAFRIGHT;
            current_intent.summons = 1;
            current_intent.summon_minions = true;
            current_intent.description = "Illusion";
        } else if (last_move == PIERCING_GAZE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("PIERCING_GAZE");
            current_intent.description = "Regard perçant (10 dmg)";
        } else if (last_move == SAIL) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.description = "Voile";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("HARDENING_STRIKE");
            current_intent.block = sheet_move("HARDENING_STRIKE").block;
            current_intent.description = "Frappe durcissante (6 dmg, 6 blk)";
        }
    } else if (type == EnemyType::OVICOPTER) {
        // Cycles of three moves: Lay Eggs (a Tough Egg, a Minion) or, when there is no room for one, Nutritional
        // Paste (+3 Strength), then Smash 16, then Tenderizer 7 with 2 Vulnerable.
        const int step = (turn_number - 1) % 3;
        if (step == 0) phase = (turn_number == 1 || context.allies < 2) ? 0 : 1;
        if (step == 0 && phase == 0) {
            current_intent.type = EnemyIntentType::SUMMON;
            current_intent.summon_type = EnemyType::TOUGH_EGG;
            current_intent.summons = 1;
            current_intent.summon_minions = true;
            current_intent.description = "Ponte (Œuf robuste)";
        } else if (step == 0) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 4 : 3;
            current_intent.description = "Pâte nutritive (+3 Force)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SMASH");
            current_intent.description = "Fracas (16 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("TENDERIZER");
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 2 };
            current_intent.description = "Adoucir (7 dmg, Vulnérable 2)";
        }
    } else if (type == EnemyType::BYRDONIS) {
        // Swoop (17 damage) then Peck (3 damage x3), and again.
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SWOOP");
            current_intent.description = "Piqué (17 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("PECK");
            current_intent.hits = sheet_move("PECK").hits;
            current_intent.description = "Coup de bec (3 dmg x3)";
        }
    } else if (type == EnemyType::THE_INSATIABLE) {
        // Liquify Ground once (Sandpit and Frantic Escapes), then Thrash, Lunging Bite, Salivate, Thrash, and again.
        const int step = turn_number == 1 ? -1 : (turn_number - 2) % 4;
        if (step < 0) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.power = PowerId::SANDPIT;
            current_intent.power_amount = INSATIABLE_SANDPIT;
            current_intent.status_card = CardId::FRANTIC_ESCAPE;
            current_intent.status_cards = INSATIABLE_FRANTIC_ESCAPES;
            current_intent.status_pile = Pile::DRAW;
            current_intent.description = "Liquéfaction du sol (Sables mouvants)";
        } else if (step == 0 || step == 3) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("THRASH");
            current_intent.hits = sheet_move("THRASH").hits;
            current_intent.description = "Furie (8 dmg x2)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("LUNGING_BITE");
            current_intent.description = "Morsure brusque (28 dmg)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 3 : 2;
            current_intent.description = "Salivation (+2 Force)";
        }
    } else if (type == EnemyType::WRIGGLER) {
        // Nasty Bite (6 damage), then Wriggle (+2 Strength and an Infection into the discard pile), and again.
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("NASTY_BITE");
            current_intent.description = "Morsure vicieuse (6 dmg)";
        } else {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.status_card = CardId::INFECTION;
            current_intent.status_cards = 1;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Tortillement (+2 Force, Infection)";
        }
    } else if (type == EnemyType::PHROG_PARASITE) {
        // Infect (3 Infections into the discard pile), then Lash (4 damage x4), and again.
        if (turn_number % 2 == 1) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::INFECTION;
            current_intent.status_cards = 3;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Infection (3 Infection)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("LASH");
            current_intent.hits = sheet_move("LASH").hits;
            current_intent.description = "Fouet (4 dmg x4)";
        }
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
            current_intent.base_damage = move_damage("SPIKE_SPIT");
            current_intent.hits = sheet_move("SPIKE_SPIT").hits;
            current_intent.status_type = StatusType::THORNS;
            current_intent.status_amount = 2;
            current_intent.description = "Jet d'épines (3x3 dmg, +2 Épines)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("WHIRL");
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
            current_intent.base_damage = move_damage("CRASH");
            current_intent.description = "Fracas (16 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("THRASH");
            current_intent.hits = sheet_move("THRASH").hits;
            current_intent.status_type = StatusType::VIGOR;
            current_intent.status_amount = 6;
            current_intent.description = "Furie (3x3 dmg, +6 Vigueur)";
        }
    } else if (type == EnemyType::SEAPUNK) {
        // Sea Kick, Spinning Kick, Bubble Burp, and again.
        if (turn_number % 3 == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SEA_KICK");
            current_intent.description = "Coup de pied marin (11 dmg)";
        } else if (turn_number % 3 == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SPINNING_KICK");
            current_intent.hits = sheet_move("SPINNING_KICK").hits;
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
            current_intent.base_damage = move_damage("CLAMP");
            current_intent.hits = sheet_move("CLAMP").hits;
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
            current_intent.base_damage = move_damage("SAVAGE");
            current_intent.description = "Sauvagerie (12 dmg)";
        }
    } else if (type == EnemyType::INFESTED_PRISM) {
        // Jab, Radiate, Whirlwind, Pulsate, and again.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("JAB");
            current_intent.description = "Coup sec (15 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("RADIATE");
            current_intent.block = ascended_damage ? 13 : 11;
            current_intent.description = "Irradiation (11 dmg, 11 blk)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("WHIRLWIND");
            current_intent.hits = sheet_move("WHIRLWIND").hits;
            current_intent.description = "Tourbillon (5 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("PULSATE");
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
            current_intent.base_damage = move_damage("FLAIL");
            current_intent.hits = sheet_move("FLAIL").hits;
            current_intent.description = "Fléau (9 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("RAM");
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
            current_intent.base_damage = move_damage("FRAIL_SPORES");
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Spores fragilisantes (8 dmg, Fragilisé 2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SMASH");
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
            current_intent.base_damage = move_damage("TONGUE_LASH");
            current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
            current_intent.description = "Fouet de langue (13 dmg, Fragilisé 2)";
        } else if (last_move == STRIKE_DOWN_EVIL) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("STRIKE_DOWN_EVIL");
            current_intent.description = "Expier le mal (21 dmg)";
        } else if (last_move == FOR_THE_QUEEN) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 5;
            current_intent.description = "Pour la Reine (Force 5)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BEETLE_CHARGE");
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
            current_intent.base_damage = move_damage("OIL_SPRAY");
            current_intent.debuffs[0] = { StatusType::WEAK, 1 };
            current_intent.description = "Spray huileux (8 dmg, Affaibli 1)";
        } else if (last_move == SLAM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SLAM");
            current_intent.description = "Heurt (11 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("RAGE");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.description = "Rage (6 dmg, Force 3)";
        }
    } else if (type == EnemyType::ENTOMANCER) {
        // Bees, Spear, Pheromone Spit (Personal Hive 1, Strength 2 then 1), and again.
        const int phase = (turn_number - 1) % 3;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BEES");
            current_intent.hits = ascended_damage ? 8 : 7;
            current_intent.description = "Bzzzzzz !! (3 dmg x7)";
        } else if (phase == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SPEAR");
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
            current_intent.base_damage = move_damage("SOUL_SLASH");
            current_intent.description = "Frappe spirituelle (15 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SOUL_FLAME");
            current_intent.hits = sheet_move("SOUL_FLAME").hits;
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
            current_intent.base_damage = move_damage("CHARGE");
            current_intent.description = "Charge (25 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("FLAMETHROWER");
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
            current_intent.base_damage = move_damage("HEAVY_CLEAVE");
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
            current_intent.base_damage = move_damage("BITE");
            current_intent.description = "Morsure (17 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("PUNCTURE");
            current_intent.hits = sheet_move("PUNCTURE").hits;
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
            current_intent.base_damage = move_damage("TACKLE");
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Charge (9 dmg, Fragilisé 1)";
        } else if (last_move == LATCH) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("LATCH");
            current_intent.description = "Accrochage (12 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("LASH");
            current_intent.hits = sheet_move("LASH").hits;
            current_intent.description = "Fouet (3x2 dmg)";
        }
    } else if (type == EnemyType::SOUL_FYSH) {
        // Beckon, De-Gas, Gaze, Fade and Scream, in a loop.
        const int step = (turn_number - 1) % 5;
        if (step == 0) {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_card = CardId::BECKON;
            current_intent.status_cards = SOUL_FYSH_BECKONS;
            current_intent.status_pile = Pile::DRAW;
            current_intent.description = "Appel (2 Appel)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("DE_GAS");
            current_intent.description = "Dégazage (16 dmg)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("GAZE");
            current_intent.status_card = CardId::BECKON;
            current_intent.status_cards = SOUL_FYSH_GAZE_BECKONS;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Fixation (7 dmg, 1 Appel)";
        } else if (step == 3) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::INTANGIBLE;
            current_intent.status_amount = SOUL_FYSH_INTANGIBLE;
            current_intent.description = "Camouflage (Intangible 2)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("SCREAM");
            current_intent.debuffs[0] = { StatusType::VULNERABLE, SOUL_FYSH_SCREAM_VULNERABLE };
            current_intent.description = "Hurlement (13 dmg, Vuln 3)";
        }
    } else if (type == EnemyType::KNOWLEDGE_DEMON) {
        // Curse of Knowledge, Slap, Knowledge Overwhelming, Ponder, three times: each Curse of Knowledge puts its own
        // curse on the player. Then (phase 1) only Slap, Knowledge Overwhelming and Ponder, in a loop.
        constexpr int CURSE_CYCLE = 4;
        if (phase == 0 && phase_turn > KNOWLEDGE_DEMON_CURSE_TIMES * CURSE_CYCLE && enter_phase(1)) phase_turn = 1;
        const int step = phase == 0 ? (phase_turn - 1) % CURSE_CYCLE : (phase_turn - 1) % (CURSE_CYCLE - 1) + 1;
        if (step == 0) {
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.power_on_player = true;
            const int curse = (phase_turn - 1) / CURSE_CYCLE;
            if (curse == 0) {
                current_intent.power = PowerId::MIND_ROT;
                current_intent.power_amount = KNOWLEDGE_DEMON_MIND_ROT;
                current_intent.description = "Malédiction du savoir (Esprit pourri)";
            } else if (curse == 1) {
                current_intent.power = PowerId::WASTE_AWAY;
                current_intent.power_amount = KNOWLEDGE_DEMON_WASTE_AWAY;
                current_intent.description = "Malédiction du savoir (Dépérissement)";
            } else {
                current_intent.power = PowerId::DISINTEGRATION;
                current_intent.power_amount = KNOWLEDGE_DEMON_DISINTEGRATION;
                current_intent.description = "Malédiction du savoir (Désintégration)";
            }
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SLAP");
            current_intent.description = "Claque (17 dmg)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("KNOWLEDGE_OVERWHELMING");
            current_intent.hits = sheet_move("KNOWLEDGE_OVERWHELMING").hits;
            current_intent.description = "Savoir accablant (8 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_BUFF;
            current_intent.base_damage = move_damage("PONDER");
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? KNOWLEDGE_DEMON_PONDER_STRENGTH_ASC : KNOWLEDGE_DEMON_PONDER_STRENGTH;
            current_intent.heal = KNOWLEDGE_DEMON_PONDER_HEAL;
            current_intent.description = "Contemplation (11 dmg, +2 Force, soin 30)";
        }
    } else if (type == EnemyType::TEST_SUBJECT) {
        // First life: Bite, Skull Bash, in a loop. Each new life (respawn) opens on Respawn, which gives it Painful
        // Stabs, then Nemesis; then Multi-Claw every turn in its second life, and Lacerate, Big Pounce, Burning
        // Growl in a loop in its third.
        uint8_t move = SUBJECT_RESPAWN;
        if (phase == 0) move = phase_turn % 2 == 1 ? SUBJECT_BITE : SUBJECT_SKULL_BASH;
        else if (phase_turn > 1 && phase == 1) move = SUBJECT_MULTI_CLAW;
        else if (phase_turn > 1) move = static_cast<uint8_t>(SUBJECT_LACERATE + (phase_turn - 2) % 3);
        use_move(move);
        if (move == SUBJECT_RESPAWN) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.power = phase == 1 ? PowerId::PAINFUL_STABS : PowerId::NEMESIS;
            current_intent.power_amount = 1;
            // Two assignments, not a ternary: a string_view built from a char pointer calls strlen, which the GBA lacks.
            if (phase == 1) {
                current_intent.description = "Réanimation (Coups de poignard)";
            } else {
                current_intent.description = "Réanimation (Némésis)";
            }
        } else if (move == SUBJECT_BITE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BITE");
            current_intent.description = "Morsure (20 dmg)";
        } else if (move == SUBJECT_SKULL_BASH) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("SKULL_BASH");
            current_intent.debuffs[0] = { StatusType::VULNERABLE, 1 };
            current_intent.description = "Coup de boule (14 dmg, Vulnérable)";
        } else if (move == SUBJECT_MULTI_CLAW) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("MULTI_CLAW");
            current_intent.hits = sheet_move("MULTI_CLAW").hits;
            current_intent.description = "Multi-Griffes (10 dmg)";
        } else if (move == SUBJECT_LACERATE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("PHASE3_LACERATE");
            current_intent.hits = sheet_move("PHASE3_LACERATE").hits;
            current_intent.description = "Lacération (10 dmg x3)";
        } else if (move == SUBJECT_POUNCE) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BIG_POUNCE");
            current_intent.description = "Ruée bestiale (45 dmg)";
        } else {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? 3 : 2;
            current_intent.status_card = CardId::BURN;
            current_intent.status_cards = ascended_damage ? 5 : 3;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Grognement ardent (+2 Force, 3 Brûlure)";
        }
    } else if (type == EnemyType::AEONGLASS) {
        // Ebb (attack and Block), Eye Lasers, Increasing Intensity (Strength, Withers into the discard pile), in a loop.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("EBB");
            current_intent.block = sheet_move("EBB").block;
            current_intent.description = "Régression (22 dmg, 33 Armure)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("EYE_LASERS");
            current_intent.hits = sheet_move("EYE_LASERS").hits;
            current_intent.description = "Rayons oculaires (11 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::STATUS;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_card = CardId::WITHER;
            current_intent.status_pile = Pile::DISCARD;
            // Two assignments, not a ternary: a string_view built from a char pointer calls strlen, which the GBA lacks.
            if (ascended_damage) {
                current_intent.status_amount = AEONGLASS_STRENGTH_ASC;
                current_intent.status_cards = 2;
                current_intent.description = "Intensité croissante (+3 Force, 2 Érosion)";
            } else {
                current_intent.status_amount = AEONGLASS_STRENGTH;
                current_intent.status_cards = 1;
                current_intent.description = "Intensité croissante (+2 Force, 1 Érosion)";
            }
        }
    } else if (type == EnemyType::LAGAVULIN_MATRIARCH) {
        // Asleep (a lost turn) behind its Plating, for 3 turns or until it loses HP. Awake, it drops the Plating and
        // plays Slash, Disembowel, Slash (with Block) and Soul Siphon in a loop.
        if (phase == 0 && (hp < max_hp || phase_turn > LAGAVULIN_SLEEP_TURNS) && enter_phase(1)) {
            phase_turn = 1;
            clear_status(StatusType::PLATED_ARMOR);
        }
        const int step = (phase_turn - 1) % 4;
        if (phase == 0) {
            current_intent.type = EnemyIntentType::STUN;
            current_intent.description = "Sommeil";
        } else if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SLASH");
            current_intent.description = "Trancher (19 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("DISEMBOWEL");
            current_intent.hits = sheet_move("DISEMBOWEL").hits;
            current_intent.description = "Éventration (9 dmg x2)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEFEND;
            current_intent.base_damage = move_damage("SLASH2");
            current_intent.block = ascended_damage ? 14 : sheet_move("SLASH2").block;
            current_intent.description = "Trancher (12 dmg, 12 blk)";
        } else {
            // Takes 2 Strength and 2 Dexterity from the player, and gains 2 Strength.
            current_intent.type = EnemyIntentType::DEBUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.debuffs[0] = { StatusType::STRENGTH, -2 };
            current_intent.debuffs[1] = { StatusType::DEXTERITY, -2 };
            current_intent.description = "Siphon d'âme (+2 Force, -2 For/Dex)";
        }
    } else if (type == EnemyType::WATERFALL_GIANT) {
        // Pressurize (Steam Eruption), then Stomp, Ram, Siphon, Pressure Gun, Pressure Up in a loop, each with 3 Steam
        // Eruption. Killed (triggered, see about_to_blow): About to Blow (a lost turn), then its Explosion.
        const int16_t steam = WATERFALL_GIANT_STEAM;
        const int step = phase_turn < 2 ? -1 : (phase_turn - 2) % 5;
        if (phase == 1) {
            if (phase_turn == 1) {
                current_intent.type = EnemyIntentType::STUN;
                current_intent.description = "Surchauffe";
            } else {
                current_intent.type = EnemyIntentType::ATTACK;
                current_intent.base_damage = explosion;
                current_intent.description = "Explosion";
            }
        } else if (step < 0) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.power = PowerId::STEAM_ERUPTION;
            current_intent.power_amount = ascended_damage ? WATERFALL_GIANT_PRESSURIZE_ASC : WATERFALL_GIANT_PRESSURIZE;
            current_intent.description = "Pressurisation (Éruption de vapeur)";
        } else {
            current_intent.power = PowerId::STEAM_ERUPTION;
            current_intent.power_amount = steam;
            if (step == 0) {
                current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
                current_intent.base_damage = move_damage("STOMP");
                current_intent.debuffs[0] = { StatusType::WEAK, 1 };
                current_intent.description = "Piétinement (15 dmg, Faible)";
            } else if (step == 1) {
                current_intent.type = EnemyIntentType::ATTACK_BUFF;
                current_intent.base_damage = move_damage("RAM");
                current_intent.description = "Bélier (10 dmg)";
            } else if (step == 2) {
                current_intent.type = EnemyIntentType::HEAL;
                current_intent.heal = WATERFALL_GIANT_SIPHON_HEAL;
                current_intent.description = "Siphon (soin 15)";
            } else if (step == 3) {
                current_intent.type = EnemyIntentType::ATTACK;
                current_intent.base_damage = ascended_damage ? WATERFALL_GIANT_PRESSURE_GUN_ASC : WATERFALL_GIANT_PRESSURE_GUN;
                current_intent.description = "Pistolet à pression (20 dmg)";
            } else {
                current_intent.type = EnemyIntentType::ATTACK_BUFF;
                current_intent.base_damage = move_damage("PRESSURE_UP");
                current_intent.description = "Montée en pression (13 dmg)";
            }
        }
    } else if (type == EnemyType::KIN_FOLLOWER) {
        // Quick Slash 5, Boomerang 2x2, Power Dance (+2 Strength, 3 at Ascension), in a loop.
        const int step = (turn_number - 1) % 3;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("QUICK_SLASH");
            current_intent.description = "Coup vif (5 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BOOMERANG");
            current_intent.hits = sheet_move("BOOMERANG").hits;
            current_intent.description = "Boomerang (2 dmg x2)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? KIN_POWER_DANCE_ASC : KIN_POWER_DANCE;
            if (ascended_damage) {
                current_intent.description = "Transe Martiale (+3 Force)";
            } else {
                current_intent.description = "Transe Martiale (+2 Force)";
            }
        }
    } else if (type == EnemyType::KIN_PRIEST) {
        // Orb of Frailty 8 (Frail 1), Orb of Weakness 8 (Weak 1), Beam 3x3, Ritual (+2 Strength), in a loop.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("ORB_OF_FRAILTY");
            current_intent.debuffs[0] = { StatusType::FRAIL, 1 };
            current_intent.description = "Orbe de fragilité (8 dmg, Fragilisé)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("ORB_OF_WEAKNESS");
            current_intent.debuffs[0] = { StatusType::WEAK, 1 };
            current_intent.description = "Orbe de faiblesse (8 dmg, Affaibli)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BEAM");
            current_intent.hits = sheet_move("BEAM").hits;
            current_intent.description = "Rayon animique (3 dmg x3)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = ascended_damage ? KIN_RITUAL_ASC : KIN_RITUAL;
            if (ascended_damage) {
                current_intent.description = "Sombre rituel (+3 Force)";
            } else {
                current_intent.description = "Sombre rituel (+2 Force)";
            }
        }
    } else if (type == EnemyType::VANTOM) {
        // Ink Blot, Inky Lance, Dismember (3 Wounds), Prepare (+2 Strength), in a loop.
        const int step = (turn_number - 1) % 4;
        if (step == 0) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("INK_BLOT");
            current_intent.description = "Tache d'encre (7 dmg)";
        } else if (step == 1) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("INKY_LANCE");
            current_intent.hits = sheet_move("INKY_LANCE").hits;
            current_intent.description = "Javelot d'encre (6 dmg x2)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("DISMEMBER");
            current_intent.status_card = CardId::WOUND;
            current_intent.status_cards = 3;
            current_intent.status_pile = Pile::DISCARD;
            current_intent.description = "Démembrement (26 dmg, 3 Plaie)";
        } else {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 2;
            current_intent.description = "Préparation (+2 Force)";
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
            current_intent.base_damage = move_damage("ROLL_OUT");
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
            current_intent.base_damage = move_damage("BITE");
            current_intent.description = "Morsure (13 dmg)";
        } else if (last_move == TUNNELER_BURROW) {
            current_intent.type = EnemyIntentType::DEFEND;
            current_intent.block = sheet_move("BURROW").block;
            current_intent.power = PowerId::BURROWED;
            current_intent.power_amount = 1;
            current_intent.description = "Creuser (32 blk, Enfoui)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("BELOW");
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
            current_intent.base_damage = move_damage("FURIOUS_PUMMELING");
            current_intent.hits = sheet_move("FURIOUS_PUMMELING").hits;
            current_intent.description = "Ruée de coups (4 dmg x4)";
        } else if (step == 2) {
            current_intent.type = EnemyIntentType::BUFF;
            current_intent.status_type = StatusType::STRENGTH;
            current_intent.status_amount = 3;
            current_intent.debuffs[0] = { StatusType::WEAK, 3 };
            current_intent.description = "Câlin sangsue (Force 3, Affaibli 3)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("SMOTHER");
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
            current_intent.base_damage = move_damage("SOUL_BURN");
            current_intent.description = "Brûlure d'âme (29 dmg)";
        } else if (last_move == MAELSTROM) {
            current_intent.type = EnemyIntentType::ATTACK;
            current_intent.base_damage = move_damage("MAELSTROM");
            current_intent.hits = sheet_move("MAELSTROM").hits;
            current_intent.description = "Maelstrom (6x4 dmg)";
        } else {
            current_intent.type = EnemyIntentType::ATTACK_DEBUFF;
            current_intent.base_damage = move_damage("DRAIN_LIFE");
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
    wounding_hits = 0;

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
            const int16_t hp_before = player.hp;
            player.take_damage_with_minion(intent_damage(player, halved));
            if (player.hp < hp_before) ++wounding_hits;
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
