#pragma once
// Symbiote (SYMBIOTE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_symbiote() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SYMBIOTE");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Approach: an Attack of the player's choice is Corrupted; a Defend cannot be.
    {
        EventRun approach("SYMBIOTE");
        const size_t deck = approach.run.master_deck.size();
        approach.pick(0);
        assert(approach.run.has_deck_choice() && approach.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(approach.run.deck_choice.enchantment == Enchantment::CORRUPTED);
        size_t defend = 0;
        while (CardDef::get(approach.run.master_deck[defend].id).type == CardType::ATTACK) ++defend;
        assert(!approach.run.deck_choice_allows(static_cast<int>(defend)));
        assert(!approach.run.resolve_deck_choice(static_cast<int>(defend)));
        size_t attack = 0;
        while (CardDef::get(approach.run.master_deck[attack].id).type != CardType::ATTACK) ++attack;
        assert(approach.run.resolve_deck_choice(static_cast<int>(attack)));
        assert(!approach.run.has_deck_choice() && approach.run.master_deck.size() == deck);
        int corrupted = 0;
        for (const DeckCard& card : approach.run.master_deck) corrupted += card.enchantment == Enchantment::CORRUPTED;
        assert(corrupted == 1 && approach.run.master_deck[attack].enchantment == Enchantment::CORRUPTED);
        assert(approach.run.hp == 40 && approach.run.max_hp == 80 && approach.run.gold == 100);
        assert(approach.run.potions.empty() && approach.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Kill with Fire: a card of the player's choice is transformed into another one.
    {
        EventRun fire("SYMBIOTE");
        const size_t deck = fire.run.master_deck.size();
        const CardId before = fire.run.master_deck[0].id;
        fire.pick(1);
        assert(fire.run.has_deck_choice() && fire.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        assert(fire.run.resolve_deck_choice(0));
        assert(!fire.run.has_deck_choice() && fire.run.master_deck.size() == deck);
        assert(fire.run.master_deck[0].enchantment == Enchantment::NONE);
        assert(fire.run.master_deck[0].id != before);
        assert(fire.run.hp == 40 && fire.run.gold == 100 && fire.run.potions.empty());
    }

    std::cout << "[PASS] Symbiote: Approach (Corrupted Attack of your choice) / Kill with Fire (transform a card)"
              << std::endl;
}
