#pragma once
#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"
#include "engine/character.h"
#include "engine/card.h"
#include "engine/map.h"
#include "engine/combat.h"
#include "engine/shop.h"
#include "engine/event_manager.h"

namespace engine {

enum class RunPhase : uint8_t {
    MAP_NAVIGATION,
    IN_COMBAT,
    REWARD_SCREEN,
    CAMPFIRE,
    SHOP,
    TREASURE,
    EVENT,
    ANCIENT_BLESSING,
    VICTORY_ACT,
    GAME_OVER
};

struct AscensionModifier {
    uint8_t level;
    std::string_view title;
    std::string_view description;
};

class RunManager {
public:
    CharacterId character = CharacterId::IRONCLAD;
    uint8_t ascension_level = 0; // 0 to 10
    uint8_t current_act = 1;     // 1, 2, 3
    uint8_t current_floor = 1;   // 1 to 16
    int16_t hp = 80;
    int16_t max_hp = 80;
    int16_t gold = 99;

    FixedVector<DeckCard, 50> master_deck;
    FixedVector<std::string_view, 25> relics;
    FixedVector<std::string_view, 3> potions;

    MapGraph map;
    uint32_t map_seed = 0; // seed of the current act's map, saved so a resumed run gets the same map
    int current_node_index = -1;
    RunPhase phase = RunPhase::MAP_NAVIGATION;

    CombatEngine combat;
    FixedVector<DeckCard, 3> current_card_rewards;
    int16_t gold_reward = 0;

    ShopInventory current_shop;
    uint8_t card_removals_count = 0;
    PaletteVariant palette = PaletteVariant::CLASSIC;

    EventEncounter current_event;

    void start_run(CharacterId char_id, uint8_t ascension = 0, PaletteVariant pal = PaletteVariant::CLASSIC, uint32_t seed = 42);
    FixedVector<int, 3> get_available_rooms() const;
    void enter_room(int node_index);
    void trigger_event();
    bool choose_event_choice(int choice_index, char* out_msg = nullptr, size_t max_len = 0);

    // Campfire actions
    void rest_at_campfire();
    bool can_rest() const;
    bool smith_at_campfire(int card_index);
    bool can_smith() const;

    // Shop actions
    void open_shop();
    bool buy_shop_card(int index);
    bool buy_shop_relic(int index);
    bool buy_shop_potion(int index);
    // Drinks the potion at `index` of the belt in the current combat and removes it from the belt.
    bool use_potion(int index, int target_index = -1);
    bool remove_card_at_shop(int deck_index);
    void leave_shop();

    // Rewards & Deck modification
    void generate_rewards();
    // Engine-supported cards a hero can be offered after a combat.
    static std::span<const CardId> card_reward_pool(CharacterId char_id);
    void pick_card_reward(int reward_index);
    void skip_card_rewards();
    void add_relic(std::string_view relic_id);
    bool has_relic(std::string_view relic_id) const;
    std::string_view draw_treasure_relic();
    bool remove_card_from_deck(int deck_index);
    bool upgrade_card_in_deck(int deck_index);

    // Act Progression
    void apply_combat_start_relics();
    void proceed_after_combat();
    void advance_act();
    bool is_run_completed() const;

    // Ascension rules
    static std::span<const AscensionModifier> get_ascension_modifiers();
    // Percentages applied to the enemies of a room: normal monsters from A2 (damage) / A7 (HP), elites from
    // A3 / A8, bosses from A4 / A9.
    int16_t get_enemy_damage_multiplier(RoomType room = RoomType::MONSTER) const;
    int16_t get_enemy_hp_multiplier(RoomType room = RoomType::MONSTER) const;

    // Random generator state, saved and restored with the run.
    [[nodiscard]] uint32_t rng_state() const { return rng_state_; }
    void set_rng_state(uint32_t state) { rng_state_ = state ? state : 12345; }

private:
    uint32_t rng_state_ = 12345;
    uint32_t next_random();
};

} // namespace engine
