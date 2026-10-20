#pragma once
// Ceci ou Cela ? (THIS_OR_THAT): each option is chosen on its own run and what it does to the run is checked.
#include <iostream>
#include "event_test_support.h"

inline void test_this_or_that() {
    using namespace event_test;

    const EventDef* def = EventManager::find("THIS_OR_THAT");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // This: 6 HP for 55 gold, nothing else.
    {
        EventRun bag("THIS_OR_THAT");
        const size_t deck = bag.run.master_deck.size();
        bag.pick(0);
        assert(bag.run.hp == 40 - 6 && bag.run.gold == 100 + 55);
        assert(bag.run.master_deck.size() == deck && bag.run.relics.size() == 1 && !bag.run.has_deck_choice());
        assert(bag.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // This is greyed out when the 6 HP cannot be paid without dying.
    {
        EventRun weak("THIS_OR_THAT");
        weak.run.hp = 6;
        assert(weak.run.start_event("THIS_OR_THAT") && !weak.run.current_event.choices[0].enabled);
        assert(weak.run.current_event.choices[1].enabled);
    }

    // That: the Clumsy curse joins the deck and a random relic is gained.
    {
        EventRun relic("THIS_OR_THAT");
        const size_t deck = relic.run.master_deck.size();
        const size_t relics = relic.run.relics.size();
        relic.pick(1);
        assert(relic.run.master_deck.size() == deck + 1 && relic.run.master_deck.back().id == CardId::CLUMSY);
        assert(relic.run.relics.size() == relics + 1);
        assert(relic.run.hp == 40 && relic.run.gold == 100 && relic.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] This or That: 6 HP for 55 gold / Clumsy curse for a random relic" << std::endl;
}
