#pragma once

#include <cstdint>
#include <string_view>
#include "engine/card.h"
#include "engine/entity.h"
#include "engine/fixed_vector.h"
#include "engine/character.h"

namespace engine {

enum class CombatState : uint8_t {
    PLAYER_TURN,
    ENEMY_TURN,
    VICTORY,
    DEFEAT
};

// What one enemy did during the last enemy turn, so the presentation layer can replay it step by step.
struct EnemyActionRecord {
    int8_t enemy_index = -1;
    EnemyIntentType intent_type = EnemyIntentType::ATTACK;
    int16_t damage_to_player = 0; // HP actually lost by the player
    int16_t damage_blocked = 0;   // Absorbed by the player's block or minion
    int16_t block_gained = 0;
    int16_t player_hp_after = 0;
};

class CombatEngine {
public:
    Player player;
    Enemy enemy; // Primary enemy (kept in sync with enemies[0])
    FixedVector<Enemy, 3> enemies; // Multi-enemy field (Yu-Gi-Oh / Slay style: 1 to 3 enemies)
    int selected_target_index = 0;
    CharacterId current_character = CharacterId::IRONCLAD;
    
    FixedVector<DeckCard, 50> draw_pile;
    FixedVector<DeckCard, 10> hand;
    FixedVector<DeckCard, 50> discard_pile;
    FixedVector<DeckCard, 50> exhaust_pile;

    FixedVector<EnemyActionRecord, 3> last_enemy_actions;

    CombatState state = CombatState::PLAYER_TURN;
    int turn_number = 1;
    int selected_card_index = 0;
    
    // Simple notification message
    char combat_log[48] = {0};

    void init_starter_deck(CharacterId character = CharacterId::IRONCLAD);
    void start_combat(EnemyType enemy_type, CharacterId character = CharacterId::IRONCLAD);
    void start_combat_multi(FixedVector<EnemyType, 3> enemy_types, CharacterId character = CharacterId::IRONCLAD);
    void start_player_turn();
    void draw_card();
    void draw_cards(int count);
    void shuffle_discard_into_draw();
    
    bool can_play_card(int index) const;
    bool play_card(int index, int target_index = -1);
    void end_player_turn();
    void execute_enemy_turn();

    // Applies a potion's effect during the player's turn. False when it can't be drunk (unknown potion, enemy turn).
    bool use_potion(std::string_view potion_id, int target_index = -1);

    bool has_alive_enemies() const;
    int count_alive_enemies() const;
    int get_first_alive_enemy_index() const;

    void shuffle_draw_pile();
    void set_log(std::string_view msg);

private:
    uint32_t rng_state_ = 123456789;
    uint32_t next_random();
};

} // namespace engine
