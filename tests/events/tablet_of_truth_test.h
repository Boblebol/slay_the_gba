#pragma once
// Stèle de la Vérité (TABLET_OF_TRUTH): smash for 20 HP, or decipher page after page and leave whenever wanted.
#include <iostream>
#include "event_test_support.h"

inline void test_tablet_of_truth() {
    using namespace event_test;

    const EventDef* def = EventManager::find("TABLET_OF_TRUTH");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 5 && def->pages[0].options.size() == 2);

    // Smash: 20 HP healed and nothing else.
    {
        EventRun e("TABLET_OF_TRUTH");
        const size_t deck = e.run.master_deck.size();
        e.pick(1);
        assert(e.run.hp == 60 && e.run.max_hp == 80 && e.run.gold == 100 && e.upgraded_cards() == 0);
        assert(e.run.master_deck.size() == deck && e.run.potions.empty() && e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Decipher once and leave: 3 Max HP for one upgrade.
    {
        EventRun e("TABLET_OF_TRUTH");
        e.pick(0);
        assert(e.run.phase == RunPhase::EVENT && e.run.current_event.page == 1);
        assert(e.run.max_hp == 77 && e.run.hp == 40 && e.upgraded_cards() == 1);
        e.pick(1); // give up
        assert(e.run.phase == RunPhase::MAP_NAVIGATION && e.run.max_hp == 77 && e.upgraded_cards() == 1);
        assert(e.run.gold == 100 && e.run.potions.empty());
    }

    // Decipher to the end: 5 readings, 15 Max HP, every card upgraded.
    {
        EventRun e("TABLET_OF_TRUTH");
        const size_t deck = e.run.master_deck.size();
        for (int page = 0; page < 4; ++page) {
            assert(e.run.current_event.page == page && e.run.current_event.choices.size() == 2);
            assert(e.run.current_event.choices[0].enabled);
            e.pick(0);
            assert(e.run.phase == RunPhase::EVENT && e.upgraded_cards() == page + 1);
            assert(e.run.max_hp == 80 - 3 * (page + 1));
        }
        assert(e.run.current_event.page == 4);
        e.pick(0);
        assert(e.run.phase == RunPhase::MAP_NAVIGATION && e.run.max_hp == 80 - 15);
        assert(e.run.master_deck.size() == deck && e.upgraded_cards() == static_cast<int>(deck));
        assert(e.run.hp == 40 && e.run.gold == 100 && !e.run.has_deck_choice());
    }

    std::cout << "[PASS] Tablet of Truth: smash (+20 HP) / decipher 5 pages (-3 Max HP, upgrades, all at the end) / give up"
              << std::endl;
}
