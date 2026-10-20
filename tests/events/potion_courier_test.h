#pragma once
// Coursier de potions (POTION_COURIER): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_potion_courier() {
    using namespace event_test;

    const EventDef* def = EventManager::find("POTION_COURIER");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Grab Potions: three Foul Potions (the belt holds at least 3).
    {
        EventRun grab("POTION_COURIER");
        assert(grab.run.potions.empty() && grab.run.potion_slots >= 3);
        const size_t deck = grab.run.master_deck.size();
        grab.pick(0);
        assert(grab.run.potions.size() == 3);
        for (const auto& potion : grab.run.potions) assert(potion == "FOUL_POTION");
        assert(grab.run.hp == 40 && grab.run.gold == 100 && grab.run.master_deck.size() == deck);
        assert(grab.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Ransack: one random potion.
    {
        EventRun ransack("POTION_COURIER");
        ransack.pick(1);
        assert(ransack.run.potions.size() == 1 && !ransack.run.potions[0].empty());
        assert(ransack.run.hp == 40 && ransack.run.gold == 100 && ransack.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Potion Courier: 3 Foul Potions / 1 random potion" << std::endl;
}
