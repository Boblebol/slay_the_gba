#pragma once
// Phare englouti (DROWNING_BEACON): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_drowning_beacon() {
    using namespace event_test;

    const EventDef* def = EventManager::find("DROWNING_BEACON");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Bottle: a Glowwater Potion, nothing else.
    {
        EventRun bottle("DROWNING_BEACON");
        assert(bottle.run.current_event.choices.size() == 2 && bottle.run.current_event.choices[0].enabled);
        const size_t deck = bottle.run.master_deck.size();
        const size_t relics = bottle.run.relics.size();
        bottle.pick(0);
        assert(bottle.run.potions.size() == 1 && bottle.run.potions[0] == "GLOWWATER_POTION");
        assert(bottle.run.hp == 40 && bottle.run.max_hp == 80 && bottle.run.gold == 100);
        assert(bottle.run.relics.size() == relics && !bottle.run.has_relic("FRESNEL_LENS"));
        assert(bottle.run.master_deck.size() == deck && !bottle.run.has_deck_choice());
        assert(bottle.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Climb: Fresnel Lens and 13 Max HP lost.
    {
        EventRun climb("DROWNING_BEACON");
        const size_t deck = climb.run.master_deck.size();
        const size_t relics = climb.run.relics.size();
        climb.pick(1);
        assert(climb.run.has_relic("FRESNEL_LENS") && climb.run.relics.size() == relics + 1);
        assert(climb.run.max_hp == 80 - 13 && climb.run.hp == 40);
        assert(climb.run.gold == 100 && climb.run.potions.empty() && climb.run.master_deck.size() == deck);
        assert(climb.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Drowning Beacon: Glowwater Potion / Fresnel Lens for 13 Max HP" << std::endl;
}
