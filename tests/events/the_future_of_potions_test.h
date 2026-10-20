#pragma once
// L'avenir des potions ? (THE_FUTURE_OF_POTIONS): a potion for an upgraded card of the same rarity.
#include <iostream>
#include "event_test_support.h"

inline void test_the_future_of_potions() {
    using namespace event_test;

    const EventDef* def = EventManager::find("THE_FUTURE_OF_POTIONS");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // No potion: the machine cannot be used.
    {
        EventRun empty("THE_FUTURE_OF_POTIONS");
        assert(empty.run.potions.empty());
        assert(!empty.run.current_event.choices[0].enabled && empty.run.current_event.choices[1].enabled);
    }

    // A Common potion (the first of the belt) gives an upgraded Common card; the other potion is kept.
    {
        EventRun common("THE_FUTURE_OF_POTIONS");
        common.run.potions.push_back("BLOCK_POTION");
        common.run.potions.push_back("BEETLE_JUICE");
        common.run.start_event("THE_FUTURE_OF_POTIONS");
        assert(common.run.current_event.choices[0].enabled);
        const size_t deck = common.run.master_deck.size();
        common.pick(0);
        assert(common.run.potions.size() == 1 && common.run.potions[0] == "BEETLE_JUICE");
        assert(common.run.master_deck.size() == deck + 1);
        const DeckCard& card = common.run.master_deck.back();
        assert(card.upgraded && card_meta(card.id).rarity == CardRarity::COMMON);
        assert(card_meta(card.id).pool == CardPool::IRONCLAD && common.run.gold == 100 && common.run.hp == 40);
        assert(common.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // A Rare potion gives an upgraded Rare card.
    {
        EventRun rare("THE_FUTURE_OF_POTIONS");
        rare.run.potions.push_back("BEETLE_JUICE");
        rare.run.start_event("THE_FUTURE_OF_POTIONS");
        rare.pick(0);
        assert(rare.run.potions.empty());
        const DeckCard& card = rare.run.master_deck.back();
        assert(card.upgraded && card_meta(card.id).rarity == CardRarity::RARE);
    }

    // Leaving keeps the potion.
    {
        EventRun leave("THE_FUTURE_OF_POTIONS");
        leave.run.potions.push_back("BLOCK_POTION");
        leave.run.start_event("THE_FUTURE_OF_POTIONS");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(1);
        assert(leave.run.potions.size() == 1 && leave.run.master_deck.size() == deck);
    }

    std::cout << "[PASS] The Future of Potions: a potion for an upgraded card of the same rarity / leave" << std::endl;
}
