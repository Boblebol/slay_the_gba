#pragma once
// Source mystique (WELLSPRING): bathe (remove a chosen card, gain Guilty) / bottle (a random potion).
#include <iostream>
#include "event_test_support.h"

inline void test_wellspring() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WELLSPRING");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    {
        EventRun bathe("WELLSPRING");
        const size_t deck = bathe.run.master_deck.size();
        bathe.pick(0);
        assert(bathe.run.has_deck_choice() && bathe.run.deck_choice.kind == DeckChoiceKind::REMOVE);
        assert(bathe.run.deck_choice.remaining == 1 && !bathe.run.deck_choice.optional);
        // Guilty is already in the deck: the first card is removed and the curse stays.
        assert(bathe.run.resolve_deck_choice(0));
        assert(!bathe.run.has_deck_choice() && bathe.run.master_deck.size() == deck);
        int guilty = 0;
        for (const DeckCard& card : bathe.run.master_deck) guilty += card.id == CardId::GUILTY;
        assert(guilty == 1 && bathe.run.potions.empty());
        assert(bathe.run.hp == 40 && bathe.run.max_hp == 80 && bathe.run.gold == 100);
    }

    {
        EventRun bottle("WELLSPRING");
        const size_t deck = bottle.run.master_deck.size();
        bottle.pick(1);
        assert(bottle.run.potions.size() == 1 && bottle.run.master_deck.size() == deck);
        assert(!bottle.run.has_deck_choice() && bottle.run.phase == RunPhase::MAP_NAVIGATION);
        assert(bottle.run.hp == 40 && bottle.run.gold == 100);
    }

    std::cout << "[PASS] Wellspring: bathe (remove a card, Guilty) / bottle (random potion)" << std::endl;
}
