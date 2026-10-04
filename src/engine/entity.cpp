#include "engine/entity.h"
#include <span>
#include "engine/card.h"

namespace engine {

namespace {

using namespace std::string_view_literals;

// ---------------------------------------------------------------------------------------------------------
// Move tables. One small description per monster, static constexpr so that it lives in the cartridge's ROM.
// Source: bible/data/monster_v0.111.0.json ("moves" and "flow" of each monster). HP is the low end of the
// reference range, as for the monsters written by hand below.
// ---------------------------------------------------------------------------------------------------------

// What one move does. The damage is dealt `hits` times; the statuses and the cards come after the attack.
struct EnemyMove {
    std::string_view name;
    EnemyIntentType type = EnemyIntentType::ATTACK;
    uint8_t damage = 0;
    uint8_t hits = 1;
    uint8_t block = 0;
    StatusType self_status = StatusType::NONE;
    int8_t self_amount = 0;
    StatusType player_status = StatusType::NONE;
    int8_t player_amount = 0;
    CardId card = CardId::NONE; // Status card added to the player's discard pile, `card_count` times
    uint8_t card_count = 0;
    uint8_t heal = 0;
};

// How often a move drawn at random can come back.
enum class Repeat : uint8_t {
    FREE,      // any time
    NO_REPEAT, // not twice in a row
    ONCE       // once per fight
};

struct MoveOption {
    uint8_t move = 0;   // index in the monster's moves
    uint8_t weight = 0; // share of the draw; 0 for an unused slot
    Repeat repeat = Repeat::FREE;
};

// One step of a pattern: a move, or a draw among several, then the step that follows. A sequence is steps
// that follow each other, a loop is a step whose `next` goes back.
struct EnemyStep {
    uint8_t next = 0;
    MoveOption options[4];
};

constexpr EnemyStep play(uint8_t move, uint8_t next) {
    return { next, { { move, 1, Repeat::FREE } } };
}

constexpr EnemyStep draw(uint8_t next, MoveOption a, MoveOption b, MoveOption c = {}, MoveOption d = {}) {
    return { next, { a, b, c, d } };
}

struct MonsterDef {
    EnemyType type;
    std::string_view name; // shown to the player, in French
    int16_t hp;
    std::span<const EnemyMove> moves;
    std::span<const EnemyStep> steps;
    // First step of the first monster of a group, and of the ones behind it; a monster alone starts at step 0.
    uint8_t front_start = 0;
    uint8_t back_start = 0;
    // Second phase: the first plan made at or under `phase_hp_percent` % of its Max HP goes to `phase_step`,
    // once per fight. 0: no second phase.
    uint8_t phase_hp_percent = 0;
    uint8_t phase_step = 0;
};

constexpr EnemyIntentType ATTACK = EnemyIntentType::ATTACK;
constexpr EnemyIntentType BUFF = EnemyIntentType::BUFF;
constexpr EnemyIntentType DEFEND = EnemyIntentType::DEFEND;
constexpr EnemyIntentType ATTACK_DEFEND = EnemyIntentType::ATTACK_DEFEND;
constexpr EnemyIntentType ATTACK_BUFF = EnemyIntentType::ATTACK_BUFF;
constexpr EnemyIntentType ATTACK_DEBUFF = EnemyIntentType::ATTACK_DEBUFF;
constexpr EnemyIntentType DEBUFF = EnemyIntentType::DEBUFF;
constexpr EnemyIntentType STATUS = EnemyIntentType::STATUS;
constexpr EnemyIntentType STUN = EnemyIntentType::STUN;

// Charge Up, then Repeater Blast twice and Expel, and again from the first Repeater Blast.
constexpr EnemyMove CUBEX_CONSTRUCT_MOVES[] = {
    { .name = "Chargement"sv, .type = BUFF, .self_status = StatusType::STRENGTH, .self_amount = 2 },
    { .name = "Tir répéteur"sv, .type = ATTACK_BUFF, .damage = 7, .self_status = StatusType::STRENGTH,
      .self_amount = 2 },
    { .name = "Éjection"sv, .type = ATTACK, .damage = 5, .hits = 2 },
};
constexpr EnemyStep CUBEX_CONSTRUCT_STEPS[] = { play(0, 1), play(1, 2), play(1, 3), play(2, 1) };

// Jab, then Piercing Gaze or Whirlwind, never the same twice in a row.
constexpr EnemyMove INKLET_MOVES[] = {
    { .name = "Coup sec"sv, .type = ATTACK, .damage = 3 },
    { .name = "Tourbillon"sv, .type = ATTACK, .damage = 2, .hits = 3 },
    { .name = "Regard perçant"sv, .type = ATTACK, .damage = 10 },
};
constexpr EnemyStep INKLET_STEPS[] = {
    play(0, 1),
    draw(1, { 2, 1, Repeat::NO_REPEAT }, { 1, 1, Repeat::NO_REPEAT }),
};

// Butt, Slice, Hiss, and again. The first of a group starts on Slice, the ones behind on Hiss.
constexpr EnemyMove NIBBIT_MOVES[] = {
    { .name = "Derrière"sv, .type = ATTACK, .damage = 12 },
    { .name = "Slice"sv, .type = ATTACK_DEFEND, .damage = 6, .block = 5 },
    { .name = "Chuintement"sv, .type = BUFF, .self_status = StatusType::STRENGTH, .self_amount = 2 },
};
constexpr EnemyStep NIBBIT_STEPS[] = { play(0, 1), play(1, 2), play(2, 0) };

// Energy Orb every turn.
constexpr EnemyMove SNAPPING_JAXFRUIT_MOVES[] = {
    { .name = "Orbe d'énergie"sv, .type = ATTACK_BUFF, .damage = 3, .self_status = StatusType::STRENGTH,
      .self_amount = 2 },
};
constexpr EnemyStep SNAPPING_JAXFRUIT_STEPS[] = { play(0, 0) };

// Sticky Shot, then Pokey Pounce or Sticky Shot, never two Sticky Shots in a row.
constexpr EnemyMove TWIG_SLIME_M_MOVES[] = {
    { .name = "Petit saut"sv, .type = ATTACK, .damage = 11 },
    { .name = "Tir collant"sv, .type = STATUS, .card = CardId::SLIMED, .card_count = 1 },
};
constexpr EnemyStep TWIG_SLIME_M_STEPS[] = {
    play(1, 1),
    draw(1, { 0, 1, Repeat::FREE }, { 1, 1, Repeat::NO_REPEAT }),
};

// Sticky Shot, Clump Shot, and again.
constexpr EnemyMove LEAF_SLIME_M_MOVES[] = {
    { .name = "Tir groupé"sv, .type = ATTACK, .damage = 8 },
    { .name = "Tir adhésif"sv, .type = STATUS, .card = CardId::SLIMED, .card_count = 2 },
};
constexpr EnemyStep LEAF_SLIME_M_STEPS[] = { play(1, 1), play(0, 0) };

// Opens with Frail Spores or Smash, then draws among its three moves, never the same twice in a row.
constexpr EnemyMove FLYCONID_MOVES[] = {
    { .name = "Spores fragilisantes"sv, .type = DEBUFF, .player_status = StatusType::VULNERABLE,
      .player_amount = 2 },
    { .name = "Spores fragilisantes"sv, .type = ATTACK_DEBUFF, .damage = 8, .player_status = StatusType::FRAIL,
      .player_amount = 2 },
    { .name = "Fracas"sv, .type = ATTACK, .damage = 11 },
};
constexpr EnemyStep FLYCONID_STEPS[] = {
    draw(1, { 1, 1, Repeat::NO_REPEAT }, { 2, 1, Repeat::NO_REPEAT }),
    draw(1, { 0, 1, Repeat::NO_REPEAT }, { 1, 1, Repeat::NO_REPEAT }, { 2, 1, Repeat::NO_REPEAT }),
};

// Track once, then Hounds for the rest of the fight.
constexpr EnemyMove TRACKER_RUBY_RAIDER_MOVES[] = {
    { .name = "Traque"sv, .type = DEBUFF, .player_status = StatusType::FRAIL, .player_amount = 2 },
    { .name = "Lâcher les chiens"sv, .type = ATTACK, .damage = 1, .hits = 8 },
};
constexpr EnemyStep TRACKER_RUBY_RAIDER_STEPS[] = { play(0, 1), play(1, 1) };

// Claw, then a draw: Rip and Tear and Claw never twice in a row, Roar once per fight.
constexpr EnemyMove MAWLER_MOVES[] = {
    { .name = "Déchirure"sv, .type = ATTACK, .damage = 14 },
    { .name = "Rugissement"sv, .type = DEBUFF, .player_status = StatusType::VULNERABLE, .player_amount = 3 },
    { .name = "Griffe"sv, .type = ATTACK, .damage = 4, .hits = 2 },
};
constexpr EnemyStep MAWLER_STEPS[] = {
    play(2, 1),
    draw(1, { 0, 1, Repeat::NO_REPEAT }, { 1, 1, Repeat::ONCE }, { 2, 1, Repeat::NO_REPEAT }),
};

// Spiken, Spike Spit, Whirl, and again. The ones behind the first start on Whirl.
constexpr EnemyMove TOADPOLE_MOVES[] = {
    { .name = "Jet d'épines"sv, .type = ATTACK, .damage = 3, .hits = 3, .self_status = StatusType::THORNS,
      .self_amount = 2 },
    { .name = "Tourbillonnement"sv, .type = ATTACK, .damage = 7 },
    { .name = "Épines"sv, .type = BUFF, .self_status = StatusType::THORNS, .self_amount = 2 },
};
constexpr EnemyStep TOADPOLE_STEPS[] = { play(2, 1), play(0, 2), play(1, 0) };

// Oil Spray, then a draw among its three moves, never the same twice in a row.
constexpr EnemyMove SLUDGE_SPINNER_MOVES[] = {
    { .name = "Spray huileux"sv, .type = ATTACK_DEBUFF, .damage = 8, .player_status = StatusType::WEAK,
      .player_amount = 1 },
    { .name = "Heurt"sv, .type = ATTACK, .damage = 11 },
    { .name = "Rage"sv, .type = ATTACK_BUFF, .damage = 6, .self_status = StatusType::STRENGTH, .self_amount = 3 },
};
constexpr EnemyStep SLUDGE_SPINNER_STEPS[] = {
    play(0, 1),
    draw(1, { 0, 1, Repeat::NO_REPEAT }, { 1, 1, Repeat::NO_REPEAT }, { 2, 1, Repeat::NO_REPEAT }),
};

// Ready, Fast Punch, Strong Punch, and again.
constexpr EnemyMove PUNCH_CONSTRUCT_MOVES[] = {
    { .name = "PRET"sv, .type = DEFEND, .block = 10 },
    { .name = "Poing puissant"sv, .type = ATTACK, .damage = 14 },
    { .name = "Poing rapide"sv, .type = ATTACK_DEBUFF, .damage = 5, .hits = 2, .player_status = StatusType::FRAIL,
      .player_amount = 1 },
};
constexpr EnemyStep PUNCH_CONSTRUCT_STEPS[] = { play(0, 1), play(2, 2), play(1, 0) };

// Gimme, Double Smash, Hehe, and again.
constexpr EnemyMove GREMLIN_MERC_MOVES[] = {
    { .name = "Donne-moi ça !"sv, .type = ATTACK, .damage = 7, .hits = 2 },
    { .name = "Fracas double"sv, .type = ATTACK_DEBUFF, .damage = 6, .hits = 2, .player_status = StatusType::WEAK,
      .player_amount = 2 },
    { .name = "Héhé"sv, .type = ATTACK_BUFF, .damage = 8, .self_status = StatusType::STRENGTH, .self_amount = 2 },
};
constexpr EnemyStep GREMLIN_MERC_STEPS[] = { play(0, 1), play(1, 2), play(2, 0) };

// Haunt once, then Swipe and Stomp in turn.
constexpr EnemyMove HAUNTED_SHIP_MOVES[] = {
    { .name = "Griffe"sv, .type = ATTACK, .damage = 13 },
    { .name = "Piétinement"sv, .type = ATTACK, .damage = 4, .hits = 3 },
    { .name = "Hanter"sv, .type = DEBUFF, .player_status = StatusType::WEAK, .player_amount = 3,
      .card = CardId::DAZED, .card_count = 5 },
};
constexpr EnemyStep HAUNTED_SHIP_STEPS[] = { play(2, 1), play(0, 2), play(1, 1) };

// Whip Slap, Glomp, Goop, and again.
constexpr EnemyMove CORPSE_SLUG_MOVES[] = {
    { .name = "Fouet"sv, .type = ATTACK, .damage = 3, .hits = 2 },
    { .name = "Câlin brutal"sv, .type = ATTACK, .damage = 8 },
    { .name = "Mucus"sv, .type = DEBUFF, .player_status = StatusType::FRAIL, .player_amount = 2 },
};
constexpr EnemyStep CORPSE_SLUG_STEPS[] = { play(0, 1), play(1, 2), play(2, 0) };

// Latch, then a draw among its three moves, which can repeat.
constexpr EnemyMove FOSSIL_STALKER_MOVES[] = {
    { .name = "Charge"sv, .type = ATTACK_DEBUFF, .damage = 9, .player_status = StatusType::FRAIL,
      .player_amount = 1 },
    { .name = "Accrochage"sv, .type = ATTACK, .damage = 12 },
    { .name = "Fouet"sv, .type = ATTACK, .damage = 3, .hits = 2 },
};
constexpr EnemyStep FOSSIL_STALKER_STEPS[] = {
    play(1, 1),
    draw(1, { 1, 1, Repeat::FREE }, { 0, 1, Repeat::FREE }, { 2, 1, Repeat::FREE }),
};

// Sea Kick, Spinning Kick, Bubble Burp, and again.
constexpr EnemyMove SEAPUNK_MOVES[] = {
    { .name = "Coup de pied marin"sv, .type = ATTACK, .damage = 11 },
    { .name = "Coup de pied tournoyant"sv, .type = ATTACK, .damage = 2, .hits = 4 },
    { .name = "Jet de bulles"sv, .type = BUFF, .block = 7, .self_status = StatusType::STRENGTH, .self_amount = 1 },
};
constexpr EnemyStep SEAPUNK_STEPS[] = { play(0, 1), play(1, 2), play(2, 0) };

// Swoop, Peck, and again.
constexpr EnemyMove BYRDONIS_MOVES[] = {
    { .name = "Coup de bec"sv, .type = ATTACK, .damage = 3, .hits = 3 },
    { .name = "Piqué"sv, .type = ATTACK, .damage = 17 },
};
constexpr EnemyStep BYRDONIS_STEPS[] = { play(1, 1), play(0, 0) };

// Sleeps a turn, wakes with 10 Strength, then Slashes for the rest of the fight.
constexpr EnemyMove BYGONE_EFFIGY_MOVES[] = {
    { .name = "Sommeil"sv, .type = STUN },
    { .name = "Éveil"sv, .type = BUFF, .self_status = StatusType::STRENGTH, .self_amount = 10 },
    { .name = "Bourrasque"sv, .type = ATTACK, .damage = 13 },
};
constexpr EnemyStep BYGONE_EFFIGY_STEPS[] = { play(0, 1), play(1, 2), play(2, 2) };

// Zoom twice, Inertia, Piercing Stabs, and again.
constexpr EnemyMove SKULKING_COLONY_MOVES[] = {
    { .name = "Bourdonnement"sv, .type = ATTACK, .damage = 14 },
    { .name = "Inertie"sv, .type = ATTACK_BUFF, .damage = 9, .self_status = StatusType::STRENGTH, .self_amount = 2 },
    { .name = "Coups perforants"sv, .type = ATTACK, .damage = 7, .hits = 2 },
};
constexpr EnemyStep SKULKING_COLONY_STEPS[] = { play(0, 1), play(0, 2), play(1, 3), play(2, 0) };

// Crash and Thrash in turn. At half its HP: stunned for a turn, Terror, then Crash and Thrash again.
constexpr EnemyMove TERROR_EEL_MOVES[] = {
    { .name = "Fracas"sv, .type = ATTACK, .damage = 16 },
    { .name = "Furie"sv, .type = ATTACK_BUFF, .damage = 3, .hits = 3, .self_status = StatusType::VIGOR,
      .self_amount = 6 },
    { .name = "Étourdi"sv, .type = STUN },
    { .name = "Aura de terreur"sv, .type = DEBUFF, .player_status = StatusType::VULNERABLE, .player_amount = 99 },
};
constexpr EnemyStep TERROR_EEL_STEPS[] = { play(0, 1), play(1, 0), play(2, 3), play(3, 0) };

constexpr MonsterDef MONSTERS[] = {
    { EnemyType::CUBEX_CONSTRUCT, "Assemblage Cubex"sv, 65, CUBEX_CONSTRUCT_MOVES, CUBEX_CONSTRUCT_STEPS },
    { EnemyType::INKLET, "Encrelin"sv, 11, INKLET_MOVES, INKLET_STEPS },
    { EnemyType::NIBBIT, "Grignotin"sv, 42, NIBBIT_MOVES, NIBBIT_STEPS, 1, 2 },
    { EnemyType::SNAPPING_JAXFRUIT, "Jaxier à clapet"sv, 31, SNAPPING_JAXFRUIT_MOVES, SNAPPING_JAXFRUIT_STEPS },
    { EnemyType::TWIG_SLIME_M, "Limon brindille (M)"sv, 26, TWIG_SLIME_M_MOVES, TWIG_SLIME_M_STEPS },
    { EnemyType::LEAF_SLIME_M, "Limon feuillu (M)"sv, 32, LEAF_SLIME_M_MOVES, LEAF_SLIME_M_STEPS },
    { EnemyType::FLYCONID, "Mycoptère"sv, 47, FLYCONID_MOVES, FLYCONID_STEPS },
    { EnemyType::TRACKER_RUBY_RAIDER, "Rôdeur Traqueur"sv, 21, TRACKER_RUBY_RAIDER_MOVES,
      TRACKER_RUBY_RAIDER_STEPS },
    { EnemyType::MAWLER, "Écharneur"sv, 72, MAWLER_MOVES, MAWLER_STEPS },
    { EnemyType::TOADPOLE, "Crapautard"sv, 21, TOADPOLE_MOVES, TOADPOLE_STEPS, 0, 2 },
    { EnemyType::SLUDGE_SPINNER, "Tisse-boue"sv, 37, SLUDGE_SPINNER_MOVES, SLUDGE_SPINNER_STEPS },
    { EnemyType::PUNCH_CONSTRUCT, "Assemblage de pugilat"sv, 55, PUNCH_CONSTRUCT_MOVES, PUNCH_CONSTRUCT_STEPS },
    { EnemyType::GREMLIN_MERC, "Diablotin mercenaire"sv, 47, GREMLIN_MERC_MOVES, GREMLIN_MERC_STEPS },
    { EnemyType::HAUNTED_SHIP, "Navire hanté"sv, 63, HAUNTED_SHIP_MOVES, HAUNTED_SHIP_STEPS },
    { EnemyType::CORPSE_SLUG, "Putrilimace"sv, 25, CORPSE_SLUG_MOVES, CORPSE_SLUG_STEPS },
    { EnemyType::FOSSIL_STALKER, "Traqueur fossilisé"sv, 51, FOSSIL_STALKER_MOVES, FOSSIL_STALKER_STEPS },
    { EnemyType::SEAPUNK, "Voyou marin"sv, 44, SEAPUNK_MOVES, SEAPUNK_STEPS },
    { EnemyType::BYRDONIS, "Oizodon"sv, 81, BYRDONIS_MOVES, BYRDONIS_STEPS },
    { EnemyType::BYGONE_EFFIGY, "Idole Oubliée"sv, 127, BYGONE_EFFIGY_MOVES, BYGONE_EFFIGY_STEPS },
    { EnemyType::SKULKING_COLONY, "Colonie de rôdeurs"sv, 75, SKULKING_COLONY_MOVES, SKULKING_COLONY_STEPS },
    { EnemyType::TERROR_EEL, "Murène de l'effroi"sv, 140, TERROR_EEL_MOVES, TERROR_EEL_STEPS, 0, 0, 50, 2 },
};

// The move table of a monster, or nullptr for the monsters written by hand in init() and plan_turn().
const MonsterDef* move_table(EnemyType type) {
    for (const MonsterDef& def : MONSTERS) {
        if (def.type == type) return &def;
    }
    return nullptr;
}

uint8_t first_step(const MonsterDef& def, EnemyRank rank) {
    if (rank == EnemyRank::FRONT) return def.front_start;
    return rank == EnemyRank::BACK ? def.back_start : 0;
}

bool is_drawn(const EnemyStep& step) {
    return step.options[1].weight > 0;
}

// The move a step plays. A drawn step picks among the options its rule allows, by weight; when the rules
// leave nothing, among all of them.
uint8_t pick_move(const EnemyStep& step, uint8_t last_move, uint8_t used_moves, uint32_t roll) {
    if (!is_drawn(step)) return step.options[0].move;
    uint32_t total = 0;
    bool allowed[4] = {};
    for (size_t i = 0; i < 4; ++i) {
        const MoveOption& option = step.options[i];
        if (option.weight == 0) continue;
        if (option.repeat == Repeat::NO_REPEAT && option.move == last_move) continue;
        if (option.repeat == Repeat::ONCE && (used_moves >> option.move & 1)) continue;
        allowed[i] = true;
        total += option.weight;
    }
    if (total == 0) {
        for (size_t i = 0; i < 4; ++i) {
            allowed[i] = step.options[i].weight > 0;
            total += step.options[i].weight;
        }
    }
    uint32_t pick = roll % total;
    for (size_t i = 0; i < 4; ++i) {
        if (!allowed[i]) continue;
        if (pick < step.options[i].weight) return step.options[i].move;
        pick -= step.options[i].weight;
    }
    return step.options[0].move;
}

} // namespace

void Enemy::init(EnemyType enemy_type, EnemyRank rank, uint32_t roll) {
    type = enemy_type;
    statuses.clear();
    block = 0;
    damage_percent = 100;
    move_step = 0;
    last_move = 0xFF;
    used_moves = 0;
    phase_changed = false;
    escaped = false;

    if (const MonsterDef* def = move_table(type)) {
        name = def->name;
        max_hp = def->hp;
        hp = def->hp;
        move_step = first_step(*def, rank);
        plan_turn(1, roll);
        return;
    }

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
    } else {
        name = "Monstre";
        max_hp = 30;
        hp = 30;
    }

