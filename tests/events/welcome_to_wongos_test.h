#pragma once
// Bienvenue chez Wongo's (WELCOME_TO_WONGOS): three relic offers for 100 / 200 / 300 gold, or leave and lose an upgrade.
#include <iostream>
#include "engine/relic.h"
#include "event_test_support.h"

inline void test_welcome_to_wongos() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WELCOME_TO_WONGOS");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 4);

    // With 100 gold only the bargain bin is open.
    {
        EventRun e("WELCOME_TO_WONGOS");
        const auto& choices = e.run.current_event.choices;
        assert(choices.size() == 4 && choices[0].enabled && !choices[1].enabled && !choices[2].enabled && choices[3].enabled);
    }

    // Bargain bin: 100 gold for one Common relic.
    {
        EventRun e("WELCOME_TO_WONGOS");
        const size_t relics = e.run.relics.size();
        e.pick(0);
        assert(e.run.gold == 0 && e.run.relics.size() == relics + 1);
        assert(find_relic(e.run.relics.back())->rarity == RelicRarity::COMMON);
        // (a relic can change HP or the deck when obtained, so only the event's own effects are checked)
        assert(e.run.phase != RunPhase::EVENT);
    }

    // Featured item: 200 gold for Frozen Egg.
    {
        EventRun e("WELCOME_TO_WONGOS");
        e.run.gold = 250;
        e.run.start_event("WELCOME_TO_WONGOS");
        assert(e.run.current_event.choices[1].enabled && !e.run.current_event.choices[2].enabled);
        const size_t relics = e.run.relics.size();
        e.pick(1);
        assert(e.run.gold == 50 && e.run.has_relic("FROZEN_EGG") && e.run.relics.size() == relics + 1);
        assert(e.run.phase != RunPhase::EVENT);
    }

    // Mystery box: 300 gold for 3 relics.
    {
        EventRun e("WELCOME_TO_WONGOS");
        e.run.gold = 300;
        e.run.start_event("WELCOME_TO_WONGOS");
        assert(e.run.current_event.choices[2].enabled);
        const size_t relics = e.run.relics.size();
        e.pick(2);
        assert(e.run.gold == 0 && e.run.relics.size() == relics + 3);
        assert(e.run.phase != RunPhase::EVENT);
    }

    // Leave: one upgraded card is downgraded, gold untouched.
    {
        EventRun e("WELCOME_TO_WONGOS");
        e.run.master_deck[2].upgraded = true;
        e.run.master_deck[4].upgraded = true;
        const size_t relics = e.run.relics.size();
        e.pick(3);
        assert(e.upgraded_cards() == 1 && e.run.gold == 100 && e.run.relics.size() == relics);
        assert(e.run.hp == 40 && e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Leave with nothing upgraded: nothing happens.
    {
        EventRun e("WELCOME_TO_WONGOS");
        e.pick(3);
        assert(e.upgraded_cards() == 0 && e.run.gold == 100 && e.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Welcome to Wongo's: bargain bin / featured Frozen Egg / mystery box (gold checks) / leave and downgrade"
              << std::endl;
}
