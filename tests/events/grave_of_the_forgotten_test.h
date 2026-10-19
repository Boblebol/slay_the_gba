#pragma once
// Tombe du guerrier oublié (GRAVE_OF_THE_FORGOTTEN): the Forgotten Soul relic, or Decay and Soul's Power on a card.
#include <iostream>
#include "event_test_support.h"

inline void test_grave_of_the_forgotten() {
    using namespace event_test;

    const EventDef* def = EventManager::find("GRAVE_OF_THE_FORGOTTEN");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Accept: the relic and nothing else.
    {
        EventRun e("GRAVE_OF_THE_FORGOTTEN");
        const size_t deck = e.run.master_deck.size();
        const size_t relics = e.run.relics.size();
        e.pick(0);
        assert(e.run.has_relic("FORGOTTEN_SOUL") && e.run.relics.size() == relics + 1);
        assert(e.run.master_deck.size() == deck && e.run.hp == 40 && e.run.max_hp == 80 && e.run.gold == 100);
        assert(e.run.potions.empty() && !e.run.has_deck_choice() && e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Confront is greyed out while no card of the deck has Exhaust (the starter deck has none).
    {
        EventRun e("GRAVE_OF_THE_FORGOTTEN");
        assert(e.run.current_event.choices[0].enabled && !e.run.current_event.choices[1].enabled);
    }

    // Confront: Decay in the deck, and the Exhaust card chosen loses its Exhaust.
    {
        EventRun e("GRAVE_OF_THE_FORGOTTEN");
        e.run.add_card_to_deck(DeckCard{ CardId::IMPERVIOUS, false });
        e.run.start_event("GRAVE_OF_THE_FORGOTTEN");
        assert(e.run.current_event.choices[1].enabled);
        const size_t deck = e.run.master_deck.size();
        const size_t relics = e.run.relics.size();
        e.pick(1);
        assert(e.run.master_deck.size() == deck + 1 && e.run.master_deck.back().id == CardId::DECAY);
        assert(e.run.has_deck_choice() && e.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(e.run.deck_choice.enchantment == Enchantment::SOULS_POWER);
        int impervious = -1;
        for (size_t i = 0; i < e.run.master_deck.size(); ++i) {
            if (e.run.deck_choice_allows(static_cast<int>(i))) {
                assert(e.run.master_deck[i].id == CardId::IMPERVIOUS); // only an Exhaust card is offered
                impervious = static_cast<int>(i);
            }
        }
        assert(impervious >= 0 && e.run.resolve_deck_choice(impervious));
        const DeckCard& card = e.run.master_deck[impervious];
        assert(card.enchantment == Enchantment::SOULS_POWER && !CardDef::has_keyword(card, KEYWORD_EXHAUST));
        assert(!e.run.has_relic("FORGOTTEN_SOUL") && e.run.relics.size() == relics);
        assert(e.run.hp == 40 && e.run.max_hp == 80 && e.run.gold == 100 && e.run.potions.empty());
        assert(e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Grave of the Forgotten: Forgotten Soul / Decay and Soul's Power (greyed out without an Exhaust card)"
              << std::endl;
}
