#pragma once
// Events described by data (#253): pages of text, options, and the effects each option has on the run. The
// tables are constexpr, so they stay in the ROM on the GBA; EventManager (event_manager.cpp) reads them.
//
// ── Adding an event ─────────────────────────────────────────────────────────────────────────────────────────
// One event is one file, and no list shared with the other events has to change:
//
// 1. Create src/engine/events/<id in lower case>.h (e.g. hungry_for_mushrooms.h for HUNGRY_FOR_MUSHROOMS) on the
//    model of src/engine/events/hungry_for_mushrooms.h. It defines `inline constexpr EventDef <ID>` in the
//    namespace engine::events. The registry (src/engine/event_registry.cpp) already holds one slot for each of
//    the 57 events of bible/data/event_v0.111.0.json and picks the file up with __has_include: there is nothing
//    to register. The choices and effects come from bible/data/event_choices/{a,b,c}.json.
//    - `acts`: the zones it can be met in (EVENT_ACT_1A ... EVENT_ACT_3, the "acts" of the bible). 0: never
//      drawn, only opened by RunManager::start_event (THE_LANTERN_KEY, brought by its quest card).
//    - `pages`: page 0 opens the event. An option with a GOTO_PAGE effect leads to that page; any other option
//      ends the event (the "leave" options have no effect at all).
//    - An effect a relic already has when obtained (Big Mushroom: +20 Max HP) is not repeated in the event:
//      GAIN_RELIC alone gives it.
//    - `condition` (on the event or on an option): the event is not drawn, or the option is shown greyed out,
//      while the run does not meet it (gold to pay, HP to lose...).
//    - At most MAX_EVENT_OPTIONS options per page, and a page text of about 200 characters (5 lines on screen).
// 2. Background (optional): `uv run --with pillow --with numpy python3 -c "import sys; sys.path.insert(0,
//    'scripts'); import build_gba_art as b; b.build_scene_bgs()"` draws graphics/bg_event_<id>.bmp from
//    bible/images/events/<id>.webp for every event file; src/gba/assets.cpp already finds it with
//    __has_include. Without it the scene shows the default background. Keep only the new bmp/json files.
// 3. Test: create tests/events/<id>_test.h with `void test_<id>()` (model: tests/events/
//    hungry_for_mushrooms_test.h). tests/events_test.cpp already calls it through the same __has_include
//    slots. Run `make test UNIT_TESTS=tests/events_test.cpp`.
// 4. An effect the interpreter lacks: add it to EventEffectType and to EventManager::apply_effect
//    (event_manager.cpp), with a test in tests/events_test.cpp. That switch is shared: keep the new case short.
// ────────────────────────────────────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <span>
#include <string_view>

#include "engine/card.h"

