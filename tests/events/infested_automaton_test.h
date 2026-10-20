#pragma once
// Automate infesté (INFESTED_AUTOMATON): study (a random Power joins the deck) / touch the heart (a random card
// costing 0 joins the deck).
#include <iostream>
#include "event_test_support.h"

inline void test_infested_automaton() {
    using namespace event_test;

    const EventDef* def = EventManager::find("INFESTED_AUTOMATON");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    const auto reseed = [](EventRun& e, int round) {
        e.run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42 + round);
        e.run.hp = 40;
        e.run.max_hp = 80;
        e.run.gold = 100;
        const bool opened = e.run.start_event("INFESTED_AUTOMATON");
        assert(opened);
        (void)opened;
    };

    // A new run of another seed each time, the event open again: the card is always of the right kind and never a Basic one.
    for (int round = 0; round < 8; ++round) {
        EventRun study("INFESTED_AUTOMATON");
        reseed(study, round);
        const size_t deck = study.run.master_deck.size();
        study.pick(0);
        assert(study.run.master_deck.size() == deck + 1);
        const DeckCard& power = study.run.master_deck[deck];
        assert(CardDef::get(power.id).type == CardType::POWER && !power.upgraded);
        assert(card_meta(power.id).rarity != CardRarity::BASIC);
        assert(study.run.hp == 40 && study.run.gold == 100 && study.run.phase == RunPhase::MAP_NAVIGATION);

        EventRun touch("INFESTED_AUTOMATON");
        reseed(touch, round);
        touch.pick(1);
        assert(touch.run.master_deck.size() == deck + 1);
        const DeckCard& free_card = touch.run.master_deck[deck];
        assert(CardDef::get_cost(free_card.id, false) == 0 && card_meta(free_card.id).rarity != CardRarity::BASIC);
        assert(touch.run.hp == 40 && touch.run.gold == 100 && touch.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Infested Automaton: study (random Power) / touch the heart (random 0-cost card)" << std::endl;
}
