#pragma once
// Champ de trous à taille corporelle (FIELD_OF_MAN_SIZED_HOLES): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_field_of_man_sized_holes() {
    using namespace event_test;

    const EventDef* def = EventManager::find("FIELD_OF_MAN_SIZED_HOLES");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Enter Your Hole: one card of the player's choice gets Perfect Fit.
    {
        EventRun enter("FIELD_OF_MAN_SIZED_HOLES");
        const size_t deck = enter.run.master_deck.size();
        enter.pick(0);
        assert(enter.run.has_deck_choice() && enter.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(enter.run.deck_choice.enchantment == Enchantment::PERFECT_FIT);
        assert(enter.run.resolve_deck_choice(2));
        assert(!enter.run.has_deck_choice() && enter.run.master_deck.size() == deck);
        int fitted = 0;
        for (const DeckCard& card : enter.run.master_deck) fitted += card.enchantment == Enchantment::PERFECT_FIT;
        assert(fitted == 1 && enter.run.master_deck[2].enchantment == Enchantment::PERFECT_FIT);
        assert(enter.run.hp == 40 && enter.run.max_hp == 80 && enter.run.gold == 100 && enter.run.potions.empty());
        assert(enter.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Resist: 2 cards removed (the player's choice), a Normality curse added.
    {
        EventRun resist("FIELD_OF_MAN_SIZED_HOLES");
        const size_t deck = resist.run.master_deck.size();
        resist.pick(1);
        assert(resist.run.has_deck_choice() && resist.run.deck_choice.kind == DeckChoiceKind::REMOVE);
        assert(resist.run.deck_choice.remaining == 2);
        assert(resist.run.resolve_deck_choice(0));
        assert(resist.run.resolve_deck_choice(0));
        assert(!resist.run.has_deck_choice());
        assert(resist.run.master_deck.size() == deck - 2 + 1);
        int normality = 0;
        for (const DeckCard& card : resist.run.master_deck) normality += card.id == CardId::NORMALITY;
        assert(normality == 1);
        assert(resist.run.hp == 40 && resist.run.gold == 100 && resist.run.potions.empty());
    }

    std::cout << "[PASS] Field of Man-Sized Holes: Enter Your Hole (Perfect Fit) / Resist (remove 2 cards, Normality)"
              << std::endl;
}
