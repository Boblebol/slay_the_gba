#pragma once
// Site sans repos (UNREST_SITE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_unrest_site() {
    using namespace event_test;

    const EventDef* def = EventManager::find("UNREST_SITE");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    auto count_of = [](const RunManager& run, CardId id) {
        int n = 0;
        for (const DeckCard& card : run.master_deck) n += card.id == id;
        return n;
    };

    // Rest anyway: full HP, and Poor Sleep in the deck.
    {
        EventRun rest("UNREST_SITE");
        assert(rest.run.current_event.choices.size() == 2 && rest.run.current_event.choices[0].enabled);
        const size_t deck = rest.run.master_deck.size();
        const size_t relics = rest.run.relics.size();
        rest.pick(0);
        assert(rest.run.hp == 80 && rest.run.max_hp == 80 && rest.run.gold == 100);
        assert(rest.run.master_deck.size() == deck + 1 && count_of(rest.run, CardId::POOR_SLEEP) == 1);
        assert(rest.run.relics.size() == relics && rest.run.potions.empty() && rest.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Kill the trees: 8 Max HP lost (the HP follow the new maximum), one random relic.
    {
        EventRun trees("UNREST_SITE");
        trees.run.hp = 78;
        trees.run.start_event("UNREST_SITE");
        const size_t deck = trees.run.master_deck.size();
        const size_t relics = trees.run.relics.size();
        trees.pick(1);
        assert(trees.run.max_hp == 80 - 8 && trees.run.hp == 72 && trees.run.gold == 100);
        assert(trees.run.relics.size() == relics + 1);
        assert(trees.run.master_deck.size() == deck && count_of(trees.run, CardId::POOR_SLEEP) == 0);
        assert(trees.run.potions.empty() && trees.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Unrest Site: rest anyway (full HP, Poor Sleep) / kill the trees (-8 Max HP, random relic)"
              << std::endl;
}
