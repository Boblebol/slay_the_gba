#pragma once
#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"
#include "engine/character.h"
#include "engine/card.h"
#include "engine/map.h"
#include "engine/combat.h"
#include "engine/rewards.h"
#include "engine/shop.h"
#include "engine/event_manager.h"
#include "engine/ancient.h"

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
    TRANSFORM,
    ENCHANT,
    DUPLICATE, // the chosen card gets one more copy in the deck
    ADD        // the chosen card of RunManager::deck_choice_offers joins the deck (Sea Glass)
};

struct DeckChoice {
    DeckChoiceKind kind = DeckChoiceKind::NONE;
    uint8_t remaining = 0;  // cards still to choose
    bool optional = false;  // "up to N": the choice can be left before the last card
    bool upgrade_result = false; // TRANSFORM only: the new card comes upgraded
    CardId transform_into = CardId::NONE; // TRANSFORM only: fixed result card (NONE = random from the reward pool)
    Enchantment enchantment = Enchantment::NONE; // ENCHANT only: the enchantment put on each chosen card
    uint8_t enchant_amount = 0;                  // ENCHANT only: its amount
};

// Sea Glass: cards of another hero offered on pickup.
constexpr size_t SEA_GLASS_CARDS = 15;

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
    COOK, // Meat Cleaver
    CLONE, // a card of the deck is enchanted with Clone
    LEAVE, // Miniature Tent: the rest site is left once the player has done what they wanted
    HATCH  // the deck holds a Byrdonis Egg
};

constexpr size_t MAX_CAMPFIRE_ACTIONS = 8;

// A number a relic keeps from room to room ("every 3 combats", "3 times max"), saved with the run.
struct RelicCounter {
    std::string_view relic_id;
    uint8_t value = 0;
};

constexpr size_t MAX_RELIC_COUNTERS = 8;

// Slots of the potion belt: at the start of a run, and the most relics can bring it to.
constexpr uint8_t BASE_POTION_SLOTS = 3;
constexpr uint8_t MAX_POTION_SLOTS = 8;

// A relic an Ancient offers: where it sits in the slots of get_ancient(RunManager::current_ancient).
struct AncientOffer {
    uint8_t slot = 0;
    uint8_t option = 0;
};

// Every Ancient offers 3 relics at most (2 blessings and 1 curse for Neow, 1 per slot or 3 from one slot for the
// others).
constexpr size_t MAX_ANCIENT_OFFERS = 3;

// Lantern Key: the Gold of the special event it unlocks in the next act. GUESSED, not in the reference data.
constexpr int16_t LANTERN_KEY_GOLD = 100;

