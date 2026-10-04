#pragma once

#include <cstdint>
#include <string_view>
#include "engine/status.h"
#include "engine/fixed_vector.h"

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

    void tick_end_of_turn_statuses() {
        // Decrease Vulnerable, Weak, Frail and Intangible by 1
        for (size_t i = 0; i < statuses.size();) {
            const StatusType type = statuses[i].type;
            if (type == StatusType::VULNERABLE || type == StatusType::WEAK || type == StatusType::FRAIL ||
                type == StatusType::INTANGIBLE) {
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
        if (amount <= 0) return 0;
        if (amount > 1 && get_status(StatusType::INTANGIBLE) > 0) amount = 1;
        if (get_status(StatusType::BUFFER) > 0) {
            add_status(StatusType::BUFFER, -1);
            return 0;
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

    // Necrobinder Souls & Minion (Osty)
    uint8_t souls = 0; // stands in for the Soul cards until they exist: Souls made this combat (see SOUL_CARD)
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
        souls = 0;
        minion_hp = 0;
        minion_block = 0;
        minion_max_hp = 0;
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
    void add_souls(uint8_t count);
    // Summon `m_hp`: Osty comes with that many HP, or gains as many HP and Max HP when he is already alive.
    void summon_minion(int16_t m_hp, int16_t m_block = 0);
    bool minion_alive() const { return minion_hp > 0; }
    // Osty recovers HP, up to his Max HP. Nothing happens when he is dead. Returns the HP recovered.
    int16_t heal_minion(int16_t amount);
    // Osty dies: his HP, Max HP and Block are gone. Returns the HP he had left.
    int16_t kill_minion();
};

// Attack damage as in Slay the Spire: (base + Strength + Vigor), x0.75 when the attacker is Weak, x1.5 when the
// target is Vulnerable, rounded down once at the end, and 1 at most against an Intangible target.
inline int16_t attack_damage(int16_t base, const CombatEntity& attacker, const CombatEntity& target) {
    int32_t damage = base + attacker.get_status(StatusType::STRENGTH) + attacker.get_status(StatusType::VIGOR);
    if (damage <= 0) return 0;
    damage *= attacker.get_status(StatusType::WEAK) > 0 ? 3 : 4;
    damage *= target.get_status(StatusType::VULNERABLE) > 0 ? 3 : 2;
    damage /= 8;
    if (damage > 1 && target.get_status(StatusType::INTANGIBLE) > 0) damage = 1;
    return static_cast<int16_t>(damage);
}

enum class EnemyIntentType : uint8_t {
    ATTACK,
    BUFF,
    DEFEND,
    ATTACK_DEFEND,
    // Appended: the values above keep their number
    ATTACK_BUFF,   // attacks, then gives itself a status
    ATTACK_DEBUFF, // attacks, then gives the player a debuff or Status cards
    DEBUFF,        // gives the player a debuff
    STATUS,        // adds Status cards to the player's piles
    STUN,          // skips its turn
    HEAL,          // recovers HP
    ESCAPE         // leaves the fight
};

// Whether a move of this kind hits the player.
constexpr bool intent_attacks(EnemyIntentType type) {
    return type == EnemyIntentType::ATTACK || type == EnemyIntentType::ATTACK_DEFEND ||
           type == EnemyIntentType::ATTACK_BUFF || type == EnemyIntentType::ATTACK_DEBUFF;
}

enum class CardId : uint16_t; // engine/card.h

struct EnemyIntent {
    EnemyIntentType type = EnemyIntentType::ATTACK;
    int16_t base_damage = 0;
    int16_t block = 0;
    StatusType status_type = StatusType::NONE;
    int16_t status_amount = 0;
    std::string_view description = "";
    // What a move does to the player beyond one hit.
    uint8_t hits = 1;                              // times `base_damage` is dealt
    StatusType player_status = StatusType::NONE;   // given to the player after the attack
    int8_t player_status_amount = 0;
    CardId card{};                                 // Status card added to the player's piles, `card_count` times
    uint8_t card_count = 0;
    bool card_to_draw_pile = false;                // at a random place of the draw pile instead of the discard pile
    int16_t heal = 0;                              // HP the enemy recovers
};

// Where a monster stands in its group: some patterns start on a different move for each place.
enum class EnemyRank : uint8_t {
    ALONE,
    FRONT, // first of several
    BACK
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
    // Played from the move tables of entity.cpp (MONSTERS)
    CUBEX_CONSTRUCT,
    INKLET,
    NIBBIT,
    SNAPPING_JAXFRUIT,
    TWIG_SLIME_M,
    LEAF_SLIME_M,
    FLYCONID,
    TRACKER_RUBY_RAIDER,
    MAWLER,
    TOADPOLE,
    SLUDGE_SPINNER,
    PUNCH_CONSTRUCT,
    GREMLIN_MERC,
    HAUNTED_SHIP,
    CORPSE_SLUG,
    FOSSIL_STALKER,
    SEAPUNK,
    BYRDONIS,        // elite
    BYGONE_EFFIGY,   // elite
    SKULKING_COLONY, // elite
    TERROR_EEL       // elite
};

class Enemy : public CombatEntity {
public:
    EnemyType type = EnemyType::CULTIST;
    std::string_view name = "Cultist";
    EnemyIntent current_intent;
    int16_t damage_percent = 100; // Ascension bonus on attack damage
    int16_t temp_strength_loss = 0; // Strength taken away until the end of this enemy's turn
    bool minion = false; // summoned by another enemy: killing it triggers no Fatal effect
    uint8_t hang_marks = 0; // Hang cards already played on this enemy: each one doubles the next
    // Where a monster of the move tables is in its pattern.
    uint8_t move_step = 0;      // step the next plan_turn() plays
    uint8_t last_move = 0xFF;   // move of the current intent, 0xFF before the first one
    uint8_t used_moves = 0;     // one bit per move already announced, for the moves used only once
    bool phase_changed = false; // its HP threshold has been passed
    bool escaped = false;       // left the fight: it is gone without having been killed

    // `rank` and `roll` only matter to the monsters of the move tables: the place in the group, and the random
    // number that picks the first move of those that open with a drawn one (see opens_with_draw()).
    void init(EnemyType enemy_type, EnemyRank rank = EnemyRank::ALONE, uint32_t roll = 0);
    // Whether the first move of this monster is drawn at random.
    [[nodiscard]] static bool opens_with_draw(EnemyType enemy_type, EnemyRank rank = EnemyRank::ALONE);
    // Whether the next plan_turn() draws its move at random, and so needs a `roll`.
    [[nodiscard]] bool wants_roll() const;
    // Damage one hit of the current intent would deal to `target` right now (0 when it does not attack). The
    // intent deals it `current_intent.hits` times.
    int16_t intent_damage(const CombatEntity& target) const;
    void plan_turn(int turn_number, uint32_t roll = 0);
    // Plays the current intent. An attack is answered by the player's Thorns, plus `damage_back` (Flame Barrier),
    // plus the damage their Block absorbed when `reflect` is set (Reflect).
    void execute_turn(Player& player, int16_t damage_back = 0, bool reflect = false);
};

} // namespace engine
