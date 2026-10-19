#pragma once
// Nid d'oizodon (BYRDONIS_NEST): eating the egg gives Max HP, taking it adds the Byrdonis Egg card.
#include <iostream>
#include "event_test_support.h"

inline void test_byrdonis_nest() {
    using namespace event_test;

    const EventDef* def = EventManager::find("BYRDONIS_NEST");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    {
        EventRun eat("BYRDONIS_NEST");
        const size_t deck = eat.run.master_deck.size();
        eat.pick(0);
        assert(eat.run.max_hp == 80 + 7 && eat.run.hp == 40 + 7 && eat.run.gold == 100);
        assert(eat.run.master_deck.size() == deck && eat.run.phase == RunPhase::MAP_NAVIGATION);
        for (const DeckCard& card : eat.run.master_deck) assert(card.id != CardId::BYRDONIS_EGG);
    }

    {
        EventRun take("BYRDONIS_NEST");
        const size_t deck = take.run.master_deck.size();
        take.pick(1);
        int eggs = 0;
        for (const DeckCard& card : take.run.master_deck) eggs += card.id == CardId::BYRDONIS_EGG;
        assert(take.run.master_deck.size() == deck + 1 && eggs == 1);
        assert(take.run.max_hp == 80 && take.run.hp == 40 && take.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Byrdonis Nest: eat (+7 Max HP) / take (Byrdonis Egg)" << std::endl;
}
