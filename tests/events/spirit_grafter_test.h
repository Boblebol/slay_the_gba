#pragma once
// Esprit greffeur (SPIRIT_GRAFTER): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_spirit_grafter() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SPIRIT_GRAFTER");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Let It In: 25 HP healed and Metamorphosis added to the deck.
    {
        EventRun in("SPIRIT_GRAFTER");
        const size_t deck = in.run.master_deck.size();
        in.pick(0);
        assert(in.run.hp == 40 + 25 && in.run.max_hp == 80 && in.run.gold == 100);
        assert(in.run.master_deck.size() == deck + 1 && in.run.master_deck.back().id == CardId::METAMORPHOSIS);
        assert(in.upgraded_cards() == 0 && in.run.potions.empty() && !in.run.has_deck_choice());
        assert(in.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Rejection: 10 HP lost, then a card of the player's choice upgraded.
    {
        EventRun reject("SPIRIT_GRAFTER");
        const size_t deck = reject.run.master_deck.size();
        reject.pick(1);
        assert(reject.run.hp == 40 - 10 && reject.run.has_deck_choice() && reject.upgraded_cards() == 0);
        assert(reject.run.resolve_deck_choice(0));
        assert(reject.upgraded_cards() == 1 && reject.run.master_deck.size() == deck);
        assert(reject.run.gold == 100 && reject.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Spirit Grafter: 25 HP and Metamorphosis / 10 HP for an upgrade" << std::endl;
}
