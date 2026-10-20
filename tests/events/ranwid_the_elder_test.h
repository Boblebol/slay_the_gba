#pragma once
// Ranwid l'Aîné (RANWID_THE_ELDER): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_ranwid_the_elder() {
    using namespace event_test;

    const EventDef* def = EventManager::find("RANWID_THE_ELDER");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    // Give 100 gold: one random relic. Greyed out below 100 gold.
    {
        EventRun gold("RANWID_THE_ELDER");
        assert(gold.run.current_event.choices[0].enabled);
        // Fresh run: no potion, a single relic: the other two options are greyed out.
        assert(!gold.run.current_event.choices[1].enabled && !gold.run.current_event.choices[2].enabled);
        const size_t deck = gold.run.master_deck.size();
        const size_t relics = gold.run.relics.size();
        gold.pick(0);
        assert(gold.run.gold == 0 && gold.run.relics.size() == relics + 1 && gold.run.hp == 40);
        assert(gold.run.master_deck.size() == deck && gold.run.potions.empty() && gold.run.phase == RunPhase::MAP_NAVIGATION);

        EventRun poor("RANWID_THE_ELDER");
        poor.run.gold = 99;
        poor.run.start_event("RANWID_THE_ELDER");
        assert(!poor.run.current_event.choices[0].enabled);
    }

    // Give a potion: the potion goes, one random relic comes.
    {
        EventRun potion("RANWID_THE_ELDER");
        potion.run.potions.push_back("FIRE_POTION");
        potion.run.start_event("RANWID_THE_ELDER");
        assert(potion.run.current_event.choices[1].enabled);
        const size_t relics = potion.run.relics.size();
        potion.pick(1);
        assert(potion.run.potions.empty() && potion.run.relics.size() == relics + 1);
        assert(potion.run.gold == 100 && potion.run.hp == 40 && potion.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Give a relic: the last one obtained goes (never the starter relic), two random relics come.
    {
        EventRun relic("RANWID_THE_ELDER");
        relic.run.add_relic("VAJRA");
        relic.run.start_event("RANWID_THE_ELDER");
        assert(relic.run.current_event.choices[2].enabled);
        const size_t relics = relic.run.relics.size();
        relic.pick(2);
        assert(!relic.run.has_relic("VAJRA") && relic.run.has_relic("BURNING_BLOOD") && relic.run.relics.size() == relics + 1);
        assert(relic.run.gold == 100 && relic.run.potions.empty() && relic.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Ranwid the Elder: 100 gold / a potion / a relic (2 back) for relics, greyed out without them"
              << std::endl;
}
