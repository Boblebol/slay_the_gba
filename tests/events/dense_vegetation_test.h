#pragma once
// Jungle dense (DENSE_VEGETATION): each option is chosen on its own run and what it does to the run is checked.
#include <iostream>
#include "event_test_support.h"

inline void test_dense_vegetation() {
    using namespace event_test;

    const EventDef* def = EventManager::find("DENSE_VEGETATION");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 3 && def->pages[0].options.size() == 2);

    // Overcome: one random card is removed, nothing else changes.
    {
        EventRun overcome("DENSE_VEGETATION");
        const size_t deck = overcome.run.master_deck.size();
        overcome.pick(1);
        assert(overcome.run.master_deck.size() == deck - 1 && !overcome.run.has_deck_choice());
        assert(overcome.run.hp == 40 && overcome.run.gold == 100 && overcome.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Hold On: 3 HP, then 4, then 5 for every further hold; the event goes on and no card is removed.
    {
        EventRun hold("DENSE_VEGETATION");
        const size_t deck = hold.run.master_deck.size();
        hold.pick(0);
        assert(hold.run.hp == 37 && hold.run.phase == RunPhase::EVENT && hold.run.current_event.page == 1);
        hold.pick(0);
        assert(hold.run.hp == 33 && hold.run.current_event.page == 2);
        hold.pick(0);
        assert(hold.run.hp == 28 && hold.run.current_event.page == 2);
        assert(hold.run.master_deck.size() == deck && hold.run.gold == 100);
        // Then it ends on Overcome.
        hold.pick(1);
        assert(hold.run.master_deck.size() == deck - 1 && hold.run.hp == 28 && hold.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Dense Vegetation: Hold On (3, 4, 5 HP) / Overcome (random card removed)" << std::endl;
}
