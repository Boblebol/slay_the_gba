#pragma once
// Helpers shared by the tests of the events (tests/events/<id>_test.h, called from tests/events_test.cpp).
#include <cassert>
#include <string_view>
#include "engine/event_manager.h"
#include "engine/run.h"

namespace event_test {

using namespace engine;

// An Ironclad run of seed 42 at 40 / 80 HP and 100 gold, with the event `id` open on its first page.
struct EventRun {
    RunManager run;
    char message[256] = {};

    explicit EventRun(std::string_view id) {
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
        run.hp = 40;
        run.max_hp = 80;
        run.gold = 100;
        const bool opened = run.start_event(id);
        assert(opened && run.phase == RunPhase::EVENT && run.current_event.id == id);
        (void)opened;
    }

    // Picks the option at `index` of the page on screen; its result text is left in `message`.
    void pick(int index) {
        const std::string_view expected = run.current_event.choices[index].result_message;
        const bool ok = run.choose_event_choice(index, message, sizeof(message));
        assert(ok && std::string_view(message) == expected);
        (void)ok;
        (void)expected;
    }

    [[nodiscard]] int upgraded_cards() const {
        int count = 0;
        for (const DeckCard& card : run.master_deck) count += card.upgraded;
        return count;
    }
};

} // namespace event_test
