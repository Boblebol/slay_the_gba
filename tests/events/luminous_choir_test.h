#pragma once
// Chœur lumineux (LUMINOUS_CHOIR): each option is chosen on its own run and what it does to the run is checked.
#include <iostream>
#include "event_test_support.h"

inline void test_luminous_choir() {
    using namespace event_test;

    const EventDef* def = EventManager::find("LUMINOUS_CHOIR");
    assert(def && def->acts == EVENT_ACT_1A && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    // Tribute: greyed out under 149 gold; with enough, 149 gold for a random relic.
    {
        EventRun poor("LUMINOUS_CHOIR");
        assert(poor.run.current_event.choices.size() == 3 && !poor.run.current_event.choices[0].enabled);
        assert(poor.run.current_event.choices[1].enabled && poor.run.current_event.choices[2].enabled);
        assert(!poor.run.choose_event_choice(0, poor.message, sizeof(poor.message)));

        EventRun rich("LUMINOUS_CHOIR");
        rich.run.gold = 200;
        assert(rich.run.start_event("LUMINOUS_CHOIR") && rich.run.current_event.choices[0].enabled);
        const size_t relics = rich.run.relics.size();
        const size_t deck = rich.run.master_deck.size();
        rich.pick(0);
        assert(rich.run.gold == 200 - 149 && rich.run.relics.size() == relics + 1);
        assert(rich.run.master_deck.size() == deck && rich.run.hp == 40 && !rich.run.has_deck_choice());
    }

    // Reach into the flesh: Spore Mind joins the deck and 2 cards are to be removed by the player.
    {
        EventRun flesh("LUMINOUS_CHOIR");
        const size_t deck = flesh.run.master_deck.size();
        flesh.pick(1);
        assert(flesh.run.has_deck_choice() && flesh.run.deck_choice.kind == DeckChoiceKind::REMOVE);
        assert(flesh.run.deck_choice.remaining == 2);
        assert(flesh.run.master_deck.size() == deck + 1 && flesh.run.master_deck.back().id == CardId::SPORE_MIND);
        assert(flesh.run.resolve_deck_choice(0) && flesh.run.resolve_deck_choice(0));
        assert(!flesh.run.has_deck_choice() && flesh.run.master_deck.size() == deck - 1);
        assert(flesh.run.gold == 100 && flesh.run.hp == 40 && flesh.run.relics.size() == 1);
    }

    // Leave: nothing changes.
    {
        EventRun leave("LUMINOUS_CHOIR");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(2);
        assert(leave.run.gold == 100 && leave.run.hp == 40 && leave.run.relics.size() == 1);
        assert(leave.run.master_deck.size() == deck && !leave.run.has_deck_choice());
        assert(leave.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Luminous Choir: 149 gold for a relic (greyed out without them) / 2 cards removed for Spore Mind / leave"
              << std::endl;
}
