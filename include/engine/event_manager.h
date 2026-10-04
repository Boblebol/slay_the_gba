#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string_view>
#include "engine/card.h"
#include "engine/encounter.h"
#include "engine/fixed_vector.h"
#include "engine/map.h"

namespace engine {

class RunManager;

// What a choice needs before it can be taken. `EventChoice::condition_value` is the amount compared.
enum class EventCondition : uint8_t {
    NONE = 0,
    GOLD_AT_LEAST,    // gold >= value
    HP_ABOVE,         // HP > value: a choice that costs HP can refuse the heroes it would leave at 1 HP
    MAX_HP_ABOVE,     // Max HP > value
    DECK_AT_LEAST,    // cards in the deck >= value: something to remove or to transform
    UPGRADABLE_CARD,  // a card of the deck is not upgraded yet
    FREE_POTION_SLOT  // room in the belt
};

// The fixed set of things a choice can do. An outcome is one line of an event's table.
enum class EventOutcomeType : uint8_t {
    NONE = 0,
    GAIN_GOLD,
    LOSE_GOLD,      // never below 0
    HEAL,           // never above Max HP
    LOSE_HP,        // never below 1: an event does not kill
    GAIN_MAX_HP,    // HP rise by the same amount
    LOSE_MAX_HP,    // never below 1, HP follow
    GAIN_RELIC,     // `item`, or a random relic with an implemented effect when `item` is empty
    GAIN_POTION,    // `item` (a Fire Potion when empty: the engine has no pool of potions to draw from yet);
                    // lost when the belt is full
    GAIN_CARD,      // `card`, or a random card of the hero's reward pool when `card` is NONE
    REMOVE_CARD,    // `value` cards
    UPGRADE_CARD,   // `value` cards among the ones not upgraded yet
    TRANSFORM_CARD, // `value` cards, each replaced with a random card of the hero's reward pool
    FIGHT           // a fight of the current act, started once every other outcome is applied
};

// Which card of the deck a card outcome acts on. There is no card picker in the event scene yet.
enum class EventPick : uint8_t {
    RANDOM = 0,
    FIRST // first card of the deck that qualifies
};

// Room tier of a FIGHT outcome (`EventOutcome::value`).
constexpr int16_t EVENT_FIGHT_MONSTER = 0;
constexpr int16_t EVENT_FIGHT_ELITE = 1;

struct EventOutcome {
    EventOutcomeType type = EventOutcomeType::NONE;
    int16_t value = 0;         // amount, number of cards, or tier of the fight
    int16_t spread = 0;        // the amount is `value` plus a random 0..spread
    std::string_view item {};  // relic or potion id
    CardId card = CardId::NONE;
    EventPick pick = EventPick::RANDOM;
};

// A short constant list: the tables are `constexpr`, so they stay in ROM.
template<typename T, size_t Capacity>
class EventList {
public:
    constexpr EventList() = default;
    constexpr EventList(std::initializer_list<T> values) {
        for (const T& value : values) {
            items_[count_++] = value; // one too many is a compile error in a constexpr table
        }
    }

    [[nodiscard]] constexpr size_t size() const { return count_; }
    [[nodiscard]] constexpr bool empty() const { return count_ == 0; }
    constexpr const T& operator[](size_t index) const { return items_[index]; }
    constexpr const T* begin() const { return items_; }
    constexpr const T* end() const { return items_ + count_; }

private:
    T items_[Capacity] {};
    uint8_t count_ = 0;
};

constexpr size_t EVENT_MAX_CHOICES = 4;
constexpr size_t EVENT_MAX_OUTCOMES = 4;

struct EventChoice {
    std::string_view label;
    std::string_view cost_benefit;
    EventList<EventOutcome, EVENT_MAX_OUTCOMES> outcomes {};
    std::string_view result_message;
    EventCondition condition = EventCondition::NONE;
    int16_t condition_value = 0;
};

// Bits of `EventEncounter::zones`, one per ActZone.
constexpr uint8_t zone_bit(ActZone zone) { return static_cast<uint8_t>(1u << static_cast<uint8_t>(zone)); }
constexpr uint8_t EVENT_ZONE_ACT1A = zone_bit(ActZone::ACT1A);
constexpr uint8_t EVENT_ZONE_ACT1B = zone_bit(ActZone::ACT1B);
constexpr uint8_t EVENT_ZONE_ACT2 = zone_bit(ActZone::ACT2);
constexpr uint8_t EVENT_ZONE_ACT3 = zone_bit(ActZone::ACT3);
constexpr uint8_t EVENT_ZONE_ALL = EVENT_ZONE_ACT1A | EVENT_ZONE_ACT1B | EVENT_ZONE_ACT2 | EVENT_ZONE_ACT3;

struct EventEncounter {
    std::string_view id;
    std::string_view title;
    std::string_view dialogue;
    uint8_t zones = EVENT_ZONE_ALL;
    EventList<EventChoice, EVENT_MAX_CHOICES> choices {};
};

// One thing a choice did, for the screen that tells it to the player.
struct EventEffect {
    EventOutcomeType type = EventOutcomeType::NONE;
    int16_t amount = 0;       // what really changed: a heal at full HP reports 0
    std::string_view name {}; // French name of the relic, the potion or the card
};

struct EventResult {
    FixedVector<EventEffect, 6> effects;
    bool fight = false; // the choice started a fight: the run is IN_COMBAT
};

class EventManager {
public:
    // Every event, in ROM.
    static std::span<const EventEncounter> all();
    static const EventEncounter* find(std::string_view id);

    // The event of an event room. It only depends on the act, on the seed of its map and on the floor, so a
    // resumed run finds the same event and nothing has to be saved. The events of the zone come first, each
    // once, in an order drawn from the seed; a zone with fewer events than the map has event floors goes on
    // with the events of the other zones rather than showing one twice.
    static const EventEncounter& pick(const MapGraph& map, uint8_t act, uint32_t map_seed, uint8_t floor);

    static bool choice_available(const RunManager& run, const EventChoice& choice);
    // Applies every outcome of `choice`. Refused (false, run untouched) when its condition is not met.
    // Random outcomes are drawn from `seed`: same seed, same result.
    static bool apply_choice(RunManager& run, const EventChoice& choice, char* out_msg, size_t max_len,
                             EventResult* result = nullptr, uint32_t seed = 0);
};

} // namespace engine
