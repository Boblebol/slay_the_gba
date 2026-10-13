#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "engine/status.h"
#include "engine/power.h"
#include "engine/card.h"
#include "engine/fixed_vector.h"
#include "engine/monster.h"

namespace engine {

// One slot per StatusType: an entity can then hold every status at once, and none is dropped.
constexpr size_t MAX_STATUSES = 18;
static_assert(MAX_STATUSES >= static_cast<size_t>(StatusType::CONFUSED), "a status added to StatusType needs its slot");

class CombatEntity {
public:
    int16_t hp = 0;
    int16_t max_hp = 0;
    int16_t block = 0;
    FixedVector<StatusEffect, MAX_STATUSES> statuses;
    // Vulnerable, Weak and Frail received during the enemies' turn (one bit each): they are not worn down by the
    // end of that same round, or a debuff of 1 turn would be gone before it did anything.
    uint8_t fresh_debuffs = 0;
    uint8_t debilitate_turns = 0;          // turns left where this entity's Vulnerable and Weak are twice as effective
    bool halve_vulnerable_damage = false;  // takes 50% less damage from Vulnerable enemies until its next turn (Colossus)
    // HP this entity may still lose this turn, -1 for no limit (Beating Remnant, set at the start of the player's turn)
    int16_t hp_loss_budget = -1;
    uint8_t hp_loss_reduction = 0;         // every HP loss is this much smaller (Tungsten Rod, the player only)
    bool paper_krane = false;              // Weak enemies deal 40% less damage to it instead of 25% (Paper Krane, the player only)

    bool is_alive() const { return hp > 0; }

    void add_status(StatusType type, int16_t amount) {
        for (size_t i = 0; i < statuses.size(); ++i) {
            if (statuses[i].type == type) {
                statuses[i].amount += amount;
                // A spent status frees its slot.
                if (statuses[i].amount == 0) statuses.erase(i);
                return;
            }
        }
        if (amount != 0) statuses.push_back({ type, amount });
    }

    int16_t get_status(StatusType type) const {
        for (size_t i = 0; i < statuses.size(); ++i) {
            if (statuses[i].type == type) {
                return statuses[i].amount;
            }
        }
        return 0;
    }

    // A status given by an effect: Artifact cancels a debuff, and is used up doing so. False when it did.
    bool receive_status(StatusType type, int16_t amount) {
        if (is_debuff(type, amount) && get_status(StatusType::ARTIFACT) > 0) {
            add_status(StatusType::ARTIFACT, -1);
            return false;
        }
        add_status(type, amount);
        return true;
    }

    // Removes the whole amount of a status.
    void clear_status(StatusType type) {
        add_status(type, static_cast<int16_t>(-get_status(type)));
    }

    // Bit of `fresh_debuffs` for a status that wears off each round, 0 for the others.
    static constexpr uint8_t fresh_bit(StatusType type) {
        return type == StatusType::VULNERABLE ? 1 : (type == StatusType::WEAK ? 2 : (type == StatusType::FRAIL ? 4 : 0));
    }

    void tick_end_of_turn_statuses() {
        // Decrease Vulnerable, Weak, Frail and Intangible by 1
        const uint8_t fresh = fresh_debuffs;
        fresh_debuffs = 0;
        for (size_t i = 0; i < statuses.size();) {
            const StatusType type = statuses[i].type;
            if ((type == StatusType::VULNERABLE || type == StatusType::WEAK || type == StatusType::FRAIL ||
                 type == StatusType::INTANGIBLE) && !(fresh & fresh_bit(type))) {
                statuses[i].amount -= 1;
                if (statuses[i].amount <= 0) {
                    statuses.erase(i);
                    continue;
                }
            }
            ++i;
        }
    }

    void clear_block() {
        block = 0;
    }

    // Block from a card: Dexterity is added, then Frail takes a quarter away (rounded down).
    void gain_block(int16_t amount) {
        if (amount > 0) {
            int16_t dex = get_status(StatusType::DEXTERITY);
            int16_t total = amount + dex;
            if (total > 0 && get_status(StatusType::FRAIL) > 0) {
                total = static_cast<int16_t>(total * 3 / 4);
            }
            if (total > 0) {
                block += total;
            }
        }
    }

