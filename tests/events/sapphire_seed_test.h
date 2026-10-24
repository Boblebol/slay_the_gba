#pragma once
// Graine de saphir (SAPPHIRE_SEED): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_sapphire_seed() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SAPPHIRE_SEED");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    {
        EventRun consume("SAPPHIRE_SEED");
        consume.pick(0);
        assert(consume.run.hp == 49 && consume.run.has_deck_choice() && consume.run.deck_choice.kind == DeckChoiceKind::UPGRADE);
        assert(consume.run.resolve_deck_choice(0) && !consume.run.has_deck_choice());
        assert(consume.upgraded_cards() == 1 && consume.run.master_deck[0].upgraded);
    }
    {
        EventRun plant("SAPPHIRE_SEED");
        plant.pick(1);
        assert(plant.run.hp == 40 && plant.run.has_deck_choice() && plant.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(plant.run.resolve_deck_choice(2) && !plant.run.has_deck_choice());
        assert(plant.run.master_deck[2].enchantment != Enchantment::NONE && plant.upgraded_cards() == 0);
    }

    std::cout << "[PASS] Sapphire Seed: consume (heal 9, upgrade) / plant (enchant a card)" << std::endl;
}
