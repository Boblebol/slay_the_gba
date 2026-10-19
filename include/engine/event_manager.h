#pragma once

#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"

namespace engine {

class RunManager;


enum class EventOutcomeType : uint8_t {
    NONE = 0,
    HEAL,
    DAMAGE,
    GOLD,
    UPGRADE_CARD,
    RELIC,
    POTION
};

struct EventChoice {
    std::string_view label;
    std::string_view cost_benefit;
    EventOutcomeType outcome_type;
    int16_t value;
    std::string_view extra_id;
    std::string_view result_message;
};

struct EventEncounter {
    std::string_view id;
    std::string_view title;
    std::string_view dialogue;
    FixedVector<EventChoice, 3> choices;
};

class EventManager {
public:
    // The special event the Lantern Key quest card unlocks in the act after the one it was found in.
    static EventEncounter lantern_key_encounter();
    static EventEncounter get_encounter_for_floor(uint8_t floor, uint32_t seed);
    static bool apply_choice(RunManager& run, const EventChoice& choice, char* out_msg, size_t max_len);
};

} // namespace engine
