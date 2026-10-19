#pragma once
// Autel de la Tour (MYSTIC_ALTAR), an event of the game before the data.
#include <iostream>
#include "event_test_support.h"

inline void test_mystic_altar() {
    using namespace event_test;

    EventRun blood("MYSTIC_ALTAR");
    blood.pick(0);
    assert(blood.run.hp == 40 - 7 && blood.run.gold == 100 + 90);

    EventRun prayer("MYSTIC_ALTAR");
    prayer.pick(1);
    assert(prayer.run.deck_choice.kind == DeckChoiceKind::UPGRADE && prayer.run.resolve_deck_choice(3));
    assert(prayer.run.master_deck[3].upgraded && prayer.upgraded_cards() == 1);
    assert(prayer.run.hp == 40 && prayer.run.gold == 100);

    EventRun leave("MYSTIC_ALTAR");
    leave.pick(2);
    assert(leave.run.hp == 40 && leave.run.gold == 100);

    std::cout << "[PASS] Mystic Altar: 7 HP for 90 gold / upgrade a chosen card / leave" << std::endl;
}