namespace engine {

// Zones an event can be met in (bits of EventDef::acts), as in ActZone.
constexpr uint8_t EVENT_ACT_1A = 1 << 0;
constexpr uint8_t EVENT_ACT_1B = 1 << 1;
constexpr uint8_t EVENT_ACT_2 = 1 << 2;
constexpr uint8_t EVENT_ACT_3 = 1 << 3;
constexpr uint8_t EVENT_ALL_ACTS = EVENT_ACT_1A | EVENT_ACT_1B | EVENT_ACT_2 | EVENT_ACT_3;

// Options shown on one page of an event.
constexpr size_t MAX_EVENT_OPTIONS = 4;

enum class EventEffectType : uint8_t {
    GAIN_GOLD,      // amount
    LOSE_GOLD,      // amount, never below 0
    HEAL,           // amount, up to the max HP
    LOSE_HP,        // amount; it can kill (the run then ends on the event)
    GAIN_MAX_HP,    // amount, the HP rise with it
    LOSE_MAX_HP,    // amount
    ADD_CARD,       // `amount` copies (1 when 0) of `card` (or of a card of `card_pool`; a random Colorless card when both
                    // are empty; `amount` random Common cards of the hero when `ref` is "COMMON"), upgraded when `upgraded`
    ADD_CURSE,      // `amount` copies (1 when 0) of `card`, or of a random curse when `card` is NONE
    ADD_RANDOM_CARD, // a non-Basic card of the hero's pool: a Power (`ref` "POWER") or one costing 0 ("ZERO_COST")
    REMOVE_CARD,    // `amount` cards (1 when 0), chosen by the player or drawn at random (`pick`); with `card`, that many
                    // copies of exactly that card (Amalgamator: 2 Defends), no choice
    UPGRADE_CARD,   // same
    TRANSFORM_CARD, // same; the new card is upgraded when `upgraded`
    ENCHANT_CARD,   // same, with `enchantment` of `enchant_amount`
    CARD_REWARD,    // a card reward (1 card out of 3, may be skipped) once the event is left; `amount` of them in a row;
                    // `ref` a hero ("SILENT"...): his cards, "COLORLESS": Colorless cards
    GAIN_RELIC,     // relic `ref`, or one of `relic_pool` not owned, or a random Common one when `ref` is "COMMON", or a random relic (as in a chest) when both empty
    RELIC_REWARD,   // a relic drawn like an elite's, offered on the reward screen once the event is left
    GAIN_POTION,    // potion `ref`, or `amount` random potions (1 when 0) when `ref` is empty
    FIGHT,          // a fight of the act once the event is left: `ref` "ELITE" for an elite one, EVENT_FIGHT_PUNCH_CONSTRUCTS
                    // for the Punch Off one (2 Punch Constructs, a relic reward), else a normal one
    TRADE_POTION,   // the first potion of the belt for an upgraded card of the same rarity, drawn from the hero's pool
    GOTO_PAGE,      // the event goes on with page `amount`; with `enchant_amount` > 1, one of the `enchant_amount` pages
                    // from `amount` on, drawn at random
    LOSE_POTION,    // the potion `ref` leaves the belt (the Foul Potion thrown at the Fake Merchant)
    DOWNGRADE_CARD, // `amount` random upgraded cards (1 when 0) go back to their base version
    DUPLICATE_DECK, // every card of the deck is copied once (as far as the deck holds)
    LEAVE,          // nothing: makes a "leave" option explicit in the data
    CARD_CHOICE     // `amount` cards of the hero's pool (rare ones included) are shown, the player adds 1 to the deck
};

// `ref` of a FIGHT effect: two Punch Constructs instead of the fight of the act, and a relic with the rewards.
constexpr std::string_view EVENT_FIGHT_PUNCH_CONSTRUCTS = "PUNCH_CONSTRUCTS";

// How the cards of REMOVE / UPGRADE / TRANSFORM / ENCHANT are found.
enum class EventPick : uint8_t {
    CHOOSE, // the player picks them (deck choice screen)
    RANDOM
};

struct EventEffect {
    EventEffectType type = EventEffectType::LEAVE;
    int16_t amount = 0;
    std::string_view ref = {};
    CardId card = CardId::NONE;
    EventPick pick = EventPick::CHOOSE;
    bool upgraded = false;
    Enchantment enchantment = Enchantment::NONE;
    uint8_t enchant_amount = 0;
    std::span<const std::string_view> relic_pool = {}; // GAIN_RELIC: one relic drawn at random among these
    std::span<const CardId> card_pool = {};            // ADD_CARD: one card drawn at random among these
};

enum class EventCheck : uint8_t {
    NONE,
    MIN_GOLD,   // gold >= amount
    MIN_HP,     // hp > amount (the HP can be paid without dying)
    HAS_RELIC,  // relic `ref` owned
    NO_RELIC,   // relic `ref` not owned
    MIN_DECK,   // at least `amount` cards in the deck
    HAS_POTION, // at least one potion on the belt, or the potion `ref` when given
    CAN_SOULS_POWER, // a card of the deck has Exhaust, so it can receive Soul's Power
    HAS_CARD    // at least `amount` copies of `card` in the deck
};

struct EventCondition {
    EventCheck check = EventCheck::NONE;
    int16_t amount = 0;
    std::string_view ref = {};
    CardId card = CardId::NONE;
};

struct EventOption {
    std::string_view label;        // one short line
    std::string_view cost_benefit; // what it does, one line under the options
    std::string_view result;       // the text shown once it is chosen
    std::span<const EventEffect> effects = {};
    EventCondition condition = {};
};

struct EventPage {
    std::string_view text;
    std::span<const EventOption> options;
};

struct EventDef {
    std::string_view id; // the id of bible/data/event_v0.111.0.json
    std::string_view title;
    uint8_t acts = EVENT_ALL_ACTS;
    std::span<const EventPage> pages;
    EventCondition condition = {};
};

// One slot of the registry: every known event has one, `def` is null while its file does not exist.
struct EventSlot {
    std::string_view id;
    const EventDef* def;
};

} // namespace engine
