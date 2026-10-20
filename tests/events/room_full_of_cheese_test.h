#pragma once
// Salle du Fromage (ROOM_FULL_OF_CHEESE): 2 random Common cards, or 14 HP for the Chosen Cheese relic.
#include <iostream>
#include "event_test_support.h"

inline void test_room_full_of_cheese() {
    using namespace event_test;

    const EventDef* def = EventManager::find("ROOM_FULL_OF_CHEESE");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Gorge: 2 Common cards of the hero, nothing else.
    {
        EventRun e("ROOM_FULL_OF_CHEESE");
        const size_t deck = e.run.master_deck.size();
        const size_t relics = e.run.relics.size();
        e.pick(0);
        assert(e.run.master_deck.size() == deck + 2);
        for (size_t i = deck; i < deck + 2; ++i) {
            assert(card_meta(e.run.master_deck[i].id).rarity == CardRarity::COMMON);
            assert(card_meta(e.run.master_deck[i].id).pool == CardPool::IRONCLAD);
        }
        assert(e.run.hp == 40 && e.run.max_hp == 80 && e.run.gold == 100 && e.run.relics.size() == relics);
        assert(e.run.potions.empty() && !e.run.has_deck_choice() && e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Search: 14 HP lost, Chosen Cheese gained.
    {
        EventRun e("ROOM_FULL_OF_CHEESE");
        const size_t deck = e.run.master_deck.size();
        const size_t relics = e.run.relics.size();
        assert(e.run.current_event.choices[1].enabled);
        e.pick(1);
        assert(e.run.hp == 40 - 14 && e.run.max_hp == 80 && e.run.gold == 100);
        assert(e.run.has_relic("CHOSEN_CHEESE") && e.run.relics.size() == relics + 1);
        assert(e.run.master_deck.size() == deck && e.upgraded_cards() == 0 && e.run.potions.empty());
        assert(e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Search is greyed out when the 14 HP cannot be paid.
    {
        EventRun e("ROOM_FULL_OF_CHEESE");
        e.run.hp = 14;
        e.run.start_event("ROOM_FULL_OF_CHEESE");
        assert(e.run.current_event.choices[0].enabled && !e.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] Room Full of Cheese: 2 random Common cards / 14 HP for Chosen Cheese (greyed out at 14 HP)"
              << std::endl;
}
