#pragma once
// Affamé de champis (HUNGRY_FOR_MUSHROOMS): the model of an event test. Each option is chosen once, on its own run,
// and what it does to the run (HP, gold, deck, relics, potions) is checked.
#include <iostream>
#include "event_test_support.h"

inline void test_hungry_for_mushrooms() {
    using namespace event_test;

    const EventDef* def = EventManager::find("HUNGRY_FOR_MUSHROOMS");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Big Mushroom: the relic, and its 20 Max HP on pickup (the HP rise with them).
    {
        EventRun big("HUNGRY_FOR_MUSHROOMS");
        assert(big.run.current_event.choices.size() == 2 && big.run.current_event.choices[0].enabled);
        const size_t deck = big.run.master_deck.size();
        const size_t relics = big.run.relics.size();
        big.pick(0);
        assert(big.run.has_relic("BIG_MUSHROOM") && big.run.relics.size() == relics + 1);
        assert(big.run.max_hp == 80 + 20 && big.run.hp == 40 + 20);
        assert(big.run.gold == 100 && big.run.master_deck.size() == deck && big.upgraded_cards() == 0);
        assert(big.run.potions.empty() && !big.run.has_deck_choice());
        assert(big.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Fragrant Mushroom: the relic, 15 HP lost and 2 random cards upgraded on pickup.
    {
        EventRun fragrant("HUNGRY_FOR_MUSHROOMS");
        const size_t deck = fragrant.run.master_deck.size();
        fragrant.pick(1);
        assert(fragrant.run.has_relic("FRAGRANT_MUSHROOM") && !fragrant.run.has_relic("BIG_MUSHROOM"));
        assert(fragrant.run.hp == 40 - 15 && fragrant.run.max_hp == 80);
        assert(fragrant.run.gold == 100 && fragrant.run.master_deck.size() == deck && fragrant.upgraded_cards() == 2);
        assert(fragrant.run.potions.empty() && fragrant.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Hungry for Mushrooms: Big Mushroom (+20 Max HP) / Fragrant Mushroom (-15 HP, 2 upgrades)"
              << std::endl;
}
