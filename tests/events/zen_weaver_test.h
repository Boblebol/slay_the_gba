#pragma once
// Tisseur zen (ZEN_WEAVER): each option is chosen on its own run and what it does to the run is checked.
#include <iostream>
#include "event_test_support.h"

namespace zen_weaver_test {

// The event opened on a run with `gold` gold.
inline void reopen(event_test::EventRun& r, int gold) {
    r.run.gold = static_cast<int16_t>(gold);
    const bool ok = r.run.start_event("ZEN_WEAVER");
    assert(ok);
    (void)ok;
}

} // namespace zen_weaver_test

inline void test_zen_weaver() {
    using namespace event_test;

    const EventDef* def = EventManager::find("ZEN_WEAVER");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 1 && def->pages[0].options.size() == 4);

    // With 100 gold only the breathing techniques and leaving are open.
    {
        EventRun poor("ZEN_WEAVER");
        const auto& choices = poor.run.current_event.choices;
        assert(choices.size() == 4 && choices[0].enabled && !choices[1].enabled && !choices[2].enabled && choices[3].enabled);
        assert(!poor.run.choose_event_choice(1, poor.message, sizeof(poor.message)));
    }

    // Breathing techniques: 50 gold for 2 Enlightenment.
    {
        EventRun breathing("ZEN_WEAVER");
        const size_t deck = breathing.run.master_deck.size();
        breathing.pick(0);
        assert(breathing.run.gold == 50 && breathing.run.master_deck.size() == deck + 2);
        assert(breathing.run.master_deck[deck].id == CardId::ENLIGHTENMENT && breathing.run.master_deck[deck + 1].id == CardId::ENLIGHTENMENT);
        assert(breathing.run.hp == 40 && !breathing.run.has_deck_choice() && breathing.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Emotional awareness: 125 gold, one card removed by the player.
    {
        EventRun emotional("ZEN_WEAVER");
        zen_weaver_test::reopen(emotional, 200);
        const size_t deck = emotional.run.master_deck.size();
        assert(emotional.run.current_event.choices[1].enabled && !emotional.run.current_event.choices[2].enabled);
        emotional.pick(1);
        assert(emotional.run.gold == 200 - 125);
        assert(emotional.run.has_deck_choice() && emotional.run.deck_choice.kind == DeckChoiceKind::REMOVE);
        assert(emotional.run.deck_choice.remaining == 1 && emotional.run.resolve_deck_choice(0));
        assert(emotional.run.master_deck.size() == deck - 1 && !emotional.run.has_deck_choice());
    }

    // Arachnid acupuncture: 250 gold, two cards removed by the player.
    {
        EventRun acupuncture("ZEN_WEAVER");
        zen_weaver_test::reopen(acupuncture, 300);
        const size_t deck = acupuncture.run.master_deck.size();
        assert(acupuncture.run.current_event.choices[2].enabled);
        acupuncture.pick(2);
        assert(acupuncture.run.gold == 300 - 250);
        assert(acupuncture.run.has_deck_choice() && acupuncture.run.deck_choice.remaining == 2);
        assert(acupuncture.run.resolve_deck_choice(0) && acupuncture.run.resolve_deck_choice(0));
        assert(acupuncture.run.master_deck.size() == deck - 2 && !acupuncture.run.has_deck_choice());
    }

    // Leave: nothing changes.
    {
        EventRun leave("ZEN_WEAVER");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(3);
        assert(leave.run.gold == 100 && leave.run.hp == 40 && leave.run.master_deck.size() == deck);
        assert(leave.run.relics.size() == 1 && !leave.run.has_deck_choice() && leave.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Zen Weaver: 50 gold for 2 Enlightenment / 125 gold, remove 1 / 250 gold, remove 2 / leave" << std::endl;
}
