#pragma once

#include <cstdint>
#include <string_view>
#include "engine/status.h"
#include "engine/fixed_vector.h"

namespace engine {

constexpr size_t MAX_STATUSES = 6;

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
                // A spent status frees its slot (only MAX_STATUSES fit).
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

    void tick_end_of_turn_statuses() {
        // Decrease vulnerable, weak by 1
        for (size_t i = 0; i < statuses.size();) {
            if (statuses[i].type == StatusType::VULNERABLE || statuses[i].type == StatusType::WEAK) {
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

    void gain_block(int16_t amount) {
        if (amount > 0) {
            int16_t dex = get_status(StatusType::DEXTERITY);
            int16_t total = amount + dex;
            if (total > 0) {
                block += total;
            }
        }
    }

    int16_t apply_poison_tick() {
        int16_t p = get_status(StatusType::POISON);
        if (p > 0) {
            hp -= p;
            if (hp < 0) hp = 0;
            add_status(StatusType::POISON, -1);
            return p;
        }
        return 0;
    }

    void apply_metallicize_tick() {
        int16_t m = get_status(StatusType::METALLICIZE);
        if (m > 0) {
            block += m;
        }
    }

    // Returns damage inflicted to HP
    int16_t take_damage(int16_t amount) {
        if (amount <= 0) return 0;

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

        if (unblocked > 0) {
            hp -= unblocked;
            if (hp < 0) hp = 0;
        }
        return unblocked;
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

    // Defect Orbs (Max 3 slots)
    FixedVector<OrbType, 3> orbs;
    int16_t dark_orb_charge = 0;

    // Regent Sovereign Stars (0 to 5)
    uint8_t sovereign_stars = 0;

    // Necrobinder Souls & Minion (Osty)
    uint8_t souls = 0;
    int16_t minion_hp = 0;
    int16_t minion_block = 0;

    void init() {
        max_hp = 80;
        hp = 80;
        block = 0;
        energy = 3;
        max_energy = 3;
        statuses.clear();
        orbs.clear();
        dark_orb_charge = 0;
        sovereign_stars = 0;
        souls = 0;
        minion_hp = 0;
        minion_block = 0;
    }

    void reset_turn() {
        block = 0;
        energy = max_energy;
    }

    int16_t take_damage_with_minion(int16_t amount) {
        if (minion_hp > 0) {
            if (minion_block > 0) {
                if (minion_block >= amount) {
                    minion_block -= amount;
                    return 0;
                } else {
                    amount -= minion_block;
                    minion_block = 0;
                }
            }
            if (minion_hp >= amount) {
                minion_hp -= amount;
                return 0;
            } else {
                amount -= minion_hp;
                minion_hp = 0;
            }
        }
        return take_damage(amount);
    }

    void channel_orb(OrbType type, CombatEntity* enemy = nullptr);
    // Triggers the oldest orb's evoke effect `times` times (Dualcast: 2), then removes it.
    void evoke_oldest_orb(CombatEntity* enemy = nullptr, int times = 1);
    // End of turn passives: Lightning hits, Frost blocks, Dark charges.
    void trigger_passive_orbs(CombatEntity* enemy = nullptr);
    // Start of turn passive: each Plasma orb gives 1 energy.
    void trigger_start_of_turn_orbs();
    void add_stars(uint8_t count);
    void add_souls(uint8_t count);
    void summon_minion(int16_t m_hp, int16_t m_block = 0);
};

// Attack damage as in Slay the Spire: (base + Strength), x0.75 when the attacker is Weak, x1.5 when the target
// is Vulnerable, rounded down once at the end.
inline int16_t attack_damage(int16_t base, const CombatEntity& attacker, const CombatEntity& target) {
    int32_t damage = base + attacker.get_status(StatusType::STRENGTH);
    if (damage <= 0) return 0;
    damage *= attacker.get_status(StatusType::WEAK) > 0 ? 3 : 4;
    damage *= target.get_status(StatusType::VULNERABLE) > 0 ? 3 : 2;
    return static_cast<int16_t>(damage / 8);
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
    CEREMONIAL_BEAST // Act 1 boss
};

class Enemy : public CombatEntity {
public:
    EnemyType type = EnemyType::CULTIST;
    std::string_view name = "Cultist";
    EnemyIntent current_intent;

    void init(EnemyType enemy_type);
    void plan_turn(int turn_number);
    void execute_turn(Player& player);
};

} // namespace engine
