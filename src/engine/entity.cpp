#include "engine/entity.h"

namespace engine {

void Enemy::init(EnemyType enemy_type) {
    type = enemy_type;
    statuses.clear();
    block = 0;

    if (type == EnemyType::CULTIST) {
        name = "Cultist";
        max_hp = 50;
        hp = 50;
    } else {
        name = "Jaw Worm";
        max_hp = 42;
        hp = 42;
    }

    plan_turn(1);
}

void Enemy::plan_turn(int turn_number) {
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
    } else {
        // Jaw worm
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
    }
}

void Enemy::execute_turn(Player& player) {
    clear_block();

    // Gain block from intent if any
    if (current_intent.block > 0) {
        gain_block(current_intent.block);
    }

    // Apply status buff to self if any
    if (current_intent.status_type != StatusType::NONE && current_intent.status_amount > 0) {
        add_status(current_intent.status_type, current_intent.status_amount);
    }

    // Execute attack if any
    if (current_intent.base_damage > 0) {
        int16_t dmg = current_intent.base_damage;
        
        // Add Strength
        dmg += get_status(StatusType::STRENGTH);

        // Check Weak on enemy (-25%)
        if (get_status(StatusType::WEAK) > 0) {
            dmg = (dmg * 3) / 4;
        }

        // Check Vulnerable on player (+50%)
        if (player.get_status(StatusType::VULNERABLE) > 0) {
            dmg = (dmg * 3) / 2;
        }

        if (dmg < 0) dmg = 0;
        player.take_damage(dmg);
    }

    // End of turn status triggers (Ritual -> Strength)
    int16_t ritual = get_status(StatusType::RITUAL);
    if (ritual > 0) {
        add_status(StatusType::STRENGTH, ritual);
    }

    // Tick end-of-turn status counters (Vulnerable, Weak)
    tick_end_of_turn_statuses();
}

} // namespace engine