// Spoils Map: the extra Gold of the room it marks in the next act.
constexpr int16_t SPOILS_MAP_GOLD = 600;

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
    // Run statistics shown on the end screen (#11, #12), saved with the run.
    uint32_t play_frames = 0; // frames spent in the scenes of the run (60 per second on the GBA)
    uint8_t combats_won = 0;  // every fight won, elites and bosses included
    uint8_t elites_won = 0;
    uint8_t bosses_won = 0;

    FixedVector<DeckCard, 50> master_deck;
    FixedVector<std::string_view, 25> relics;
    FixedVector<std::string_view, MAX_POTION_SLOTS> potions;
    bool belt_buckle_active = false; // the 2 Dexterity of Belt Buckle are on the player
    uint8_t potion_slots = BASE_POTION_SLOTS; // size of the belt: `potions` never holds more
    // Cards of the deck the player still has to choose. The room is left once it is resolved.
    DeckChoice deck_choice;
    // ADD only: the cards offered, that the indices of the choice refer to instead of the deck.
    FixedVector<DeckCard, SEA_GLASS_CARDS> deck_choice_offers;
    FixedVector<RelicCounter, MAX_RELIC_COUNTERS> relic_counters;
    // How many times each hook ran since the start of the run: read by the tests and the debug tools.
    uint16_t hook_runs[static_cast<size_t>(RunHook::COUNT)] = {};
    std::string_view last_treasure_relic; // relic found in the last chest, empty when it held none

    MapGraph map;
    uint32_t map_seed = 0; // seed of the current act's map, saved so a resumed run gets the same map
    int current_node_index = -1;
    RunPhase phase = RunPhase::MAP_NAVIGATION;

    CombatEngine combat;
    // Three cards, and a fourth one when a relic adds a card to the reward (Lasting Candy).
    FixedVector<DeckCard, 4> current_card_rewards;
    // Lava Lamp: the card rewards of the fight just won are upgraded (it lost no HP). Set when the fight ends.
    bool card_rewards_upgraded = false;
    int16_t gold_reward = 0;
    uint8_t extra_card_rewards = 0; // card rewards still to offer after this one (The Hunt)
    uint8_t extra_rare_card_rewards = 0; // Rare card rewards still to offer after this one (White Star)
    uint8_t extra_relic_rewards = 0; // relics still to offer once the one shown is taken (Black Star)
    uint8_t extra_other_hero_card_rewards = 0; // card rewards of another hero still to offer after this one (Kaleidoscope)
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
    // Events met in this run, one bit per slot of EventManager::slots(): an act draws the ones it has not shown yet.
    uint64_t events_seen = 0;
    // The Ancient met at the start of the act (phase ANCIENT_BLESSING), AncientId::COUNT when there is none, and
    // the relics it offers. The player takes one of them.
    AncientId current_ancient = AncientId::COUNT;
    FixedVector<AncientOffer, MAX_ANCIENT_OFFERS> ancient_offers;
    std::string_view current_encounter; // id of the encounter of the last fight entered (see encounter.h)

    void start_run(CharacterId char_id, uint8_t ascension = 0, PaletteVariant pal = PaletteVariant::CLASSIC, uint32_t seed = 42);
    FixedVector<int, 3> get_available_rooms() const;
    void enter_room(int node_index);
    // Draws the event of a ? room among the ones of the act not met yet (see EventManager::draw).
    void trigger_event();
    // Opens the event `id` on its first page (phase EVENT) and marks it as met. False when it has no data.
    bool start_event(std::string_view id);
    // Applies an option of the page on screen. The run is then on the map, on the next page of the event (phase
    // EVENT), in a fight or on a reward screen. False for a wrong or greyed-out option.
    bool choose_event_choice(int choice_index, char* out_msg = nullptr, size_t max_len = 0);

    // Campfire actions
    // Rest, Smith, then the actions the relics of the run give, in the order of the menu.
    [[nodiscard]] FixedVector<CampfireAction, MAX_CAMPFIRE_ACTIONS> campfire_actions() const;
    void rest_at_campfire();
    // Miniature Tent: leaves the rest site, whose actions are otherwise offered until each one is done.
    void leave_campfire();
    bool can_rest() const;
    bool smith_at_campfire(int card_index);
    bool can_smith() const;
    // Meat Cleaver: asks for 2 cards of the deck to remove, then gives +5 Max HP and uses up the rest site. With fewer
    // than 2 removable cards it removes what it can and still gives the Max HP. False without the relic.
    bool cook_at_campfire();
    // Girya: gains 1 Strength for every combat of the run, at most 3 times. Uses up the rest site. False without the
    // relic or once the 3 lifts are done.
    bool lift_at_campfire();
    // Shovel: digs for a random relic at the campfire (false when there is none to find).
    bool dig_at_campfire();
    // Clone: adds to the deck a copy of the card at `deck_index`, which must be enchanted with Clone. Uses up the
    // rest site. False when the card cannot be cloned or the deck is full.
    bool clone_at_campfire(int deck_index);
    [[nodiscard]] bool can_clone(int deck_index) const;
    // Byrdonis Egg: the first egg of the deck leaves it and hatches into the Byrdpip relic (kept once only). Uses up
    // the rest site. False without an egg.
    bool hatch_at_campfire();

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
    // Driftwood: the card reward on screen can be drawn again, once per reward.
    [[nodiscard]] bool can_reroll_card_rewards() const;
    bool reroll_card_rewards();
    // Paël's Wing: the card reward on screen is given up to Paël, and every second one gives a random relic. True
    // when this one gave it.
    [[nodiscard]] bool can_sacrifice_card_rewards() const;
    bool sacrifice_card_rewards();
    // Takes the potion of the reward screen. With a full belt, `replace_index` is the potion thrown away to make
    // room; without it the potion stays on offer.
    bool take_potion_reward(int replace_index = -1);
    // Takes the relic of the reward screen.
    bool take_relic_reward();
    // Stores the relic, then resolves what it does when obtained.
    void add_relic(std::string_view relic_id);
    bool has_relic(std::string_view relic_id) const;
    // Fur Coat: one of the combats of the act it marked, where the enemies have 1 HP.
    [[nodiscard]] bool fur_coat_room(int node_index) const;
    // Juzu Bracelet: whether a ? room may hold a regular enemy combat (false with the relic).
    [[nodiscard]] bool question_room_may_fight() const;

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
    // Enchants the card at `deck_index` (mechanic #107). A card holds one enchantment: false when it already has
    // one, or when the enchantment does not fit it (see can_enchant).
    bool enchant_card_in_deck(int deck_index, Enchantment enchantment, uint8_t amount = 0);
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
    void request_deck_choice(DeckChoiceKind kind, int count, bool optional = false, bool upgrade_result = false, CardId transform_into = CardId::NONE,
                              Enchantment enchantment = Enchantment::NONE, uint8_t enchant_amount = 0);
    [[nodiscard]] bool has_deck_choice() const { return deck_choice.kind != DeckChoiceKind::NONE; }
    // True when the card at `deck_index` (of deck_choice_offers for ADD) can be picked for the choice in progress.
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
    // Opens the next act on its own map, then meets its Ancient (phase ANCIENT_BLESSING).
    void advance_act();
    // Spoils Map: the room of the current act's map it marks (the card keeps it as node index + 1 in `bonus`), -1
    // when the deck holds no marked Spoils Map.
    [[nodiscard]] int spoils_map_node() const;
    bool is_run_completed() const;
    // One more frame of play time (stops at the largest count instead of wrapping around).
    void count_play_frame() {
        if (play_frames != UINT32_MAX) ++play_frames;
    }

    // Ancients
    // The Ancient met at the start of `act`, drawn with `seed` among the ones of that act. Darv can come at
    // every act.
    [[nodiscard]] static AncientId draw_ancient(uint8_t act, uint32_t seed);
    // Meets `id`: the phase becomes ANCIENT_BLESSING and each slot of the Ancient offers as many relics as it
    // says, drawn among its options the run does not own and the engine can resolve. Nothing is offered once the
    // relic list is full.
    void meet_ancient(AncientId id);
    // The relic at `index` of ancient_offers.
    [[nodiscard]] const AncientRelic& ancient_offer(int index) const;
    // Takes the relic at `index` of the offer and leaves the Ancient. Its effect can open a card reward
    // (REWARD_SCREEN) or ask for cards of the deck; otherwise the run is back on the map. False for a wrong index.
    bool choose_ancient_relic(int index);
    // Leaves an Ancient that has nothing to offer. False while it offers a relic: one has to be taken.
    bool leave_ancient();

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
    friend class EventManager; // the effects of the events use the draws and the fights below
    // Starts a fight of the act in a room of type `room` (MONSTER, ELITE or BOSS): the enemies are drawn from the
    // encounter tables, with the current HP and the master deck. A Fur Coat room gives them 1 HP.
    void begin_fight(RoomType room, bool fur_coat = false);
    // What follows a campfire action: the map, or with Miniature Tent the rest site again, without that action.
    void end_campfire_action(CampfireAction action);
    uint32_t rng_state_ = 12345;
    uint32_t next_random();
    // Monsters of a fighting room, drawn from the encounter table of the act and the floor.
    FixedVector<EnemyType, MAX_ENEMIES> draw_encounter(RoomType room);
    // Seed of the reward stream of the current room (see RewardRng).
    // What the card reward on screen was drawn with, so that Driftwood draws it again the same way.
    struct CardRewardDraw {
        bool boss = false;
        CardRarity rarity = CardRarity::NONE;
        bool elite = false;
        CharacterId hero = CharacterId::COUNT;
        uint8_t silver_crucible = 0; // relic counters before the draw, which the draw moves
        uint8_t silken_tress = 0;
        bool rerolled = false;
    };
    CardRewardDraw last_card_reward_draw;

    uint32_t reward_seed() const;
    // Leaves the reward screen: the gold is paid, and a boss opens the next act.
    void finish_rewards();
    // `rarity` restricts the offer to one rarity; NONE draws from every card of the pool. `hero` is whose pool the
    // cards come from (COUNT: the run's own hero).
    void draw_card_rewards(bool boss, RewardRng& rng, CardRarity rarity = CardRarity::NONE, bool elite = false,
                           CharacterId hero = CharacterId::COUNT);
    // A hero drawn at random among the ones the run is not played with (Kaleidoscope).
    CharacterId random_other_hero();
    void next_card_reward_or_finish();
    // Lasting Candy: adds to the card reward a Power of the hero's pool it does not offer yet.
    void add_power_card_reward();
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
    // Spoils Map: each copy of the deck marks a room of the act's map just drawn (see spoils_map_node).
    void mark_spoils_map_sites();
    // Lantern Key: a key in the deck when a new act opens (bonus 1) unlocks its special event for the act's first ? room.
    void unlock_lantern_keys();
    int upgrade_random_cards(int count, bool any_type, CardType type);
};

} // namespace engine
