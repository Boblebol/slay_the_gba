#pragma once
// La clé de la lanterne (THE_LANTERN_KEY): opened by the Lantern Key quest card (tests/quest_cards_test.cpp), never
// drawn at random.
#include <iostream>
#include "event_test_support.h"

inline void test_the_lantern_key() {
    using namespace event_test;

    const EventDef* def = EventManager::find("THE_LANTERN_KEY");
    assert(def && def->acts == 0);

    // Give the key back: 100 gold.
    EventRun give("THE_LANTERN_KEY");
    assert(give.run.current_event.choices.size() == 2);
    give.pick(0);
    assert(give.run.gold == 100 + LANTERN_KEY_GOLD && give.run.potions.empty() && give.run.hp == 40);
    assert(give.run.phase == RunPhase::MAP_NAVIGATION);

    // Keep it: a potion.
    EventRun keep("THE_LANTERN_KEY");
    keep.pick(1);
    assert(keep.run.potions.size() == 1 && keep.run.potions[0] == "FIRE_POTION" && keep.run.gold == 100);

    // Never met in a ? room of any act without the key.
    for (uint8_t act = 1; act <= 3; ++act) {
        for (uint32_t seed = 1; seed < 40; ++seed) {
            RunManager run;
            run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
            run.current_act = act;
            run.trigger_event();
            assert(run.current_event.id != "THE_LANTERN_KEY");
        }
    }

    std::cout << "[PASS] The Lantern Key: give the key back for 100 gold / keep it for a potion, never drawn" << std::endl;
}
