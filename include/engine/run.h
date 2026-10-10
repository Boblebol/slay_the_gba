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

// What the player is asked to do with cards of the deck, outside a combat (a relic just obtained).
enum class DeckChoiceKind : uint8_t {
    NONE,
    UPGRADE,
    REMOVE,
    TRANSFORM
};

struct DeckChoice {
    DeckChoiceKind kind = DeckChoiceKind::NONE;
    uint8_t remaining = 0;  // cards still to choose
    bool optional = false;  // "up to N": the choice can be left before the last card
    bool upgrade_result = false; // TRANSFORM only: the new card comes upgraded
    CardId transform_into = CardId::NONE; // TRANSFORM only: fixed result card (NONE = random from the reward pool)
};

// Moments of a run, outside the fights, a relic can act on. Each one is a private `on_...` method of RunManager.
enum class RunHook : uint8_t {
    ROOM_ENTERED,      // any room, before it is resolved
    RESTED,            // the Rest action of a campfire
    SHOP_OPENED,       // the stock of the merchant is drawn
    COMBAT_WON,        // a fight is over, before its rewards are drawn
    REWARDS_GENERATED, // the rewards of a fight are drawn, before they are shown
    CARD_ADDED,        // a card joins the deck (reward, merchant, event, relic)
    GOLD_GAINED,       // gold is received (never when it is spent)
    COUNT
};

// What can be done at a campfire. Rest and Smith are always there, the others are given by a relic.
enum class CampfireAction : uint8_t {
    REST,
    SMITH,
    DIG,  // Shovel
    LIFT, // Girya
    COOK  // Meat Cleaver
};

// A number a relic keeps from room to room ("every 3 combats", "3 times max"), saved with the run.
struct RelicCounter {
    std::string_view relic_id;
    uint8_t value = 0;
};

constexpr size_t MAX_RELIC_COUNTERS = 8;

