#pragma once
// Bains Abyssaux (ABYSSAL_BATHS), as the game had it before the data; the bath now costs its 35 gold.
#include <iostream>
#include "event_test_support.h"

inline void test_abyssal_baths() {
    using namespace event_test;

    EventRun bath("ABYSSAL_BATHS");
    bath.pick(0);
    assert(bath.run.hp == 40 + 25 && bath.run.gold == 100 - 35);

    // Without the 35 gold the bath is greyed out and cannot be chosen.
    EventRun poor("ABYSSAL_BATHS");
    poor.run.gold = 20;
    assert(poor.run.start_event("ABYSSAL_BATHS"));
    assert(!poor.run.current_event.choices[0].enabled && poor.run.current_event.choices[1].enabled);
    assert(!poor.run.choose_event_choice(0) && poor.run.phase == RunPhase::EVENT && poor.run.hp == 40);

    EventRun dive("ABYSSAL_BATHS");
    dive.pick(1);
    assert(dive.run.hp == 40 - 8 && dive.run.gold == 100 + 80);

    EventRun leave("ABYSSAL_BATHS");
    leave.pick(2);
    assert(leave.run.hp == 40 && leave.run.gold == 100);

    std::cout << "[PASS] Abyssal Baths: 25 HP for 35 gold (greyed out without them) / 80 gold for 8 damage / leave"
              << std::endl;
}