    // HP loss that Block does not absorb: Intangible reduces it to 1, Buffer cancels it. Returns the loss that went
    // through, which can be more than the HP that was left.
    int16_t lose_hp(int16_t amount) {
        if (amount > 0) amount = static_cast<int16_t>(amount > hp_loss_reduction ? amount - hp_loss_reduction : 0);
        if (amount <= 0) return 0;
        if (amount > 1 && get_status(StatusType::INTANGIBLE) > 0) amount = 1;
        if (get_status(StatusType::BUFFER) > 0) {
            add_status(StatusType::BUFFER, -1);
            return 0;
        }
        if (hp_loss_budget >= 0) {
            amount = static_cast<int16_t>(amount < hp_loss_budget ? amount : hp_loss_budget);
            hp_loss_budget = static_cast<int16_t>(hp_loss_budget - amount);
            if (amount <= 0) return 0;
        }
        hp = static_cast<int16_t>(hp - amount);
        if (hp < 0) hp = 0;
        return amount;
    }

    int16_t apply_poison_tick() {
        int16_t p = get_status(StatusType::POISON);
        if (p > 0) {
            int16_t lost = lose_hp(p);
            add_status(StatusType::POISON, -1);
            return lost;
        }
        return 0;
    }

    void apply_metallicize_tick() {
        int16_t m = get_status(StatusType::METALLICIZE);
        if (m > 0) {
            block += m;
        }
    }

    // End of its owner's turn: Plating gives its Block. It goes down at the start of the turn (decay_plating).
    void apply_plating_tick() {
        int16_t plating = get_status(StatusType::PLATED_ARMOR);
        if (plating > 0) {
            block = static_cast<int16_t>(block + plating);
        }
    }

    void decay_plating() {
        if (get_status(StatusType::PLATED_ARMOR) > 0) add_status(StatusType::PLATED_ARMOR, -1);
    }

    // End of its owner's turn: Regen heals, then goes down by 1. Returns the HP healed.
    int16_t apply_regen_tick() {
        int16_t regen = get_status(StatusType::REGEN);
        if (regen <= 0) return 0;
        int16_t before = hp;
        heal(regen);
        add_status(StatusType::REGEN, -1);
        return static_cast<int16_t>(hp - before);
    }

    // Returns damage inflicted to HP
    int16_t take_damage(int16_t amount) {
        if (amount <= 0) return 0;
        // Intangible: whatever the hit, 1 damage at most, which Block can still absorb.
        if (amount > 1 && get_status(StatusType::INTANGIBLE) > 0) amount = 1;

        int16_t unblocked = amount;
        if (block > 0) {
            if (block >= unblocked) {
                block -= unblocked;
                unblocked = 0;
            } else {
                unblocked -= block;
                block = 0;
            }
        }

        return lose_hp(unblocked);
    }

    void heal(int16_t amount) {
        if (amount > 0) {
            hp = std::min<int16_t>(max_hp, hp + amount);
        }
    }

    // Lowers Max HP (never below 1) and drags HP down with it. On the player, RunManager::proceed_after_combat
    // writes Max HP back to the run, so the loss lasts for the rest of the run (Brightest Flame).
    void lose_max_hp(int16_t amount) {
        if (amount <= 0) return;
        max_hp = static_cast<int16_t>(std::max<int>(1, max_hp - amount));
        hp = std::min<int16_t>(hp, max_hp);
    }
};

class Player : public CombatEntity {
public:
    int8_t energy = 3;
    int8_t max_energy = 3;

    // Defect Orbs. CombatEngine channels, evokes and triggers them, and changes the number of slots.
    FixedVector<Orb, MAX_ORB_SLOTS> orbs;
    uint8_t orb_slots = STARTING_ORB_SLOTS;

    // Regent Stars: a second resource, kept from one turn to the next
    uint8_t sovereign_stars = 0;

    // Necrobinder Minion (Osty)
    int16_t minion_hp = 0;
    int16_t minion_block = 0;
    int16_t minion_max_hp = 0; // Osty's Max HP for this combat: 0 while he is dead

    void init() {
        max_hp = 80;
        hp = 80;
        block = 0;
        energy = 3;
        max_energy = 3;
        statuses.clear();
        orbs.clear();
        orb_slots = STARTING_ORB_SLOTS;
        sovereign_stars = 0;
        minion_hp = 0;
        minion_block = 0;
        minion_max_hp = 0;
        hp_loss_reduction = 0;
        paper_krane = false;
    }

    void reset_turn() {
        block = 0;
        energy = max_energy;
    }

