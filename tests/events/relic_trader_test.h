#pragma once
// Trafiquant de reliques (RELIC_TRADER): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_relic_trader() {
    using namespace event_test;

    const EventDef* def = EventManager::find("RELIC_TRADER");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    for (int index = 0; index < 3; ++index) {
        EventRun trade("RELIC_TRADER");
        // A second relic, so that the one given up is not always the starter one.
        trade.run.add_relic("ANCHOR");
        const size_t relics = trade.run.relics.size();
        const size_t deck = trade.run.master_deck.size();
        const std::string_view first = trade.run.relics[0];
        const std::string_view second = trade.run.relics[1];
        trade.pick(index);
        // One relic out, one relic in: the count is the same, and exactly one of the two was given up.
        assert(trade.run.relics.size() == relics);
        assert(trade.run.has_relic(first) != trade.run.has_relic(second));
        const std::string_view gained = trade.run.relics.back();
        assert(gained != first && gained != second);
        assert(trade.run.master_deck.size() == deck && trade.run.gold == 100 && trade.run.potions.empty());
        assert(trade.run.phase == RunPhase::MAP_NAVIGATION && !trade.run.has_deck_choice());
    }

    std::cout << "[PASS] Relic Trader: three offers, each trades a relic of the run for a new one" << std::endl;
}
