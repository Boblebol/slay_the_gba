#pragma once
// Le thé de la table ronde (ROUND_TEA_PARTY): the tea (full heal, Royal Poison) / the brawl (11 HP, a random relic,
// then the fee page).
#include <iostream>
#include "event_test_support.h"

inline void test_round_tea_party() {
    using namespace event_test;

    const EventDef* def = EventManager::find("ROUND_TEA_PARTY");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 2 && def->pages[0].options.size() == 2);

    {
        EventRun tea("ROUND_TEA_PARTY");
        tea.pick(0);
        assert(tea.run.hp == 80 && tea.run.has_relic("ROYAL_POISON") && tea.run.gold == 100);
        assert(tea.run.phase == RunPhase::MAP_NAVIGATION);
    }

    {
        EventRun brawl("ROUND_TEA_PARTY");
        const size_t relics = brawl.run.relics.size();
        brawl.pick(1);
        assert(brawl.run.hp == 40 - 11 && brawl.run.relics.size() == relics + 1 && !brawl.run.has_relic("ROYAL_POISON"));
        assert(brawl.run.phase == RunPhase::EVENT && brawl.run.current_event.choices.size() == 1);
        brawl.pick(0);
        assert(brawl.run.phase == RunPhase::MAP_NAVIGATION && brawl.run.hp == 29 && brawl.run.gold == 100);
    }

    // Too weak to survive the brawl: the option is greyed out.
    {
        EventRun weak("ROUND_TEA_PARTY");
        weak.run.hp = 11;
        EventManager::open_page(weak.run, *def, 0);
        assert(weak.run.current_event.choices[0].enabled && !weak.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] Round Tea Party: tea (full heal, Royal Poison) / brawl (11 HP, random relic, fee page)"
              << std::endl;
}