    // An enemy's attack: the player's Block absorbs it first, as in the original game, then Osty takes what is
    // left in the player's place, and what Osty could not take is lost by the player.
    int16_t take_damage_with_minion(int16_t amount) {
        if (amount <= 0) return 0;
        if (amount > 1 && get_status(StatusType::INTANGIBLE) > 0) amount = 1;

        const int16_t blocked = std::min(block, amount);
        block = static_cast<int16_t>(block - blocked);
        amount = static_cast<int16_t>(amount - blocked);

        if (minion_hp > 0 && amount > 0) {
            const int16_t shielded = std::min(minion_block, amount);
            minion_block = static_cast<int16_t>(minion_block - shielded);
            amount = static_cast<int16_t>(amount - shielded);

            const int16_t taken = std::min(minion_hp, amount);
            minion_hp = static_cast<int16_t>(minion_hp - taken);
            amount = static_cast<int16_t>(amount - taken);
            if (minion_hp == 0) {
                minion_max_hp = 0; // Osty dies: what is left of the hit goes on to the player
                minion_block = 0;
            }
        }
        return lose_hp(amount);
    }

    void add_stars(uint8_t count);
    // Summon `m_hp`: Osty comes with that many HP, or gains as many HP and Max HP when he is already alive.
    void summon_minion(int16_t m_hp, int16_t m_block = 0);
    bool minion_alive() const { return minion_hp > 0; }
    // Osty recovers HP, up to his Max HP. Nothing happens when he is dead. Returns the HP recovered.
    int16_t heal_minion(int16_t amount);
    // Osty dies: his HP, Max HP and Block are gone. Returns the HP he had left.
    int16_t kill_minion();
};

// Attack damage as in Slay the Spire: (base + Strength + Vigor), x0.75 when the attacker is Weak (x0.6 against a target
// with Paper Krane), x1.5 when the target is Vulnerable, rounded down once at the end, and 1 at most against an
// Intangible target. `percent` is the total of the other multipliers (Cruelty, Tracking, Lethality, Shadow Step), 100 =
// none. Debilitate makes Weak x0.5 and Vulnerable x2 on the entity that carries it.
inline int16_t attack_damage(int16_t base, const CombatEntity& attacker, const CombatEntity& target, int32_t percent = 100) {
    int32_t damage = base + attacker.get_status(StatusType::STRENGTH) + attacker.get_status(StatusType::VIGOR);
    if (damage <= 0) return 0;
    // Weak in twentieths (20 = no Weak), Vulnerable in halves (2 = no Vulnerable): the total is divided by 40.
    int32_t weak = attacker.debilitate_turns > 0 ? 10 : (target.paper_krane ? 12 : 15);
    int32_t vulnerable = target.debilitate_turns > 0 ? 4 : 3;
    damage *= attacker.get_status(StatusType::WEAK) > 0 ? weak : 20;
    damage *= target.get_status(StatusType::VULNERABLE) > 0 ? vulnerable : 2;
    damage /= 40;
    if (percent != 100) damage = damage * percent / 100;
    if (damage > 1 && target.get_status(StatusType::INTANGIBLE) > 0) damage = 1;
    return static_cast<int16_t>(damage);
}

enum class EnemyIntentType : uint8_t {
    ATTACK,
    BUFF,
    DEFEND,
    ATTACK_DEFEND,
    DEBUFF,        // only harms the player with statuses
    ATTACK_DEBUFF, // attacks, then harms the player with statuses
    ATTACK_BUFF,   // attacks, then gives itself a status
    STUN,          // loses its turn
    HEAL,          // only heals itself
    ESCAPE,        // leaves the fight: it is not killed and gives nothing
    STATUS,        // only puts cards in the player's piles
    SUMMON         // brings other enemies into the fight (EnemyIntent::summons), or hatches
};

enum class EnemyType : uint8_t;

// A status an enemy move gives the player: Weak, Vulnerable or Frail (amount > 0), or Strength or Dexterity
// taken away (amount < 0).
struct IntentDebuff {
    StatusType type = StatusType::NONE;
    int16_t amount = 0;
};

constexpr size_t MAX_INTENT_DEBUFFS = 2;

struct EnemyIntent {
    EnemyIntentType type = EnemyIntentType::ATTACK;
    int16_t base_damage = 0;
    int16_t block = 0;
    StatusType status_type = StatusType::NONE;
    int16_t status_amount = 0;
    std::string_view description = "";
    // Cards put in one of the player's piles (Status cards mostly), after the attack if the move has one.
    CardId status_card = CardId::NONE;
    uint8_t status_cards = 0;
    Pile status_pile = Pile::DISCARD;
    // Times the attack hits. Each hit goes through Block and is answered by Thorns on its own.
    uint8_t hits = 1;
    // Power the move grants once played: to the enemy itself, or to the player (`power_on_player`).
    PowerId power = PowerId::NONE;
    int16_t power_amount = 0;
    bool power_on_player = false;
    bool power_turn_only = false; // on the player: gone at the end of their next turn
    // HP the enemy gives itself back, after the attack if the move has one. Never above its maximum.
    int16_t heal = 0;
    // Statuses given to the player, after the attack if the move has one. Artifact cancels each of them.
    IntentDebuff debuffs[MAX_INTENT_DEBUFFS];
    // The enemy keeps count of what its debuffs took from the player and gives it back when it dies.
    bool steals = false;
    // Status of the enemy whose amount is added to the damage of the attack (the Forgotten's Dexterity).
    StatusType damage_status = StatusType::NONE;
    // Enemies the move brings into the fight (SUMMON), each in a free place: there are 3 at most.
    EnemyType summon_type{};
    uint8_t summons = 0;
    bool summon_minions = false; // they are Minions of the summoner: they flee when it is gone

