#pragma once
// La statue engloutie (SUNKEN_STATUE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_sunken_statue() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SUNKEN_STATUE");
    assert(def && def->acts == (EVENT_ACT_1A | EVENT_ACT_1B) && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Dive: 111 gold for 7 HP.
    {
        EventRun dive("SUNKEN_STATUE");
        assert(dive.run.current_event.choices.size() == 2 && dive.run.current_event.choices[0].enabled);
        const size_t deck = dive.run.master_deck.size();
        const size_t relics = dive.run.relics.size();
        dive.pick(0);
        assert(dive.run.gold == 100 + 111 && dive.run.hp == 40 - 7 && dive.run.max_hp == 80);
        assert(dive.run.master_deck.size() == deck && dive.run.relics.size() == relics && dive.run.potions.empty());
        assert(dive.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Too weak to dive: the option is greyed out.
    {
        EventRun weak("SUNKEN_STATUE");
        weak.run.hp = 7;
        weak.run.start_event("SUNKEN_STATUE");
        assert(!weak.run.current_event.choices[0].enabled && weak.run.current_event.choices[1].enabled);
    }

    // Sword of Stone: the relic, nothing else.
    {
        EventRun sword("SUNKEN_STATUE");
        const size_t relics = sword.run.relics.size();
        sword.pick(1);
        assert(sword.run.has_relic("SWORD_OF_STONE") && sword.run.relics.size() == relics + 1);
        assert(sword.run.gold == 100 && sword.run.hp == 40 && sword.run.max_hp == 80 && sword.run.potions.empty());
        assert(sword.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Sunken Statue: dive (+111 gold, -7 HP) / Sword of Stone" << std::endl;
}
