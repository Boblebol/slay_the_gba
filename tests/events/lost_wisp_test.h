#pragma once
// Feu follet égaré (LOST_WISP): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_lost_wisp() {
    using namespace event_test;

    const EventDef* def = EventManager::find("LOST_WISP");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    {
        EventRun capture("LOST_WISP");
        const size_t deck = capture.run.master_deck.size();
        capture.pick(0);
        assert(capture.run.has_relic("LOST_WISP") && capture.run.master_deck.size() == deck + 1);
        assert(capture.run.master_deck.back().id == CardId::DECAY);
        assert(capture.run.gold == 100 && capture.run.hp == 40 && capture.run.phase == RunPhase::MAP_NAVIGATION);
    }
    {
        EventRun search("LOST_WISP");
        const size_t deck = search.run.master_deck.size();
        search.pick(1);
        assert(search.run.gold == 160 && !search.run.has_relic("LOST_WISP") && search.run.master_deck.size() == deck);
        assert(search.run.phase == RunPhase::MAP_NAVIGATION);
    }
    {
        EventRun leave("LOST_WISP");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(2);
        assert(leave.run.gold == 100 && leave.run.hp == 40 && leave.run.master_deck.size() == deck);
        assert(leave.run.relics.size() == 1 && leave.run.phase == RunPhase::MAP_NAVIGATION);
    }
    // The wisp is taken once: with it already owned, capturing is greyed out.
    {
        EventRun owner("LOST_WISP");
        owner.run.add_relic("LOST_WISP");
        assert(owner.run.start_event("LOST_WISP") && !owner.run.current_event.choices[0].enabled);
    }

    std::cout << "[PASS] Lost Wisp: capture (relic + Decay) / search (60 gold) / leave" << std::endl;
}