// Slots of the potion belt: at the start of a run, and the most relics can bring it to.
constexpr uint8_t BASE_POTION_SLOTS = 3;
constexpr uint8_t MAX_POTION_SLOTS = 8;

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
    FixedVector<std::string_view, MAX_POTION_SLOTS> potions;
    bool belt_buckle_active = false; // the 2 Dexterity of Belt Buckle are on the player
    uint8_t potion_slots = BASE_POTION_SLOTS; // size of the belt: `potions` never holds more
    // Cards of the deck the player still has to choose. The room is left once it is resolved.
    DeckChoice deck_choice;
    FixedVector<RelicCounter, MAX_RELIC_COUNTERS> relic_counters;
    // How many times each hook ran since the start of the run: read by the tests and the debug tools.
    uint16_t hook_runs[static_cast<size_t>(RunHook::COUNT)] = {};
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
    // Paël's Tooth: cards devoured from the deck, returned one per won combat (upgraded).
    FixedVector<CardId, 5> paels_tooth_cards;
    bool preserved_fog_pending = false; // Hubris is added once the 3 removals are done
    // Meat Cleaver: the +5 Max HP of Cook come once the removals are done. Not saved: the autosave is written on the
    // map, after the choice has ended.
    bool cook_pending = false;
    uint8_t paels_tooth_picks = 0; // devour picks still to make
    PaletteVariant palette = PaletteVariant::CLASSIC;

    EventEncounter current_event;
    std::string_view current_encounter; // id of the encounter of the last fight entered (see encounter.h)

    void start_run(CharacterId char_id, uint8_t ascension = 0, PaletteVariant pal = PaletteVariant::CLASSIC, uint32_t seed = 42);
    FixedVector<int, 3> get_available_rooms() const;
    void enter_room(int node_index);
    void trigger_event();
    bool choose_event_choice(int choice_index, char* out_msg = nullptr, size_t max_len = 0);

    // Campfire actions
    // Rest, Smith, then the actions the relics of the run give, in the order of the menu.
    [[nodiscard]] FixedVector<CampfireAction, 5> campfire_actions() const;
    void rest_at_campfire();
    bool can_rest() const;
    bool smith_at_campfire(int card_index);
    bool can_smith() const;
    // Meat Cleaver: asks for 2 cards of the deck to remove, then gives +5 Max HP and uses up the rest site. With fewer
    // than 2 removable cards it removes what it can and still gives the Max HP. False without the relic.
    bool cook_at_campfire();

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
    // Regenerates the current act's map from its seed; the Golden Compass narrows Act 2 to a single path.
    void regenerate_map();
    // Engine-supported cards a hero can be offered after a combat.
    static std::span<const CardId> card_reward_pool(CharacterId char_id);
    void pick_card_reward(int reward_index);
    void skip_card_rewards();
    // Takes the potion of the reward screen. With a full belt, `replace_index` is the potion thrown away to make
    // room; without it the potion stays on offer.
    bool take_potion_reward(int replace_index = -1);
    // Takes the relic of the reward screen.
    bool take_relic_reward();
    // Stores the relic, then resolves what it does when obtained.
    void add_relic(std::string_view relic_id);
    bool has_relic(std::string_view relic_id) const;

    // Effects a relic has when obtained. Each one is also usable on its own.
    // Raises the maximum HP, and the current HP by as much.
    void gain_max_hp(int16_t amount);
    void lose_max_hp(int16_t amount);
    void heal(int16_t amount);
    void gain_gold(int16_t amount);
    // Upgrades up to `count` cards drawn at random among the ones not upgraded yet, of `type` only when given.
    // Returns how many were upgraded.
    int upgrade_random_cards(int count);
    int upgrade_random_cards(int count, CardType type);
    // Adds a card (a curse like any other) to the deck. False when the deck is full.
    bool add_card_to_deck(DeckCard card);
    // Replaces the card by another one of the hero's reward pool, drawn at random and never the same card.
    bool transform_card_in_deck(int deck_index, bool upgraded = false, CardId into = CardId::NONE);
    // Obtains up to `count` relics drawn like the ones of a chest. Returns how many were obtained.
    int gain_random_relics(int count);
    // Fills up to `count` free slots of the belt with random potions. Returns how many were obtained.
    int gain_random_potions(int count);
    void add_potion_slots(int count);
    [[nodiscard]] bool potion_belt_full() const { return potions.size() >= potion_slots; }

    // Counter of a relic: 0 until it is first written.
    [[nodiscard]] uint8_t relic_counter(std::string_view relic_id) const;
    // False when the run already keeps MAX_RELIC_COUNTERS counters.
    bool set_relic_counter(std::string_view relic_id, uint8_t value);
    // Adds 1 to the counter. On every `every`-th call it goes back to 0 and the result is true.
    bool tick_relic_counter(std::string_view relic_id, uint8_t every);
    [[nodiscard]] uint16_t hook_count(RunHook hook) const { return hook_runs[static_cast<size_t>(hook)]; }

    // Asks the player for `count` cards of the deck to upgrade, remove or transform. Nothing is asked when no
    // card of the deck fits.
    void request_deck_choice(DeckChoiceKind kind, int count, bool optional = false, bool upgrade_result = false, CardId transform_into = CardId::NONE);
    [[nodiscard]] bool has_deck_choice() const { return deck_choice.kind != DeckChoiceKind::NONE; }
    // True when the card at `deck_index` can be picked for the choice in progress.
    [[nodiscard]] bool deck_choice_allows(int deck_index) const;
    // Applies the choice in progress to the card. The choice ends with its last card, or when no card fits.
    bool resolve_deck_choice(int deck_index);
    // Adds Hubris once the Preserved Fog removals are over.
    void finish_preserved_fog();
    // Gives the Max HP of Cook once its removals are over.
    void finish_cook();
    // Leaves an optional choice. False when the choice has to be made.
    bool skip_deck_choice();
    std::string_view draw_treasure_relic();
    // Relics with an implemented effect: the ones a chest can hold.
    static std::span<const std::string_view> active_relics();
    bool remove_card_from_deck(int deck_index);
    bool upgrade_card_in_deck(int deck_index);

    // Act Progression
    // Relics that act during the fight become powers of the combat. Called before the first turn.
    void register_relic_powers();
    void apply_combat_start_relics();
    // Keeps the Dexterity of Belt Buckle in line with the potion belt during a fight.
    void refresh_belt_buckle();
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
    // What a relic does when obtained.
    void on_relic_obtained(std::string_view relic_id);
    // Hooks of the relics that act outside the fights (see RunHook). A relic is added to the hook of its
    // trigger with a literal `has_relic("ID")`.
    void on_room_entered(RoomType room);
    void on_rested();
    void on_shop_opened();
    void on_combat_won(RoomType room);
    void on_rewards_generated(RoomType room);
    void on_card_added(DeckCard& card);
    void on_gold_gained(int16_t amount);
    void count_hook(RunHook hook);
    int upgrade_random_cards(int count, bool any_type, CardType type);
};

} // namespace engine
