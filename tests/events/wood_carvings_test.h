#pragma once
// Gravures sur bois (WOOD_CARVINGS): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_wood_carvings() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WOOD_CARVINGS");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 4);

    {
        EventRun bird("WOOD_CARVINGS");
        const size_t deck = bird.run.master_deck.size();
        bird.pick(0);
        assert(bird.run.has_deck_choice() && bird.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        assert(bird.run.resolve_deck_choice(0) && !bird.run.has_deck_choice());
        assert(bird.run.master_deck.size() == deck && bird.run.master_deck[0].id == CardId::PECK);
    }
    {
        EventRun snake("WOOD_CARVINGS");
        snake.pick(1);
        assert(snake.run.has_deck_choice() && snake.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(snake.run.resolve_deck_choice(0) && !snake.run.has_deck_choice());
        assert(snake.run.master_deck[0].enchantment == Enchantment::ROYALLY_APPROVED);
    }
    {
        EventRun torus("WOOD_CARVINGS");
        const size_t deck = torus.run.master_deck.size();
        torus.pick(2);
        assert(torus.run.has_deck_choice() && torus.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        assert(torus.run.resolve_deck_choice(0) && !torus.run.has_deck_choice());
        assert(torus.run.master_deck.size() == deck && torus.run.master_deck[0].id == CardId::TORIC_TOUGHNESS);
    }
    {
        EventRun leave("WOOD_CARVINGS");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(3);
        assert(leave.run.master_deck.size() == deck && !leave.run.has_deck_choice() && leave.run.hp == 40);
        assert(leave.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Wood Carvings: bird (Peck) / snake (enchant an Attack) / torus (Toric Toughness) / leave" << std::endl;
}