    plan_turn(1);
}

bool Enemy::opens_with_draw(EnemyType enemy_type, EnemyRank rank) {
    const MonsterDef* def = move_table(enemy_type);
    return def && is_drawn(def->steps[first_step(*def, rank)]);
}

// Step the next plan plays: the one the pattern has reached, or the first of the second phase when the HP
// threshold has just been passed.
static uint8_t upcoming_step(const MonsterDef& def, const Enemy& enemy) {
    if (def.phase_hp_percent > 0 && !enemy.phase_changed &&
        enemy.hp * 100 <= enemy.max_hp * def.phase_hp_percent) {
        return def.phase_step;
    }
    return enemy.move_step;
}

bool Enemy::wants_roll() const {
    const MonsterDef* def = move_table(type);
    return def && is_drawn(def->steps[upcoming_step(*def, *this)]);
}

void Enemy::plan_turn(int turn_number, uint32_t roll) {
    // Start from a blank intent: a field set by last turn's move (a Strength buff) must not leak
    // into this one, since execute_turn() applies whatever the intent holds.
    current_intent = EnemyIntent{};
    if (const MonsterDef* def = move_table(type)) {
        const uint8_t step_index = upcoming_step(*def, *this);
        if (step_index != move_step) phase_changed = true;
        const EnemyStep& step = def->steps[step_index];
        const uint8_t move_index = pick_move(step, last_move, used_moves, roll);
        last_move = move_index;
        used_moves = static_cast<uint8_t>(used_moves | (1u << move_index));
        move_step = step.next;

        const EnemyMove& move = def->moves[move_index];
        current_intent.type = move.type;
        current_intent.base_damage = move.damage;
        current_intent.hits = move.hits;
        current_intent.block = move.block;
        current_intent.status_type = move.self_status;
        current_intent.status_amount = move.self_amount;
        current_intent.player_status = move.player_status;
        current_intent.player_status_amount = move.player_amount;
        current_intent.card = move.card;
        current_intent.card_count = move.card_count;
        current_intent.heal = move.heal;
        current_intent.description = move.name;
        return;
    }
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
    }
}

