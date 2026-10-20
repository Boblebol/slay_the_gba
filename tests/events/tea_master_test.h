#pragma once
// Maître de thé (TEA_MASTER): Bone Tea (50 gold), Ember Tea (150 gold), Tea of Discourtesy (free); the paid ones
// are greyed out while the gold is short.
#include <iostream>
#include "event_test_support.h"

inline void test_tea_master() {
    using namespace event_test;

    const EventDef* def = EventManager::find("TEA_MASTER");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    {
        EventRun bone("TEA_MASTER");
        assert(bone.run.current_event.choices[0].enabled && !bone.run.current_event.choices[1].enabled);
        assert(bone.run.current_event.choices[2].enabled);
        bone.pick(0);
        assert(bone.run.has_relic("BONE_TEA") && bone.run.gold == 100 - 50);
        assert(bone.run.hp == 40 && bone.run.phase == RunPhase::MAP_NAVIGATION);
    }

    {
        EventRun ember("TEA_MASTER");
        ember.run.gold = 150;
        // The options are enabled when the page is opened: reopen it with the richer purse.
        EventManager::open_page(ember.run, *def, 0);
        assert(ember.run.current_event.choices[0].enabled && ember.run.current_event.choices[1].enabled);
        ember.pick(1);
        assert(ember.run.has_relic("EMBER_TEA") && ember.run.gold == 0 && !ember.run.has_relic("BONE_TEA"));
    }

    {
        EventRun rude("TEA_MASTER");
        rude.run.gold = 10;
        EventManager::open_page(rude.run, *def, 0);
        assert(!rude.run.current_event.choices[0].enabled && !rude.run.current_event.choices[1].enabled);
        assert(!rude.run.choose_event_choice(0, rude.message, sizeof(rude.message)));
        rude.pick(2);
        assert(rude.run.has_relic("TEA_OF_DISCOURTESY") && rude.run.gold == 10 && rude.run.hp == 40);
    }

    std::cout << "[PASS] Tea Master: Bone Tea (50) / Ember Tea (150) greyed out when too dear / Tea of Discourtesy"
              << std::endl;
}
