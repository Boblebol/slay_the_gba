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
    } else {
        name = "Monstre";
        max_hp = 30;
        hp = 30;
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
        current_intent.status_type = StatusType::NONE;
        current_intent.status_amount = 0;
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
    }
}

void Enemy::execute_turn(Player& player) {
    clear_block();

    // Ritual gained this turn only starts granting Strength at the end of the next one.
    int16_t ritual = get_status(StatusType::RITUAL);

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
        player.take_damage_with_minion(attack_damage(current_intent.base_damage, *this, player));
    }

    // End of turn status triggers (Ritual -> Strength)
    if (ritual > 0) {
        add_status(StatusType::STRENGTH, ritual);
    }

    // Tick end-of-turn status counters (Vulnerable, Weak)
    tick_end_of_turn_statuses();
}

void Player::channel_orb(OrbType type, CombatEntity* enemy) {
    if (type == OrbType::NONE) return;
    if (orbs.size() >= 3) {
        evoke_oldest_orb(enemy);
    }
    orbs.push_back(type);
    if (type == OrbType::DARK && dark_orb_charge == 0) {
        dark_orb_charge = 6;
    }
}

void Player::evoke_oldest_orb(CombatEntity* enemy, int times) {
    if (orbs.empty()) return;
    OrbType oldest = orbs[0];
    orbs.erase(0);

    for (int i = 0; i < times; ++i) {
        switch (oldest) {
            case OrbType::LIGHTNING:
                if (enemy) enemy->take_damage(8);
                break;
            case OrbType::FROST:
                block += 5; // Orb block ignores Dexterity
                break;
            case OrbType::DARK:
                if (enemy) enemy->take_damage(dark_orb_charge > 0 ? dark_orb_charge : 6);
                break;
            case OrbType::PLASMA:
                energy += 2;
                break;
            default:
                break;
        }
    }
    if (oldest == OrbType::DARK) dark_orb_charge = 0;
}

void Player::trigger_passive_orbs(CombatEntity* enemy) {
    for (size_t i = 0; i < orbs.size(); ++i) {
        switch (orbs[i]) {
            case OrbType::LIGHTNING:
                if (enemy) enemy->take_damage(3);
                break;
            case OrbType::FROST:
                block += 2;
                break;
            case OrbType::DARK:
                dark_orb_charge += 6;
                break;
            default:
                break;
        }
    }
}

void Player::trigger_start_of_turn_orbs() {
    for (size_t i = 0; i < orbs.size(); ++i) {
        if (orbs[i] == OrbType::PLASMA) energy += 1;
    }
}

void Player::add_stars(uint8_t count) {
    sovereign_stars = (sovereign_stars + count > 5) ? 5 : (sovereign_stars + count);
}

void Player::add_souls(uint8_t count) {
    souls = (souls + count > 99) ? 99 : (souls + count);
}

void Player::summon_minion(int16_t m_hp, int16_t m_block) {
    minion_hp += m_hp;
    minion_block += m_block;
}

} // namespace engine
