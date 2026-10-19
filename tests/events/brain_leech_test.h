#pragma once
// Sangsue cérébrale (BRAIN_LEECH): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_brain_leech() {
    using namespace event_test;

    const EventDef* def = EventManager::find("BRAIN_LEECH");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Share Knowledge: 5 distinct cards are offered, the one chosen joins the deck.
    {
        EventRun share("BRAIN_LEECH");
        const size_t deck = share.run.master_deck.size();
        const size_t relics = share.run.relics.size();
        share.pick(0);
        assert(share.run.has_deck_choice() && share.run.deck_choice.kind == DeckChoiceKind::ADD);
        assert(share.run.deck_choice_offers.size() == 5);
        for (size_t a = 0; a < 5; ++a) {
            for (size_t b = a + 1; b < 5; ++b) assert(share.run.deck_choice_offers[a].id != share.run.deck_choice_offers[b].id);
        }
        const CardId taken = share.run.deck_choice_offers[2].id;
        assert(share.run.resolve_deck_choice(2));
        assert(!share.run.has_deck_choice() && share.run.master_deck.size() == deck + 1);
        assert(share.run.master_deck.back().id == taken);
        assert(share.run.hp == 40 && share.run.max_hp == 80 && share.run.gold == 100);
        assert(share.run.relics.size() == relics && share.run.potions.empty());
    }

    // Rip the Leech Off: 5 HP lost, a reward of Colorless cards.
    {
        EventRun rip("BRAIN_LEECH");
        const size_t deck = rip.run.master_deck.size();
        rip.pick(1);
        assert(rip.run.hp == 40 - 5 && rip.run.max_hp == 80 && rip.run.gold == 100);
        assert(rip.run.phase == RunPhase::REWARD_SCREEN && rip.run.current_card_rewards.size() == 3);
        for (const DeckCard& offered : rip.run.current_card_rewards) {
            assert(card_meta(offered.id).pool == CardPool::COLORLESS);
        }
        assert(rip.run.master_deck.size() == deck && !rip.run.has_deck_choice());
    }

    std::cout << "[PASS] Brain Leech: Share Knowledge (1 of 5 cards) / Rip the Leech Off (-5 HP, Colorless reward)"
              << std::endl;
}
