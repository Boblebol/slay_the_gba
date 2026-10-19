#pragma once
// Portes de l'ombre et de la lumière (DOORS_OF_LIGHT_AND_DARK): each door is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_doors_of_light_and_dark() {
    using namespace event_test;

    const EventDef* def = EventManager::find("DOORS_OF_LIGHT_AND_DARK");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Light door: 2 random cards upgraded, nothing else changes.
    {
        EventRun light("DOORS_OF_LIGHT_AND_DARK");
        const size_t deck = light.run.master_deck.size();
        light.pick(0);
        assert(light.upgraded_cards() == 2 && light.run.master_deck.size() == deck);
        assert(light.run.hp == 40 && light.run.gold == 100 && light.run.potions.empty());
        assert(!light.run.has_deck_choice() && light.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Dark door: a card removed, chosen by the player.
    {
        EventRun dark("DOORS_OF_LIGHT_AND_DARK");
        const size_t deck = dark.run.master_deck.size();
        dark.pick(1);
        assert(dark.run.has_deck_choice() && dark.run.master_deck.size() == deck);
        assert(dark.run.resolve_deck_choice(0));
        assert(dark.run.master_deck.size() == deck - 1 && dark.upgraded_cards() == 0);
        assert(dark.run.hp == 40 && dark.run.gold == 100 && dark.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Doors of Light and Dark: 2 random upgrades / 1 card removed" << std::endl;
}
