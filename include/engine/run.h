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
    std::string_view last_treasure_relic; // relic found in the last chest, empty when it held none

    MapGraph map;
    uint32_t map_seed = 0; // seed of the current act's map, saved so a resumed run gets the same map
    int current_node_index = -1;
    RunPhase phase = RunPhase::MAP_NAVIGATION;

    CombatEngine combat;
    FixedVector<DeckCard, 3> current_card_rewards;
    int16_t gold_reward = 0;
    // Potion and relic offered on the reward screen, empty when the fight gave none or once taken.
    std::string_view potion_reward;
    std::string_view relic_reward;

    ShopInventory current_shop;
    uint8_t card_removals_count = 0;
    PaletteVariant palette = PaletteVariant::CLASSIC;

    EventEncounter current_event;
    std::string_view current_encounter; // id of the encounter of the last fight entered (see encounter.h)

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
    // With a full belt, `replace_index` is the potion of the belt thrown away to make room.
    bool buy_shop_potion(int index, int replace_index = -1);
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
    // Takes the potion of the reward screen. With a full belt, `replace_index` is the potion thrown away to make
    // room; without it the potion stays on offer.
    bool take_potion_reward(int replace_index = -1);
    // Takes the relic of the reward screen.
    bool take_relic_reward();
    void add_relic(std::string_view relic_id);
    bool has_relic(std::string_view relic_id) const;
    std::string_view draw_treasure_relic();
    // Relics with an implemented effect: the ones a chest can hold.
    static std::span<const std::string_view> active_relics();
    bool remove_card_from_deck(int deck_index);
    bool upgrade_card_in_deck(int deck_index);

    // Act Progression
    // Relics that act during the fight become powers of the combat. Called before the first turn.
    void register_relic_powers();
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
    // Monsters of a fighting room, drawn from the encounter table of the act and the floor.
    FixedVector<EnemyType, 3> draw_encounter(RoomType room);
    // Seed of the reward stream of the current room (see RewardRng).
    uint32_t reward_seed() const;
    // Leaves the reward screen: the gold is paid, and a boss opens the next act.
    void finish_rewards();
};

} // namespace engine
