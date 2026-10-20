#pragma once
// Trésor englouti (SUNKEN_TREASURY): a small purse, or a large one with the Greed curse.
#include <iostream>
#include "event_test_support.h"

inline void test_sunken_treasury() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SUNKEN_TREASURY");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // First chest: a few coins, nothing else.
    {
        EventRun first("SUNKEN_TREASURY");
        const size_t deck = first.run.master_deck.size();
        first.pick(0);
        assert(first.run.gold == 100 + 52 && first.run.hp == 40 && first.run.master_deck.size() == deck);
        assert(first.run.potions.empty() && first.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Second chest: a lot of gold and a Greed curse.
    {
        EventRun second("SUNKEN_TREASURY");
        const size_t deck = second.run.master_deck.size();
        second.pick(1);
        assert(second.run.gold == 100 + 303 && second.run.hp == 40);
        assert(second.run.master_deck.size() == deck + 1 && second.run.master_deck.back().id == CardId::GREED);
        assert(second.run.potions.empty() && second.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Sunken Treasury: first chest (52 gold) / second chest (303 gold, Greed)" << std::endl;
}
