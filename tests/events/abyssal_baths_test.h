#pragma once
// Bains abyssaux (ABYSSAL_BATHS), Act 1 (B): Immerse / Abstain, then Linger or Exit Baths.
#include <iostream>
#include "event_test_support.h"

inline void test_abyssal_baths() {
    using namespace event_test;

    const EventDef* def = EventManager::find("ABYSSAL_BATHS");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 7 && def->pages[0].options.size() == 2);

    // Abstain: +10 HP, nothing else, the event ends.
    {
        EventRun abstain("ABYSSAL_BATHS");
        abstain.pick(1);
        assert(abstain.run.hp == 50 && abstain.run.max_hp == 80 && abstain.run.gold == 100);
        assert(abstain.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Immerse: +2 Max HP for 3 HP (the Max HP rise raises the HP too), then each further bath costs 1 HP more.
    {
        EventRun bath("ABYSSAL_BATHS");
        bath.pick(0);
        assert(bath.run.max_hp == 82 && bath.run.hp == 40 + 2 - 3);
        assert(bath.run.phase == RunPhase::EVENT && bath.run.current_event.page == 1);
        assert(bath.run.current_event.choices.size() == 2);
        bath.pick(0); // Linger: 4 HP
        assert(bath.run.max_hp == 84 && bath.run.hp == 39 + 2 - 4 && bath.run.current_event.page == 2);
        bath.pick(0); // 5 HP
        assert(bath.run.max_hp == 86 && bath.run.hp == 37 + 2 - 5 && bath.run.current_event.page == 3);
        bath.pick(1); // Exit Baths: nothing more
        assert(bath.run.max_hp == 86 && bath.run.hp == 34 && bath.run.phase == RunPhase::MAP_NAVIGATION);
        assert(bath.run.gold == 100);
    }

    // Guessed end: 6 baths at most (3 to 8 HP), then only the exit is left.
    {
        EventRun long_bath("ABYSSAL_BATHS");
        long_bath.run.hp = long_bath.run.max_hp = 80;
        long_bath.pick(0);
        for (int bath = 2; bath <= 6; ++bath) {
            assert(long_bath.run.current_event.choices.size() == 2);
            long_bath.pick(0);
        }
        assert(long_bath.run.current_event.page == 6 && long_bath.run.current_event.choices.size() == 1);
        assert(long_bath.run.max_hp == 80 + 12 && long_bath.run.phase == RunPhase::EVENT);
        long_bath.pick(0);
        assert(long_bath.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // A bath the HP cannot pay is greyed out (it never kills): the first one costs 3, so 3 HP is too few.
    {
        EventRun weak("ABYSSAL_BATHS");
        weak.run.hp = 3;
        assert(weak.run.start_event("ABYSSAL_BATHS"));
        assert(!weak.run.current_event.choices[0].enabled && weak.run.current_event.choices[1].enabled);
        assert(!weak.run.choose_event_choice(0) && weak.run.hp == 3 && weak.run.max_hp == 80);
    }

    std::cout << "[PASS] Abyssal Baths: Abstain +10 HP / Immerse +2 Max HP for 3 HP, Linger 1 HP more each time, 6 baths"
              << std::endl;
}
