#pragma once

#include <cstdint>
#include <string_view>
#include "engine/card.h"
#include "engine/entity.h"
#include "engine/fixed_vector.h"

namespace engine {

enum class CombatState : uint8_t {
    PLAYER_TURN,
    ENEMY_TURN,
    VICTORY,
    DEFEAT
};

class CombatEngine {
public:
    Player player;
    Enemy enemy;
    
    FixedVector<CardId, 50> draw_pile;
    FixedVector<CardId, 10> hand;
    FixedVector<CardId, 50> discard_pile;
    FixedVector<CardId, 50> exhaust_pile;

    CombatState state = CombatState::PLAYER_TURN;
    int turn_number = 1;
    int selected_card_index = 0;
    
    // Simple notification message
    char combat_log[48] = {0};

    void init_starter_deck();
    void start_combat(EnemyType enemy_type);
    void start_player_turn();
    void draw_card();
    void draw_cards(int count);
    void shuffle_discard_into_draw();
    
    bool can_play_card(int index) const;
    bool play_card(int index);
    void end_player_turn();
    void execute_enemy_turn();

    void set_log(std::string_view msg);

private:
    uint32_t rng_state_ = 123456789;
    uint32_t next_random();
    void shuffle_draw_pile();
};

} // namespace engine
