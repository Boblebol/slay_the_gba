#pragma once
// Fleur Colossale (COLOSSAL_FLOWER), as the game had it before the data.
#include <iostream>
#include "engine/relic.h"
#include "event_test_support.h"

inline void test_colossal_flower() {
    using namespace event_test;

    // "Recover 20 HP", never above the maximum.
    EventRun drink("COLOSSAL_FLOWER");
    drink.pick(0);
    assert(drink.run.hp == 40 + 20 && drink.run.gold == 100);
    EventRun full("COLOSSAL_FLOWER");
    full.run.hp = 70;
    full.pick(0);
    assert(full.run.hp == full.run.max_hp);

    // "10 damage, get Vajra".
    EventRun thorn("COLOSSAL_FLOWER");
    const size_t relics = thorn.run.relics.size();
    thorn.pick(1);
    assert(thorn.run.hp == 40 - 10 && thorn.run.has_relic("VAJRA") && thorn.run.relics.size() == relics + 1);
    assert(find_relic(thorn.run.relics.back()) != nullptr);

    // An event can kill: the run ends on it, still in the EVENT phase.
    EventRun frail("COLOSSAL_FLOWER");
    frail.run.hp = 4;
    frail.pick(1);
    assert(frail.run.hp == 0 && frail.run.phase == RunPhase::EVENT);

    EventRun walk("COLOSSAL_FLOWER");
    walk.pick(2);
    assert(walk.run.hp == 40 && walk.run.gold == 100 && walk.run.relics.size() == relics);

    std::cout << "[PASS] Colossal Flower: 20 HP / 10 damage for Vajra (can kill) / leave" << std::endl;
}
