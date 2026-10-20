#pragma once
// Répi, historien de la guerre (WAR_HISTORIAN_REPY): the Lantern Key opens the cage (History Course) or the chest
// (2 potions, 2 relics); the event is only drawn, and its options only open, with the key in the deck.
#include <iostream>
#include "event_test_support.h"

inline void test_war_historian_repy() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WAR_HISTORIAN_REPY");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    auto keys = [](const EventRun& r) {
        int count = 0;
        for (const DeckCard& card : r.run.master_deck) count += card.id == CardId::LANTERN_KEY;
        return count;
    };

    // Without the key the options are greyed out.
    {
        EventRun none("WAR_HISTORIAN_REPY");
        assert(!none.run.current_event.choices[0].enabled && !none.run.current_event.choices[1].enabled);
    }

    // The cage: the key is spent, History Course is obtained, nothing else.
    {
        EventRun cage("WAR_HISTORIAN_REPY");
        const size_t deck = cage.run.master_deck.size();
        cage.run.master_deck.push_back(DeckCard(CardId::LANTERN_KEY, false));
        cage.run.start_event("WAR_HISTORIAN_REPY");
        assert(cage.run.current_event.choices[0].enabled && cage.run.current_event.choices[1].enabled);
        const size_t relics = cage.run.relics.size();
        cage.pick(0);
        assert(keys(cage) == 0 && cage.run.master_deck.size() == deck);
        assert(cage.run.has_relic("HISTORY_COURSE") && cage.run.relics.size() == relics + 1);
        assert(cage.run.potions.empty() && cage.run.gold == 100 && cage.run.hp == 40);
        assert(cage.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // The chest: the key is spent, 2 potions and 2 relics (none of them History Course by name) are obtained.
    {
        EventRun chest("WAR_HISTORIAN_REPY");
        const size_t deck = chest.run.master_deck.size();
        chest.run.master_deck.push_back(DeckCard(CardId::LANTERN_KEY, false));
        chest.run.start_event("WAR_HISTORIAN_REPY");
        const size_t relics = chest.run.relics.size();
        chest.pick(1);
        assert(keys(chest) == 0 && chest.run.master_deck.size() == deck);
        assert(chest.run.potions.size() == 2 && chest.run.relics.size() == relics + 2);
        assert(chest.run.gold == 100 && chest.run.hp == 40 && chest.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Never drawn without a Lantern Key in the deck.
    {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
        assert(!EventManager::meets(run, def->condition));
        run.master_deck.push_back(DeckCard(CardId::LANTERN_KEY, false));
        assert(EventManager::meets(run, def->condition));
    }

    std::cout << "[PASS] War Historian Repy: cage (History Course) / chest (2 potions, 2 relics), both spend the Lantern Key"
              << std::endl;
}
