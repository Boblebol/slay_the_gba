#pragma once
// Vallon des Murmures (WHISPERING_HOLLOW): each option is chosen on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_whispering_hollow() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WHISPERING_HOLLOW");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Exchange gold: 50 gold for two random potions.
    {
        EventRun swap("WHISPERING_HOLLOW");
        assert(swap.run.current_event.choices[0].enabled);
        const size_t potions = swap.run.potions.size();
        swap.pick(0);
        assert(swap.run.gold == 50 && swap.run.potions.size() == potions + 2);
        assert(swap.run.hp == 40 && !swap.run.has_deck_choice() && swap.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Not enough gold: greyed out.
    {
        EventRun poor("WHISPERING_HOLLOW");
        poor.run.gold = 49;
        poor.run.start_event("WHISPERING_HOLLOW");
        assert(!poor.run.current_event.choices[0].enabled && poor.run.current_event.choices[1].enabled);
    }

    // Hug the tree: 9 HP lost, then a chosen card is transformed.
    {
        EventRun hug("WHISPERING_HOLLOW");
        const size_t deck = hug.run.master_deck.size();
        const CardId before = hug.run.master_deck[0].id;
        hug.pick(1);
        assert(hug.run.hp == 40 - 9 && hug.run.gold == 100 && hug.run.potions.empty());
        assert(hug.run.has_deck_choice() && hug.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        assert(hug.run.resolve_deck_choice(0));
        assert(!hug.run.has_deck_choice() && hug.run.master_deck.size() == deck);
        assert(hug.run.master_deck[0].id != before);
    }

    // Too weak to pay the 9 HP: greyed out.
    {
        EventRun weak("WHISPERING_HOLLOW");
        weak.run.hp = 9;
        weak.run.start_event("WHISPERING_HOLLOW");
        assert(weak.run.current_event.choices[0].enabled && !weak.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] Whispering Hollow: 50 gold for 2 potions / 9 HP to transform a card" << std::endl;
}
