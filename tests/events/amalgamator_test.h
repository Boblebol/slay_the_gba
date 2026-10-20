#pragma once
// Amalgamateur (AMALGAMATOR): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_amalgamator() {
    using namespace event_test;

    const EventDef* def = EventManager::find("AMALGAMATOR");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    auto count_of = [](const RunManager& run, CardId id) {
        int n = 0;
        for (const DeckCard& card : run.master_deck) n += card.id == id;
        return n;
    };

    // Combine Defends: 2 Defends out, Ultimate Defend in.
    {
        EventRun defends("AMALGAMATOR");
        assert(defends.run.current_event.choices.size() == 2 && defends.run.current_event.choices[0].enabled);
        const size_t deck = defends.run.master_deck.size();
        const int strikes = count_of(defends.run, CardId::STRIKE);
        const int base = count_of(defends.run, CardId::DEFEND);
        defends.pick(0);
        assert(count_of(defends.run, CardId::DEFEND) == base - 2 && count_of(defends.run, CardId::ULTIMATE_DEFEND) == 1);
        assert(count_of(defends.run, CardId::STRIKE) == strikes && defends.run.master_deck.size() == deck - 1);
        assert(defends.run.hp == 40 && defends.run.gold == 100 && defends.run.potions.empty());
        assert(!defends.run.has_deck_choice() && defends.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Combine Strikes: 2 Strikes out, Ultimate Strike in.
    {
        EventRun strikes("AMALGAMATOR");
        const size_t deck = strikes.run.master_deck.size();
        const int defends = count_of(strikes.run, CardId::DEFEND);
        const int base = count_of(strikes.run, CardId::STRIKE);
        strikes.pick(1);
        assert(count_of(strikes.run, CardId::STRIKE) == base - 2 && count_of(strikes.run, CardId::ULTIMATE_STRIKE) == 1);
        assert(count_of(strikes.run, CardId::DEFEND) == defends && strikes.run.master_deck.size() == deck - 1);
        assert(strikes.run.hp == 40 && strikes.run.gold == 100 && strikes.run.relics.size() == 1);
        assert(!strikes.run.has_deck_choice() && strikes.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Fewer than 2 Defends in the deck: that option is greyed out.
    {
        EventRun poor("AMALGAMATOR");
        for (size_t i = poor.run.master_deck.size(); i-- > 0;) {
            if (poor.run.master_deck[i].id == CardId::DEFEND && count_of(poor.run, CardId::DEFEND) > 1) {
                poor.run.remove_card_from_deck(static_cast<int>(i));
            }
        }
        assert(count_of(poor.run, CardId::DEFEND) == 1);
        poor.run.start_event("AMALGAMATOR");
        assert(!poor.run.current_event.choices[0].enabled && poor.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] Amalgamator: 2 Defends -> Ultimate Defend / 2 Strikes -> Ultimate Strike (greyed out without 2)"
              << std::endl;
}
