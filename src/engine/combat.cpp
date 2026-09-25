#include "engine/combat.h"
#include <cstring>

namespace engine {

uint32_t CombatEngine::next_random() {
    // 32-bit XorShift
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 17;
    rng_state_ ^= rng_state_ << 5;
    return rng_state_;
}

void CombatEngine::set_log(std::string_view msg) {
    size_t i = 0;
    for (; i < msg.size() && i + 1 < sizeof(combat_log); ++i) {
        combat_log[i] = msg[i];
    }
    combat_log[i] = '\0';
}

void CombatEngine::init_starter_deck() {
    draw_pile.clear();
    hand.clear();
    discard_pile.clear();
    exhaust_pile.clear();

    // Standard Ironclad Starter: 5 Strike, 4 Defend, 1 Bash
    for (int i = 0; i < 5; ++i) draw_pile.push_back(CardId::STRIKE);
    for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::DEFEND);
    draw_pile.push_back(CardId::BASH);

    shuffle_draw_pile();
}

void CombatEngine::shuffle_draw_pile() {
    if (draw_pile.size() <= 1) return;
    for (size_t i = draw_pile.size() - 1; i > 0; --i) {
        size_t j = next_random() % (i + 1);
        CardId temp = draw_pile[i];
        draw_pile[i] = draw_pile[j];
        draw_pile[j] = temp;
    }
}

void CombatEngine::shuffle_discard_into_draw() {
    while (!discard_pile.empty()) {
        draw_pile.push_back(discard_pile.back());
        discard_pile.pop_back();
    }
    shuffle_draw_pile();
}

void CombatEngine::draw_card() {
    if (hand.size() >= hand.capacity()) return; // Hand limit

    if (draw_pile.empty()) {
        shuffle_discard_into_draw();
    }

    if (!draw_pile.empty()) {
        CardId card = draw_pile.back();
        draw_pile.pop_back();
        hand.push_back(card);
    }
}

void CombatEngine::draw_cards(int count) {
    for (int i = 0; i < count; ++i) {
        draw_card();
    }
}

void CombatEngine::start_combat(EnemyType enemy_type) {
    player.init();
    enemy.init(enemy_type);
    turn_number = 1;
    selected_card_index = 0;
    init_starter_deck();
    state = CombatState::PLAYER_TURN;
    
    start_player_turn();
    set_log("Combat started!");
}

void CombatEngine::start_player_turn() {
    state = CombatState::PLAYER_TURN;
    player.reset_turn();
    draw_cards(5);
    selected_card_index = 0;
}

bool CombatEngine::can_play_card(int index) const {
    if (state != CombatState::PLAYER_TURN) return false;
    if (index < 0 || static_cast<size_t>(index) >= hand.size()) return false;

    const CardDef& def = CardDef::get(hand[index]);
    return player.energy >= def.cost;
}

bool CombatEngine::play_card(int index) {
    if (!can_play_card(index)) return false;

    CardId card_id = hand[index];
    const CardDef& def = CardDef::get(card_id);

    // Pay energy
    player.energy -= def.cost;

    // Remove from hand
    hand.erase(index);

    // Apply effects
    switch (card_id) {
        case CardId::STRIKE: {
            int16_t dmg = def.base_value + player.get_status(StatusType::STRENGTH);
            if (player.get_status(StatusType::WEAK) > 0) dmg = (dmg * 3) / 4;
            if (enemy.get_status(StatusType::VULNERABLE) > 0) dmg = (dmg * 3) / 2;
            enemy.take_damage(dmg);
            set_log("Played Strike!");
            break;
        }
        case CardId::DEFEND: {
            player.gain_block(def.base_value);
            set_log("Played Defend!");
            break;
        }
        case CardId::BASH: {
            int16_t dmg = def.base_value + player.get_status(StatusType::STRENGTH);
            if (player.get_status(StatusType::WEAK) > 0) dmg = (dmg * 3) / 4;
            if (enemy.get_status(StatusType::VULNERABLE) > 0) dmg = (dmg * 3) / 2;
            enemy.take_damage(dmg);
            enemy.add_status(StatusType::VULNERABLE, def.secondary_value);
            set_log("Played Bash!");
            break;
        }
        case CardId::IRON_WAVE: {
            player.gain_block(def.secondary_value);
            int16_t dmg = def.base_value + player.get_status(StatusType::STRENGTH);
            if (player.get_status(StatusType::WEAK) > 0) dmg = (dmg * 3) / 4;
            if (enemy.get_status(StatusType::VULNERABLE) > 0) dmg = (dmg * 3) / 2;
            enemy.take_damage(dmg);
            set_log("Played Iron Wave!");
            break;
        }
        case CardId::CLEAVE: {
            int16_t dmg = def.base_value + player.get_status(StatusType::STRENGTH);
            if (player.get_status(StatusType::WEAK) > 0) dmg = (dmg * 3) / 4;
            if (enemy.get_status(StatusType::VULNERABLE) > 0) dmg = (dmg * 3) / 2;
            enemy.take_damage(dmg);
            set_log("Played Cleave!");
            break;
        }
        case CardId::ANGER: {
            int16_t dmg = def.base_value + player.get_status(StatusType::STRENGTH);
            if (player.get_status(StatusType::WEAK) > 0) dmg = (dmg * 3) / 4;
            if (enemy.get_status(StatusType::VULNERABLE) > 0) dmg = (dmg * 3) / 2;
            enemy.take_damage(dmg);
            discard_pile.push_back(CardId::ANGER); // Extra copy!
            set_log("Played Anger!");
            break;
        }
        default:
            break;
    }

    // Place played card into discard
    discard_pile.push_back(card_id);

    // Adjust selection if out of bounds
    if (selected_card_index >= static_cast<int>(hand.size()) && !hand.empty()) {
        selected_card_index = static_cast<int>(hand.size()) - 1;
    }

    // Check Victory
    if (!enemy.is_alive()) {
        state = CombatState::VICTORY;
        // Burning Blood relic: heal 6 HP
        player.hp += 6;
        if (player.hp > player.max_hp) player.hp = player.max_hp;
        set_log("Victory! (+6 HP Burning Blood)");
    }

    return true;
}

void CombatEngine::end_player_turn() {
    if (state != CombatState::PLAYER_TURN) return;

    // Discard hand
    while (!hand.empty()) {
        discard_pile.push_back(hand.back());
        hand.pop_back();
    }

    player.tick_end_of_turn_statuses();
    state = CombatState::ENEMY_TURN;
    set_log("Enemy Turn...");

    execute_enemy_turn();
}

void CombatEngine::execute_enemy_turn() {
    enemy.execute_turn(player);

    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }

    // Plan next intent
    turn_number++;
    enemy.plan_turn(turn_number);

    // Start player turn again
    start_player_turn();
    set_log("Your Turn!");
}

} // namespace engine
