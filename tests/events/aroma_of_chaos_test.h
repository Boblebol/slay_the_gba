#pragma once
// Arôme du chaos (AROMA_OF_CHAOS): Let Go transforms a chosen card, Maintain Control upgrades a chosen one.
#include <iostream>
#include "event_test_support.h"

inline void test_aroma_of_chaos() {
    using namespace event_test;

    const EventDef* def = EventManager::find("AROMA_OF_CHAOS");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Let Go: a deck screen opens; the chosen card becomes another one.
    {
        EventRun let_go("AROMA_OF_CHAOS");
        const size_t deck = let_go.run.master_deck.size();
        let_go.pick(0);
        assert(let_go.run.has_deck_choice() && let_go.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        const CardId before = let_go.run.master_deck[0].id;
        assert(let_go.run.resolve_deck_choice(0));
        assert(!let_go.run.has_deck_choice() && let_go.run.master_deck.size() == deck);
        assert(let_go.run.master_deck[0].id != before);
        assert(let_go.run.hp == 40 && let_go.run.gold == 100 && let_go.run.potions.empty());
    }

    // Maintain Control: the chosen card is upgraded, nothing else changes.
    {
        EventRun control("AROMA_OF_CHAOS");
        const size_t deck = control.run.master_deck.size();
        control.pick(1);
        assert(control.run.has_deck_choice() && control.run.deck_choice.kind == DeckChoiceKind::UPGRADE);
        assert(control.run.resolve_deck_choice(0));
        assert(control.run.master_deck.size() == deck && control.upgraded_cards() == 1);
        assert(control.run.master_deck[0].upgraded);
        assert(control.run.hp == 40 && control.run.gold == 100 && control.run.potions.empty());
    }

    std::cout << "[PASS] Aroma of Chaos: Let Go (transform a card) / Maintain Control (upgrade a card)" << std::endl;
}
