#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "engine/event_script.h"
#include "engine/fixed_vector.h"

namespace engine {

class RunManager;

// An option of the page on screen, as the scenes show it.
struct EventChoice {
    std::string_view label;
    std::string_view cost_benefit;
    std::string_view result_message;
    bool enabled = true; // false while the run does not meet the option's condition
};

// The event in progress: the page on screen and its options.
struct EventEncounter {
    std::string_view id;
    std::string_view title;
    std::string_view dialogue; // text of the page on screen
    FixedVector<EventChoice, MAX_EVENT_OPTIONS> choices;
    const EventDef* def = nullptr;
    uint8_t page = 0;
};

// Reads the event tables of event_script.h and applies them to the run (see event_script.h to add an event).
class EventManager {
public:
    // One slot per known event, in a fixed order: the place of an event in it is its bit in
    // RunManager::events_seen, so it never moves.
    static std::span<const EventSlot> slots();
    static const EventDef* find(std::string_view id);
    // Place of `id` in slots(), -1 when unknown.
    static int slot_index(std::string_view id);
    static bool meets(const RunManager& run, const EventCondition& condition);
    // An event of the current act's zone the run can meet, drawn with `seed`, never one of `seen` while another
    // one is left. -1 when no event fits.
    static int draw(const RunManager& run, uint64_t seen, uint32_t seed);
    // Shows page `page` of `def`.
    static void open_page(RunManager& run, const EventDef& def, uint8_t page);
    // Applies the option at `index` of the page on screen; false for a wrong or disabled option, or outside an
    // event. The run is then back on the map, on the next page (EVENT), in a fight (IN_COMBAT) or on a reward
    // screen.
    static bool choose(RunManager& run, int index, char* out_msg, size_t max_len);
    static void apply_effect(RunManager& run, const EventEffect& effect);
};

} // namespace engine
