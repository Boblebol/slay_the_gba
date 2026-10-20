#pragma once
// Aventure du labyrinthe de la jungle (JUNGLE_MAZE_ADVENTURE): both options are chosen on their own run.
#include <iostream>
#include "event_test_support.h"

inline void test_jungle_maze_adventure() {
    using namespace event_test;

    const EventDef* def = EventManager::find("JUNGLE_MAZE_ADVENTURE");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Join forces: a small share of the loot, no cost.
    {
        EventRun together("JUNGLE_MAZE_ADVENTURE");
        together.pick(0);
        assert(together.run.gold == 100 + 42 && together.run.hp == 40 && together.run.max_hp == 80);
        assert(together.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Solo quest: the whole loot, 18 HP paid.
    {
        EventRun solo("JUNGLE_MAZE_ADVENTURE");
        assert(solo.run.current_event.choices[1].enabled);
        solo.pick(1);
        assert(solo.run.gold == 100 + 140 && solo.run.hp == 40 - 18 && solo.run.max_hp == 80);
        assert(solo.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Too weak to pay the 18 HP: the solo quest is greyed out.
    {
        EventRun weak("JUNGLE_MAZE_ADVENTURE");
        weak.run.hp = 18;
        weak.run.start_event("JUNGLE_MAZE_ADVENTURE");
        assert(weak.run.current_event.choices[0].enabled && !weak.run.current_event.choices[1].enabled);
    }

    std::cout << "[PASS] Jungle Maze Adventure: 42 gold / 140 gold for 18 HP" << std::endl;
}
