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
                return;
            }
        }
        statuses.push_back({ type, amount });
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
            block += amount;
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
};

class Player : public CombatEntity {
public:
    int8_t energy = 3;
    int8_t max_energy = 3;

    void init() {
        max_hp = 80;
        hp = 80;
        block = 0;
        energy = 3;
        max_energy = 3;
        statuses.clear();
    }

    void reset_turn() {
        block = 0;
        energy = max_energy;
    }
};

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
    JAW_WORM
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
