#pragma once
// Reflets stelfeR (REFLECTIONS): upgrade 4 / downgrade 2 random cards, or duplicate the deck and take Bad Luck.
#include <iostream>
#include "event_test_support.h"

inline void test_reflections() {
    using namespace event_test;

    const EventDef* def = EventManager::find("REFLECTIONS");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Touch a Mirror: 4 cards upgraded, then 2 of the upgraded ones downgraded: 2 stay upgraded.
    {
        EventRun touch("REFLECTIONS");
        const size_t deck = touch.run.master_deck.size();
        touch.pick(0);
        assert(touch.run.master_deck.size() == deck && touch.upgraded_cards() == 2);
        assert(touch.run.hp == 40 && touch.run.gold == 100 && touch.run.potions.empty());
        assert(touch.run.phase == RunPhase::MAP_NAVIGATION && !touch.run.has_deck_choice());
    }

    // Shatter: every card twice, plus Bad Luck.
    {
        EventRun shatter("REFLECTIONS");
        const size_t deck = shatter.run.master_deck.size();
        CardId first = shatter.run.master_deck[0].id;
        shatter.pick(1);
        assert(shatter.run.master_deck.size() == deck * 2 + 1);
        for (size_t i = 0; i < deck; ++i) assert(shatter.run.master_deck[i].id == shatter.run.master_deck[deck + i].id);
        assert(shatter.run.master_deck[0].id == first && shatter.run.master_deck.back().id == CardId::BAD_LUCK);
        assert(shatter.run.hp == 40 && shatter.run.gold == 100 && shatter.run.potions.empty());
    }

    // A deck too full to be copied entirely stops at its limit instead of failing.
    {
        EventRun full("REFLECTIONS");
        while (full.run.master_deck.size() < 40) full.run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
        full.pick(1);
        assert(full.run.master_deck.size() == full.run.master_deck.capacity());
    }

    std::cout << "[PASS] Reflections: Touch a Mirror (4 upgrades, 2 downgrades) / Shatter (deck duplicated, Bad Luck)"
              << std::endl;
}
