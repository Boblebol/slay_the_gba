#pragma once
// La légende disait vrai (THE_LEGENDS_WERE_TRUE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_the_legends_were_true() {
    using namespace event_test;

    const EventDef* def = EventManager::find("THE_LEGENDS_WERE_TRUE");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Nab the Map: the Spoils Map quest card joins the deck, nothing else changes.
    {
        EventRun nab("THE_LEGENDS_WERE_TRUE");
        const size_t deck = nab.run.master_deck.size();
        nab.pick(0);
        assert(nab.run.master_deck.size() == deck + 1 && nab.run.master_deck.back().id == CardId::SPOILS_MAP);
        assert(nab.run.hp == 40 && nab.run.max_hp == 80 && nab.run.gold == 100 && nab.run.potions.empty());
        assert(nab.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Slowly Find an Exit: 8 HP lost and a random potion.
    {
        EventRun exit("THE_LEGENDS_WERE_TRUE");
        const size_t deck = exit.run.master_deck.size();
        exit.pick(1);
        assert(exit.run.hp == 40 - 8 && exit.run.max_hp == 80 && exit.run.gold == 100);
        assert(exit.run.potions.size() == 1 && exit.run.master_deck.size() == deck);
        assert(exit.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Too weak to pay the 8 HP: the exit is greyed out.
    {
        EventRun weak("THE_LEGENDS_WERE_TRUE");
        weak.run.hp = 8;
        EventManager::open_page(weak.run, *def, 0);
        assert(weak.run.current_event.choices[0].enabled && !weak.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] The Legends Were True: Spoils Map / 8 HP for a potion" << std::endl;
}
