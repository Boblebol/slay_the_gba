#pragma once
// Tas de déchets (TRASH_HEAP): each option is chosen once, on its own run.
#include <algorithm>
#include <iostream>
#include "event_test_support.h"

inline void test_trash_heap() {
    using namespace event_test;

    const EventDef* def = EventManager::find("TRASH_HEAP");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 2);
    const auto relic_pool = def->pages[0].options[0].effects[1].relic_pool;
    const auto card_pool = def->pages[0].options[1].effects[1].card_pool;
    assert(relic_pool.size() == 5 && card_pool.size() == 10);

    // Dive In: 8 HP lost, one relic of the list gained.
    {
        EventRun dive("TRASH_HEAP");
        const size_t deck = dive.run.master_deck.size();
        const size_t relics = dive.run.relics.size();
        dive.pick(0);
        assert(dive.run.hp == 40 - 8 && dive.run.gold == 100 && dive.run.master_deck.size() == deck);
        assert(dive.run.relics.size() == relics + 1);
        const std::string_view got = dive.run.relics.back();
        assert(std::find(relic_pool.begin(), relic_pool.end(),
                         got) != relic_pool.end());
        assert(dive.run.potions.empty() && dive.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Grab Random Junk: 100 gold and one card of the list.
    {
        EventRun junk("TRASH_HEAP");
        const size_t deck = junk.run.master_deck.size();
        const size_t relics = junk.run.relics.size();
        junk.pick(1);
        assert(junk.run.hp == 40 && junk.run.gold == 200 && junk.run.relics.size() == relics);
        assert(junk.run.master_deck.size() == deck + 1);
        const CardId got = junk.run.master_deck.back().id;
        assert(std::find(card_pool.begin(), card_pool.end(),
                         got) != card_pool.end());
        assert(junk.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Every relic of the list can come out (over several seeds).
    {
        bool seen[5] = {};
        for (int seed = 0; seed < 60; ++seed) {
            RunManager run;
            run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 1000 + seed);
            run.hp = 40;
            const bool ok = run.start_event("TRASH_HEAP") && run.choose_event_choice(0);
            assert(ok);
            (void)ok;
            const std::string_view got = run.relics.back();
            for (int i = 0; i < 5; ++i) seen[i] |= (got == relic_pool[i]);
        }
        for (bool s : seen) assert(s);
    }

    std::cout << "[PASS] Trash Heap: 8 HP for one of 5 relics / 100 gold and one of 10 cards" << std::endl;
}
