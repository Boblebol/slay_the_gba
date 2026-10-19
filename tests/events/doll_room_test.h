#pragma once
// Salle aux poupées (DOLL_ROOM): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_doll_room() {
    using namespace event_test;

    const EventDef* def = EventManager::find("DOLL_ROOM");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 3 && def->pages[0].options.size() == 3);

    auto dolls = [](const RunManager& run) {
        return int(run.has_relic("DAUGHTER_OF_THE_WIND")) + int(run.has_relic("MR_STRUGGLES")) +
               int(run.has_relic("BING_BONG"));
    };

    // Pick at Random: exactly one doll, no HP lost.
    {
        EventRun random("DOLL_ROOM");
        random.pick(0);
        assert(dolls(random.run) == 1 && random.run.hp == 40 && random.run.gold == 100);
        assert(random.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Take Some Time: 5 HP, then one of two dolls.
    for (int doll = 0; doll < 2; ++doll) {
        EventRun time("DOLL_ROOM");
        time.pick(1);
        assert(time.run.phase == RunPhase::EVENT && time.run.hp == 40 - 5 && dolls(time.run) == 0);
        assert(time.run.current_event.choices.size() == 2);
        time.pick(doll);
        assert(dolls(time.run) == 1 && time.run.hp == 35 && time.run.phase == RunPhase::MAP_NAVIGATION);
        assert(time.run.has_relic(doll == 0 ? "DAUGHTER_OF_THE_WIND" : "MR_STRUGGLES"));
    }

    // Examine Each: 15 HP, then one of the three dolls.
    for (int doll = 0; doll < 3; ++doll) {
        EventRun examine("DOLL_ROOM");
        examine.pick(2);
        assert(examine.run.phase == RunPhase::EVENT && examine.run.hp == 40 - 15);
        assert(examine.run.current_event.choices.size() == 3);
        examine.pick(doll);
        assert(dolls(examine.run) == 1 && examine.run.hp == 25 && examine.run.phase == RunPhase::MAP_NAVIGATION);
        assert(examine.run.has_relic(doll == 0 ? "DAUGHTER_OF_THE_WIND" : doll == 1 ? "MR_STRUGGLES" : "BING_BONG"));
    }

    std::cout << "[PASS] Doll Room: random doll / 2 dolls for 5 HP / 3 dolls for 15 HP" << std::endl;
}
