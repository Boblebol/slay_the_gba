#pragma once

#include <cstdint>
#include <numeric>
#include <string_view>
#include "engine/status.h"
#include "engine/fixed_vector.h"

namespace engine {

// One slot per StatusType: an entity can then hold every status at once, and none is dropped.
constexpr size_t MAX_STATUSES = 18;
static_assert(MAX_STATUSES >= static_cast<size_t>(StatusType::CONFUSED), "a status added to StatusType needs its slot");

// Percent modifiers of the attack damage an entity deals and takes: 100 changes nothing. CombatEngine keeps
// the player's in step with the active powers (CombatEngine::sync_modifiers).
struct DamageModifiers {
    int16_t dealt = 100;                 // its attacks (Shadow Step: 200)
    int16_t taken = 100;                 // the attacks it takes
    int16_t weak = 75;                   // its attacks while it is Weak
    int16_t vulnerable = 150;            // its attacks on a Vulnerable target (Cruelty: 175, 200)
    int16_t dealt_to_weak = 100;         // its attacks on a Weak target (Tracking: 150)
    int16_t taken_from_vulnerable = 100; // the attacks it takes from a Vulnerable attacker (Colossus: 50)
};

class CombatEntity {
public:
    int16_t hp = 0;
    int16_t max_hp = 0;
    int16_t block = 0;
    DamageModifiers modifiers;
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

    // Block from a card: Dexterity is added, then Frail takes a quarter away and `percent` applies (200 doubles
    // the Block gained), rounded down once at the end.
    void gain_block(int16_t amount, int16_t percent = 100) {
        if (amount > 0) {
            int16_t dex = get_status(StatusType::DEXTERITY);
            int32_t total = amount + dex;
            if (total > 0) {
                total = total * (get_status(StatusType::FRAIL) > 0 ? 3 : 4) * std::max<int32_t>(percent, 0) / 400;
            }
            if (total > 0) {
                block = static_cast<int16_t>(std::min<int32_t>(block + total, INT16_MAX));
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
// target is Vulnerable, then the percent modifiers of both sides, rounded down once at the end, and 1 at most
// against an Intangible target.
inline int16_t attack_damage(int16_t base, const CombatEntity& attacker, const CombatEntity& target) {
    int32_t damage = base + attacker.get_status(StatusType::STRENGTH) + attacker.get_status(StatusType::VIGOR);
    if (damage <= 0) return 0;
    // Every factor is multiplied in before the single division. A percent is reduced first (150 % is 3/2), which
    // keeps the product small: the 64-bit division is not worth its cost on the GBA.
    int32_t divisor = 1;
    const auto scale = [&damage, &divisor](int16_t percent) {
        const int32_t factor = std::max<int32_t>(percent, 0);
        const int32_t common = std::gcd(factor, int32_t{ 100 });
        damage *= factor / common;
        divisor *= 100 / common;
    };
    const bool attacker_weak = attacker.get_status(StatusType::WEAK) > 0;
    const bool attacker_vulnerable = attacker.get_status(StatusType::VULNERABLE) > 0;
    const bool target_weak = target.get_status(StatusType::WEAK) > 0;
    const bool target_vulnerable = target.get_status(StatusType::VULNERABLE) > 0;
    if (attacker_weak) scale(attacker.modifiers.weak);
    if (target_vulnerable) scale(attacker.modifiers.vulnerable);
    if (target_weak) scale(attacker.modifiers.dealt_to_weak);
    if (attacker_vulnerable) scale(target.modifiers.taken_from_vulnerable);
    scale(attacker.modifiers.dealt);
    scale(target.modifiers.taken);
    damage /= divisor;
    if (damage > 1 && target.get_status(StatusType::INTANGIBLE) > 0) damage = 1;
    return static_cast<int16_t>(std::min<int32_t>(damage, INT16_MAX));
}

enum class EnemyIntentType : uint8_t {
    ATTACK,
    BUFF,
    DEFEND,
    ATTACK_DEFEND
};

struct EnemyIntent {
    EnemyIntentType type = EnemyIntentType::ATTACK;
    int16_t base_damage = 0;
    int16_t block = 0;
    StatusType status_type = StatusType::NONE;
    int16_t status_amount = 0;
    std::string_view description = "";
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
    SEWER_CLAM
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

    void init(EnemyType enemy_type);
    // Damage the current intent would deal to `target` right now (0 when it does not attack).
    int16_t intent_damage(const CombatEntity& target) const;
    void plan_turn(int turn_number);
    // Plays the current intent. An attack is answered by the player's Thorns, plus `damage_back` (Flame Barrier),
    // plus the damage their Block absorbed when `reflect` is set (Reflect).
    void execute_turn(Player& player, int16_t damage_back = 0, bool reflect = false);
};

} // namespace engine