    constexpr bool debuffs_player() const { return debuffs[0].type != StatusType::NONE; }
    constexpr bool gives_cards() const { return status_card != CardId::NONE && status_cards > 0; }
};

enum class EnemyType : uint8_t {
    CULTIST,
    JAW_WORM,
    LOUSE,
    GREMLIN_NOB,
    CEREMONIAL_BEAST, // Act 1 boss
    // Named like their identifier in bible/data/monster_v0.111.0.json
    FUZZY_WURM_CRAWLER,
    CROSSBOW_RUBY_RAIDER,
    BRUTE_RUBY_RAIDER,
    ASSASSIN_RUBY_RAIDER,
    AXE_RUBY_RAIDER,
    CALCIFIED_CULTIST,
    DAMP_CULTIST,
    SEWER_CLAM,
    THE_FORGOTTEN,
    LIVING_SHIELD,
    FAT_GREMLIN,
    VINE_SHAMBLER,
    EYE_WITH_TEETH,
    STABBOT,
    NOISEBOT,
    CUBEX_CONSTRUCT,
    PUNCH_CONSTRUCT,
    INKLET,
    NIBBIT,
    FLYCONID,
    CORPSE_SLUG,
    GAS_BOMB,
    GREMLIN_MERC,
    SNEAKY_GREMLIN,
    HAUNTED_SHIP,
    SLUDGE_SPINNER,
    TOADPOLE,
    HUNTER_KILLER,
    MAWLER,
    FOSSIL_STALKER,
    SPINY_TOAD,
    SEAPUNK,
    TRACKER_RUBY_RAIDER,
    OSTY,
    CHOMPER,
    SLUMBERING_BEETLE,
    PARAFRIGHT,
    TUNNELER,
    ZAPBOT,
    SLIMED_BERSERKER,
    FROG_KNIGHT,
    GLOBE_HEAD,
    LOUSE_PROGENITOR,
    GUARDBOT,
    BOWLBUG_NECTAR,
    DEVOTED_SCULPTOR,
    THE_LOST,
    AXEBOT,
    TURRET_OPERATOR,
    BOWLBUG_SILK,
    BOWLBUG_ROCK,
    BOWLBUG_EGG,
    SKULKING_COLONY,
    TERROR_EEL,
    ENTOMANCER,
    INFESTED_PRISM,
    FLAIL_KNIGHT,
    SPECTRAL_KNIGHT,
    MAGI_KNIGHT,
    SOUL_NEXUS,
    MECHA_KNIGHT,
    BYGONE_EFFIGY,
    BYRDONIS,
    PHROG_PARASITE,
    WRIGGLER,
    THE_INSATIABLE, // Act 2 boss
    TOUGH_EGG,
    THE_OBSCURA, // Act 2
    OVICOPTER,   // Act 2
    FABRICATOR,  // Act 3
    SHRINKER_BEETLE, // Act 1 (A)
    OWL_MAGISTRATE,  // Act 3
    SCROLL_OF_BITING, // Act 3
    FOGMOG,               // Act 1 (A)
    SNAPPING_JAXFRUIT,    // Act 1 (A)
    TWIG_SLIME_M,         // Act 1 (A)
    TWIG_SLIME_S,         // Act 1 (A)
    LEAF_SLIME_M,         // Act 1 (A)
    LEAF_SLIME_S,         // Act 1 (A)
    SLITHERING_STRANGLER, // Act 1 (A)
    LIVING_FOG,           // Act 1 (B)
    TWO_TAILED_RAT,       // Act 1 (B)
    LAGAVULIN_MATRIARCH,  // Act 1 (B) boss
    VANTOM,               // Act 1 (A) boss
    TEST_SUBJECT,         // Act 3 boss
    AEONGLASS             // Act 3 boss
};

// Enemies a fight holds at most, summoned ones included (CombatEngine::enemies).
constexpr uint8_t MAX_ENEMIES = 6;

// What a monster sees of the fight when it chooses its next move.
struct PlanContext {
    uint8_t allies = 0; // other enemies still alive
    uint8_t slot = 0;   // its place among the enemies, 0 for the first
};

// A move a monster may draw at random. Its chance is `weight` out of the weights of the moves it may draw
// this turn; unless `can_repeat`, the move it just played is left out of the draw.
struct MoveOption {
    uint8_t move;
    uint8_t weight = 1;
    bool can_repeat = false;
};

constexpr uint8_t NO_MOVE = 0xFF;

// Liquify Ground, the Insatiable's opening move: the Sandpit it starts from, and the Frantic Escapes shuffled
// into the player's draw pile.
constexpr uint8_t INSATIABLE_SANDPIT = 4;
constexpr uint8_t INSATIABLE_FRANTIC_ESCAPES = 6;

// The Lagavulin Matriarch starts the fight Asleep behind its Plating: it sleeps this many turns at most, and the
// first HP it loses wakes it sooner. Awake, it loses its Plating.
constexpr int16_t LAGAVULIN_PLATING = 12;
constexpr uint8_t LAGAVULIN_SLEEP_TURNS = 3;

// Stamp, the Ceremonial Beast's opening move: the Plow it gives itself (Ascension: the second value). Once its HP
// fall below it, the Beast loses its Strength, is stunned, and enters its second phase.
constexpr int16_t CEREMONIAL_BEAST_PLOW = 150;
constexpr int16_t CEREMONIAL_BEAST_PLOW_ASC = 160;

// The Test Subject has three lives: killed, it respawns (Enemy::respawn) with the HP of its next phase, twice. The
// monster database gives it no HP: these are guessed, in line with the other Act 3 bosses. The first life is also
// the HP of its sheet (src/engine/monster.cpp).
constexpr uint8_t TEST_SUBJECT_RESPAWNS = 2;
constexpr int16_t TEST_SUBJECT_HP[TEST_SUBJECT_RESPAWNS + 1] = { 100, 150, 200 };     // guessed
constexpr int16_t TEST_SUBJECT_HP_ASC[TEST_SUBJECT_RESPAWNS + 1] = { 110, 165, 220 }; // guessed

// Increasing Intensity, the Aeonglass's third move: the Strength it gives itself (Ascension: the second value). The
// monster database names the buff without its amount: guessed, in line with the other bosses' +2 / +3 Strength.
constexpr int16_t AEONGLASS_STRENGTH = 2;     // guessed
constexpr int16_t AEONGLASS_STRENGTH_ASC = 3; // guessed

// Sheet of this monster in the monster database (src/engine/monster.cpp), where its name, HP and the numbers of its
// moves come from. nullptr for the monsters that keep hand-written values: the four from the first Slay the Spire.
const MonsterData* monster_sheet(EnemyType type);

class Enemy : public CombatEntity {
public:
    EnemyType type = EnemyType::CULTIST;
    std::string_view name = "Cultist";
    EnemyIntent current_intent;
    int16_t damage_percent = 100; // Ascension bonus on attack damage
    uint8_t damage_down_turns = 0; // enemy turns left where its attacks deal 30% less damage (Beetle Juice)
    int16_t temp_strength_loss = 0; // Strength taken away until the end of this enemy's turn
    bool minion = false; // summoned by another enemy: killing it triggers no Fatal effect
    int8_t leader = -1;  // a Minion's summoner, in CombatEngine::enemies: the Minion flees when it is gone
    uint8_t hang_marks = 0; // Hang cards already played on this enemy: each one doubles the next
    // What its moves stole from the player (a negative amount of Strength or Dexterity), given back when it dies.
    IntentDebuff stolen;
    // Its attacks use the damage the reference gives for Ascension (see ascend).
    bool ascended_damage = false;
    // Debuffs of the last move played: what the player received, NONE for the ones Artifact cancelled.
    IntentDebuff last_debuffs[MAX_INTENT_DEBUFFS];

