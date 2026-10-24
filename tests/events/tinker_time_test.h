#pragma once
// Bricolage (TINKER_TIME): three pages (accept, type, rider); every type and rider is walked once.
#include <iostream>
#include "event_test_support.h"

namespace tinker_time_test {

// A run at the rider page of `type` (0 Attack, 1 Power, 2 Skill).
inline void to_riders(event_test::EventRun& r, int type) {
    using engine::RunPhase;
    r.pick(0);
    assert(r.run.phase == RunPhase::EVENT && r.run.current_event.choices.size() == 3);
    r.pick(type);
    assert(r.run.phase == RunPhase::EVENT && r.run.current_event.choices.size() == 3);
}

} // namespace tinker_time_test

inline void test_tinker_time() {
    using namespace event_test;

    const EventDef* def = EventManager::find("TINKER_TIME");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 5 && def->pages[0].options.size() == 1);

    const CardId expected[3] = { CardId::ULTIMATE_STRIKE, CardId::PANACHE, CardId::ULTIMATE_DEFEND };
    for (int type = 0; type < 3; ++type) {
        // Improvement: the card of the type, upgraded.
        EventRun improved("TINKER_TIME");
        const size_t deck = improved.run.master_deck.size();
        tinker_time_test::to_riders(improved, type);
        improved.pick(0);
        assert(improved.run.master_deck.size() == deck + 1 && improved.run.master_deck.back().id == expected[type]);
        assert(improved.run.master_deck.back().upgraded && improved.run.phase == RunPhase::MAP_NAVIGATION);

        // Chaos: a random Colorless card.
        EventRun chaos("TINKER_TIME");
        tinker_time_test::to_riders(chaos, type);
        chaos.pick(1);
        assert(chaos.run.master_deck.size() == deck + 1 && chaos.run.phase == RunPhase::MAP_NAVIGATION);

        // Wisdom: one card out of 3 to add.
        EventRun wisdom("TINKER_TIME");
        tinker_time_test::to_riders(wisdom, type);
        wisdom.pick(2);
        assert(wisdom.run.has_deck_choice() && wisdom.run.deck_choice.kind == DeckChoiceKind::ADD);
        assert(wisdom.run.deck_choice_offers.size() == 3 && wisdom.run.resolve_deck_choice(0));
        assert(wisdom.run.master_deck.size() == deck + 1 && !wisdom.run.has_deck_choice());
    }

    std::cout << "[PASS] Tinker Time: 3 card types x 3 riders (upgraded card / Colorless card / 1 of 3)" << std::endl;
}
