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

// Tallies read by card effects. The turn ones restart with each player turn, the combat ones with the fight.
struct CombatCounters {
    // This turn
    uint8_t attacks_played = 0;
    uint8_t skills_played = 0;
    uint8_t powers_played = 0;
    uint8_t cards_drawn = 0;     // drawn by effects, after the turn's opening draw
    uint8_t cards_discarded = 0; // discarded by effects, not by the end of the turn
    uint8_t energy_spent = 0;
    uint8_t hits_dealt = 0;      // attack hits on enemies
    uint8_t stars_gained = 0;
    uint8_t stars_spent = 0;
    int16_t hp_lost = 0;
    // This combat
    uint8_t combat_cards_played = 0;
    uint8_t combat_cards_drawn = 0;
    uint8_t combat_cards_created = 0;
    uint8_t combat_ethereal_played = 0;
    uint8_t combat_hp_losses = 0; // number of times the player lost HP

    uint8_t cards_played() const { return static_cast<uint8_t>(attacks_played + skills_played + powers_played); }
    void start_turn() {
        attacks_played = skills_played = powers_played = 0;
        cards_drawn = cards_discarded = energy_spent = hits_dealt = 0;
        stars_gained = stars_spent = 0;
        hp_lost = 0;
    }
};

// Effects scheduled for a later turn.
enum class DelayedType : uint8_t {
    ENERGY,            // start of turn
    BLOCK,             // start of turn, not modified by Dexterity
    DRAW,              // start of turn, with the opening draw
    STARS,             // start of turn
    SUMMON,            // start of turn
    CHANNEL_LIGHTNING, // start of turn
    DAMAGE_ALL         // end of turn: damage to every enemy, not modified by Strength (countdown cards)
};

struct DelayedEffect {
    DelayedType type = DelayedType::ENERGY;
    int16_t amount = 0;
    uint8_t delay = 1;  // turns before it first triggers: 1 = the next start of turn, or this end of turn
    uint8_t repeat = 1; // consecutive turns it triggers
};

// Why a hand card cannot be played right now.
enum class PlayBlocker : uint8_t {
    NONE,
    NOT_PLAYABLE, // not the player's turn, or no such card
    ENERGY,
    STARS,
    CONDITION,    // the card's own PlayCondition
    PLAY_FIRST,   // another card in hand must be played first
    PLAY_LIMIT    // a card in hand caps the number of cards played this turn
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
    uint8_t exhausted_this_turn = 0; // cards exhausted since the start of the player's turn
    FixedVector<DeckCard, 10> returning_cards; // played cards coming back to the hand next turn (KEYWORD_RETURN)
    CombatCounters counters;
    FixedVector<DelayedEffect, 12> delayed_effects;
    uint8_t keep_block_turns = 0;  // starts of turn where the player's Block is not removed (Blur)
    bool retain_hand = false;      // the whole hand stays at the end of this turn
    bool no_draw_this_turn = false; // "you cannot draw any more cards this turn"
    uint8_t fatal_kills = 0;       // enemies killed by the hits of the card being played (minions do not count)
    int8_t temp_strength = 0;      // Strength and Dexterity gained until the end of the turn
    int8_t temp_dexterity = 0;

    FixedVector<EnemyActionRecord, 3> last_enemy_actions;

    CombatState state = CombatState::PLAYER_TURN;
    int turn_number = 1;
    int selected_card_index = 0;
    
    // Simple notification message
    char combat_log[48] = {0};

    void init_starter_deck(CharacterId character = CharacterId::IRONCLAD);
    void start_combat(EnemyType enemy_type, CharacterId character = CharacterId::IRONCLAD);
    void start_combat_multi(FixedVector<EnemyType, 3> enemy_types, CharacterId character = CharacterId::IRONCLAD);
    // Opens the fight once the draw pile is ready: clears the combat state, puts the Innate cards on top of the
    // draw pile and starts the first turn.
    void open_first_turn();
    void start_player_turn();
    // Draws one card, reshuffling the discard pile when needed. False when nothing was drawn (full hand, empty piles).
    bool draw_card();
    void draw_cards(int count);
    void shuffle_discard_into_draw();
    // Removes the card from the combat: it goes to the exhaust pile.
    void exhaust_card(DeckCard card);
    // Exhausts the hand card at `index` and returns it.
    DeckCard exhaust_from_hand(size_t index);
    // Discards the hand card at `index` because of an effect (counted, unlike the end of turn discard).
    void discard_from_hand(size_t index);
    // Adds a card made during the combat to the discard pile.
    void create_card_in_discard(DeckCard card);

    // One attack hit on an enemy: Strength, Weak and Vulnerable applied. Returns the HP lost by the enemy.
    int16_t hit(Enemy& target, int16_t base);
    // Stars gained or spent by the Regent: every change goes through these two.
    void gain_stars(int amount);
    void spend_stars(int amount);
    // The player loses HP: Block does not absorb it.
    void lose_hp(int16_t amount);
    // Strength or Dexterity until the end of the turn.
    void gain_temporary(StatusType type, int8_t amount);
    // The enemy loses Strength until the end of its turn.
    void sap_strength(Enemy& target, int16_t amount);
    // Schedules an effect: see DelayedEffect.
    void schedule(DelayedType type, int16_t amount, uint8_t delay = 1, uint8_t repeat = 1);
    
    PlayBlocker play_blocker(int index) const;
    bool can_play_card(int index) const { return play_blocker(index) == PlayBlocker::NONE; }
    bool play_condition_met(PlayCondition condition) const;
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
    // Index of a living enemy picked at random; the first enemy when none is alive.
    int random_alive_enemy_index();
    void note_player_hp_lost(int16_t amount);
    // Triggers the scheduled effects of the start (or the end) of the turn. Returns the extra cards to draw.
    int run_delayed_effects(bool end_of_turn);
};

} // namespace engine
