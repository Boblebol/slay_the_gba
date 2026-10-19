#pragma once
// Mannequin d'Entraînement (BATTLEWORN_DUMMY), as the game had it before the data.
#include <iostream>
#include "event_test_support.h"

inline void test_battleworn_dummy() {
    using namespace event_test;

    // "6 damage, gain 60 gold".
    EventRun fight("BATTLEWORN_DUMMY");
    assert(fight.run.current_event.choices.size() == 3);
    fight.pick(0);
    assert(fight.run.hp == 40 - 6 && fight.run.gold == 100 + 60);

    // "Upgrade a card": the player picks it.
    EventRun repair("BATTLEWORN_DUMMY");
    repair.pick(1);
    assert(repair.run.has_deck_choice() && repair.run.deck_choice.kind == DeckChoiceKind::UPGRADE);
    assert(repair.run.deck_choice.remaining == 1 && !repair.run.deck_choice.optional);
    assert(repair.run.resolve_deck_choice(0));
    assert(repair.run.master_deck[0].upgraded && repair.upgraded_cards() == 1 && !repair.run.has_deck_choice());
    assert(repair.run.hp == 40 && repair.run.gold == 100);

    // Leave: nothing changes.
    EventRun flee("BATTLEWORN_DUMMY");
    flee.pick(2);
    assert(flee.run.hp == 40 && flee.run.gold == 100 && flee.run.relics.size() == 1 && !flee.run.has_deck_choice());

    std::cout << "[PASS] Battleworn Dummy: 6 damage for 60 gold / upgrade a chosen card / leave" << std::endl;
}
