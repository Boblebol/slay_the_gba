#include "engine/entity.h"

namespace engine {

void Enemy::init(EnemyType enemy_type) {
    type = enemy_type;
    statuses.clear();
    block = 0;
    damage_percent = 100;

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

void Enemy::plan_turn(int turn_number) {
    // Start from a blank intent: a field set by last turn's move (a Strength buff) must not leak
    // into this one, since execute_turn() applies whatever the intent holds.
    current_intent = EnemyIntent{};
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

    // Apply status buff to self if any
    if (current_intent.status_type != StatusType::NONE && current_intent.status_amount > 0) {
        add_status(current_intent.status_type, current_intent.status_amount);
    }

    // Execute attack if any
    if (current_intent.base_damage > 0) {
        const int16_t block_before = player.block;
        player.take_damage_with_minion(intent_damage(player));
        clear_status(StatusType::VIGOR); // spent on this attack
        // The attacker gets hurt back: the player's Thorns, Flame Barrier, and the damage Reflect sends back.
        int16_t back = static_cast<int16_t>(player.get_status(StatusType::THORNS) + damage_back);
        if (reflect) back = static_cast<int16_t>(back + block_before - player.block);
        take_damage(back);
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

void Player::add_souls(uint8_t count) {
    souls = (souls + count > 99) ? 99 : (souls + count);
}

void Player::summon_minion(int16_t m_hp, int16_t m_block) {
    minion_hp += m_hp;
    minion_block += m_block;
}

} // namespace engine
