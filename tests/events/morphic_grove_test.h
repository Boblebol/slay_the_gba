#pragma once
// Bosquet des Morphiques (MORPHIC_GROVE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_morphic_grove() {
    using namespace event_test;

    const EventDef* def = EventManager::find("MORPHIC_GROVE");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // The group: all the gold lost, 2 cards transformed (chosen one after the other).
    {
        EventRun group("MORPHIC_GROVE");
        const size_t deck = group.run.master_deck.size();
        const CardId first = group.run.master_deck[0].id;
        const CardId second = group.run.master_deck[1].id;
        group.pick(0);
        assert(group.run.gold == 0 && group.run.hp == 40 && group.run.max_hp == 80);
        assert(group.run.deck_choice.kind == DeckChoiceKind::TRANSFORM && group.run.deck_choice.remaining == 2);
        assert(group.run.resolve_deck_choice(0));
        assert(group.run.resolve_deck_choice(1));
        assert(!group.run.has_deck_choice() && group.run.master_deck.size() == deck);
        assert(group.run.master_deck[0].id != first && group.run.master_deck[1].id != second);
        assert(group.run.potions.empty() && group.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // The loner: 5 Max HP (the HP rise with them), nothing else.
    {
        EventRun loner("MORPHIC_GROVE");
        const size_t deck = loner.run.master_deck.size();
        loner.pick(1);
        assert(loner.run.max_hp == 85 && loner.run.hp == 45 && loner.run.gold == 100);
        assert(loner.run.master_deck.size() == deck && !loner.run.has_deck_choice());
        assert(loner.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Morphic Grove: all gold for 2 transformations / 5 Max HP" << std::endl;
}
