#pragma once
// Tourbillon aquatique (SPIRALING_WHIRLPOOL): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_spiraling_whirlpool() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SPIRALING_WHIRLPOOL");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    {
        EventRun drink("SPIRALING_WHIRLPOOL");
        drink.pick(0);
        assert(drink.run.hp == 60 && drink.run.gold == 100 && !drink.run.has_deck_choice());
        assert(drink.run.phase == RunPhase::MAP_NAVIGATION);
    }
    {
        EventRun observe("SPIRALING_WHIRLPOOL");
        observe.pick(1);
        assert(observe.run.hp == 40 && observe.run.has_deck_choice() && observe.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(observe.run.resolve_deck_choice(0) && !observe.run.has_deck_choice());
        assert(observe.run.master_deck[0].enchantment == Enchantment::ADROIT);
    }
    {
        EventRun reach("SPIRALING_WHIRLPOOL");
        const size_t deck = reach.run.master_deck.size();
        const size_t relics = reach.run.relics.size();
        reach.pick(2);
        assert(reach.run.relics.size() == relics + 1 && reach.run.master_deck.size() == deck + 1);
        assert(reach.run.master_deck.back().id == CardId::WRITHE && reach.run.hp == 40);
    }

    std::cout << "[PASS] Spiraling Whirlpool: drink (heal 20) / observe (enchant) / reach in (relic + Writhe)" << std::endl;
}