    // It left the fight (ESCAPE): out like a dead enemy (0 HP), but it was not killed.
    bool escaped = false;
    int16_t last_heal = 0; // HP its last move gave it back
    uint8_t wounding_hits = 0; // hits of its last move that cost the player HP (Painful Stabs)
    // HP drawn at the start of the fight on top of its lowest HP, for the monsters whose HP vary (see hp_spread).
    uint8_t hp_bonus = 0;

    // Intent pattern. A phase is a part of the pattern entered when its condition is met (alone, hurt,
    // triggered) and never left for an earlier one; `phase_turn` counts the turns planned in it, from 1.
    uint8_t phase = 0;
    uint8_t phase_turn = 0;
    uint8_t last_move = NO_MOVE; // move of the last plan, for the draws that cannot repeat it
    uint32_t move_seed = 1;      // random stream of its own draws, set by the fight

    void init(EnemyType enemy_type);
    // How far above its lowest HP this monster can start (0 for most): the Fat Gremlin has 13 to 17 HP.
    uint8_t hp_spread() const;
    // Draws its starting HP in that range from `seed`. Does nothing for a monster whose HP do not vary.
    void roll_hp(uint32_t seed);
    // Makes it lose its next turn: its intent becomes STUN.
    void stun();
    // HP the reference gives this monster at Ascension, 0 for the monsters that follow the percentages of the run.
    int16_t ascension_hp() const;
    // Move `move_id` of its sheet in the monster database (monster_sheet): damage, hits and Block. A move the
    // sheet does not have reads as no damage, no Block and one hit.
    const MonsterMoveData& sheet_move(std::string_view move_id) const;
    // Damage of that move, the Ascension one when its attacks follow Ascension (ascended_damage).
    int16_t move_damage(std::string_view move_id) const;
    // Ascension of a monster with its own values (ascension_hp() > 0): its Ascension HP and/or damage.
    // The intent is planned again by the fight (CombatEngine::plan_enemies).
    void ascend(bool more_hp, bool more_damage);
    // Ascension of the room, as percentages of HP and damage (100 = none). A monster with its own values takes
    // them instead (ascend).
    void apply_ascension(int16_t hp_pct, int16_t damage_pct);
    // Forgets the pattern followed so far: back to the first phase, with `seed` for the random draws.
    void reset_pattern(uint32_t seed);
    // Moves to `next` when the monster is in an earlier phase. True when the phase changed.
    bool enter_phase(uint8_t next);
    // The Tunneler's Block was broken while Burrowed (triggered): it enters its next phase, stunned.
    void break_burrow();
    // The Ceremonial Beast's Plow ran out (triggered): it enters its next phase, stunned.
    void break_plow();
    // The Test Subject was killed with a life left (triggered): back to full HP in its next phase, cleared of its
    // statuses and Block, it announces Respawn. False for any other enemy, or one without a life left.
    bool respawn();
    // Draws a move among `options` by weight, leaving out the last move played unless it can repeat, and
    // remembers it. When the rule leaves no move, the last one is played again. NO_MOVE for an empty list.
    uint8_t draw_move(std::span<const MoveOption> options);
    // Remembers a move chosen without a draw, so the next draw cannot repeat it.
    uint8_t use_move(uint8_t move) { return last_move = move; }
    // Gives back to the player what this enemy stole. Called when it dies.
    void return_stolen(Player& player);
    // Damage of one hit of the current intent on `target` right now (0 when it does not attack).
    int16_t intent_damage(const CombatEntity& target, bool halved = false) const;
    void plan_turn(int turn_number, const PlanContext& context = {});
    // Plays the current intent. An attack is answered by the player's Thorns, plus `damage_back` (Flame Barrier),
    // plus the damage their Block absorbed when `reflect` is set (Reflect).
    void execute_turn(Player& player, int16_t damage_back = 0, bool reflect = false, bool halved = false);
};

} // namespace engine
