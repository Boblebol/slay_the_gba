#pragma once
// Fleur colossale (COLOSSAL_FLOWER), Act 2: Extract Nectar at each depth, or Reach Deeper up to the Pollinous Core.
#include <iostream>
#include "event_test_support.h"

inline void test_colossal_flower() {
    using namespace event_test;

    const EventDef* def = EventManager::find("COLOSSAL_FLOWER");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 3 && def->pages[0].options.size() == 2);

    // Extract Nectar on the first page: 35 gold, no damage.
    {
        EventRun nectar("COLOSSAL_FLOWER");
        nectar.pick(0);
        assert(nectar.run.gold == 135 && nectar.run.hp == 40 && nectar.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Deeper once: 5 HP, then 75 gold.
    {
        EventRun deeper("COLOSSAL_FLOWER");
        deeper.pick(1);
        assert(deeper.run.hp == 35 && deeper.run.gold == 100 && deeper.run.current_event.page == 1);
        deeper.pick(0);
        assert(deeper.run.gold == 175 && deeper.run.hp == 35 && deeper.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Deeper twice: 5 then 6 HP, then 135 gold.
    {
        EventRun deeper("COLOSSAL_FLOWER");
        deeper.pick(1);
        deeper.pick(1);
        assert(deeper.run.hp == 29 && deeper.run.current_event.page == 2);
        deeper.pick(0);
        assert(deeper.run.gold == 235 && deeper.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // The center: 5 + 6 + 7 HP, the Pollinous Core, no gold.
    {
        EventRun center("COLOSSAL_FLOWER");
        const size_t relics = center.run.relics.size();
        center.pick(1);
        center.pick(1);
        center.pick(1);
        assert(center.run.hp == 40 - 5 - 6 - 7 && center.run.gold == 100);
        assert(center.run.has_relic("POLLINOUS_CORE") && center.run.relics.size() == relics + 1);
        assert(center.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Guessed: a reach the HP cannot pay (it would kill) is greyed out.
    {
        EventRun frail("COLOSSAL_FLOWER");
        frail.run.hp = 5;
        assert(frail.run.start_event("COLOSSAL_FLOWER"));
        assert(!frail.run.current_event.choices[1].enabled && frail.run.current_event.choices[0].enabled);
        assert(!frail.run.choose_event_choice(1) && frail.run.hp == 5);
    }

    std::cout << "[PASS] Colossal Flower: nectar 35 / 75 / 135 gold, reaching deeper 5 / 6 / 7 HP, Pollinous Core at the center"
              << std::endl;
}
