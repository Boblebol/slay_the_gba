#pragma once
// Tueur d'insectes (BUGSLAYER): each technique adds its card to the deck and changes nothing else.
#include <iostream>
#include "event_test_support.h"

inline void test_bugslayer() {
    using namespace event_test;

    const EventDef* def = EventManager::find("BUGSLAYER");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    const CardId cards[] = { CardId::EXTERMINATE, CardId::SQUASH };
    for (int option = 0; option < 2; ++option) {
        EventRun e("BUGSLAYER");
        assert(e.run.current_event.choices.size() == 2 && e.run.current_event.choices[option].enabled);
        const size_t deck = e.run.master_deck.size();
        const size_t relics = e.run.relics.size();
        e.pick(option);
        assert(e.run.master_deck.size() == deck + 1 && e.run.master_deck.back().id == cards[option]);
        assert(!e.run.master_deck.back().upgraded && e.upgraded_cards() == 0);
        assert(e.run.hp == 40 && e.run.max_hp == 80 && e.run.gold == 100);
        assert(e.run.relics.size() == relics && e.run.potions.empty() && !e.run.has_deck_choice());
        assert(e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Bugslayer: Exterminate / Squash added to the deck" << std::endl;
}