void Enemy::execute_turn(Player& player, int16_t damage_back, bool reflect) {
    clear_block();
    decay_plating();

    // Ritual gained this turn only starts granting Strength at the end of the next one.
    int16_t ritual = get_status(StatusType::RITUAL);

    // Gain block from intent if any
    if (current_intent.block > 0) {
        gain_block(current_intent.block);
    }

    // Apply status buff to self if any. A move that also attacks gives it after the attack, so that the hits
    // deal what the intent announced.
    const bool attacks = current_intent.base_damage > 0;
    const bool buffs = current_intent.status_type != StatusType::NONE && current_intent.status_amount > 0;
    if (buffs && !attacks) {
        add_status(current_intent.status_type, current_intent.status_amount);
    }

    // Execute attack if any, one hit after the other: each one is answered on its own.
    if (attacks) {
        for (uint8_t hit = 0; hit == 0 || hit < current_intent.hits; ++hit) {
            if (hit > 0 && (!is_alive() || !player.is_alive())) break;
            const int16_t block_before = player.block;
            player.take_damage_with_minion(intent_damage(player));
            // The attacker gets hurt back: the player's Thorns, Flame Barrier, and the damage Reflect sends back.
            int16_t back = static_cast<int16_t>(player.get_status(StatusType::THORNS) + damage_back);
            if (reflect) back = static_cast<int16_t>(back + block_before - player.block);
            take_damage(back);
        }
        clear_status(StatusType::VIGOR); // spent on this attack
        if (buffs && is_alive()) add_status(current_intent.status_type, current_intent.status_amount);
    }

    if (is_alive()) {
        // What the move does to the player once the hits are dealt. Artifact cancels a debuff.
        if (current_intent.player_status != StatusType::NONE && current_intent.player_status_amount != 0) {
            player.receive_status(current_intent.player_status, current_intent.player_status_amount);
        }
        heal(current_intent.heal);
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
}

int16_t Enemy::intent_damage(const CombatEntity& target) const {
    if (current_intent.base_damage <= 0) return 0;
    return attack_damage(static_cast<int16_t>(current_intent.base_damage * damage_percent / 100), *this, target);
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
