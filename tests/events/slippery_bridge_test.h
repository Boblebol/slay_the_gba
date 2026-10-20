#pragma once
// Pont glissant (SLIPPERY_BRIDGE): each option is chosen on its own run and what it does to the run is checked.
#include <iostream>
#include "event_test_support.h"

inline void test_slippery_bridge() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SLIPPERY_BRIDGE");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 2 && def->pages[0].options.size() == 2);

    // Trudge on: 8 HP, then a card of the player's choice is removed.
    {
        EventRun trudge("SLIPPERY_BRIDGE");
        const size_t deck = trudge.run.master_deck.size();
        trudge.pick(0);
        assert(trudge.run.hp == 40 - 8 && trudge.run.gold == 100);
        assert(trudge.run.has_deck_choice() && trudge.run.deck_choice.kind == DeckChoiceKind::REMOVE);
        assert(trudge.run.resolve_deck_choice(0));
        assert(trudge.run.master_deck.size() == deck - 1 && !trudge.run.has_deck_choice());
        assert(trudge.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Trudge on is greyed out when the 8 HP cannot be paid without dying.
    {
        EventRun weak("SLIPPERY_BRIDGE");
        weak.run.hp = 8;
        assert(weak.run.start_event("SLIPPERY_BRIDGE") && !weak.run.current_event.choices[0].enabled);
        assert(weak.run.current_event.choices[1].enabled);
    }

    // Rest: 25 HP healed, then the fight page, whose only option starts a fight with nothing else changed.
    {
        EventRun rest("SLIPPERY_BRIDGE");
        const size_t deck = rest.run.master_deck.size();
        rest.pick(1);
        assert(rest.run.hp == 40 + 25 && rest.run.phase == RunPhase::EVENT && rest.run.current_event.page == 1);
        assert(rest.run.current_event.choices.size() == 1);
        rest.pick(0);
        assert(rest.run.phase == RunPhase::IN_COMBAT && !rest.run.combat.enemies.empty());
        assert(rest.run.master_deck.size() == deck && rest.run.gold == 100 && rest.run.relics.size() == 1);
    }

    std::cout << "[PASS] Slippery Bridge: 8 HP to remove a card (greyed out when it would kill) / heal 25 then a fight"
              << std::endl;
}
