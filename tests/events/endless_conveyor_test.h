#pragma once
// Convoyeur sans fin (ENDLESS_CONVEYOR): the observe option, then each dish of the belt once, on its own run.
#include <iostream>
#include "event_test_support.h"
#include "engine/card_meta.h"

inline void test_endless_conveyor() {
    using namespace event_test;

    const EventDef* def = EventManager::find("ENDLESS_CONVEYOR");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 7);
    assert(def->pages[0].options.size() == 1);
    for (const EventPage& page : def->pages) assert(page.options.size() <= MAX_EVENT_OPTIONS);

    // A run on the first belt page, `gold` gold in the purse (the observe option is skipped).
    auto belt = [](EventRun& r, int16_t gold, int page) {
        r.run.gold = gold;
        EventManager::open_page(r.run, *EventManager::find("ENDLESS_CONVEYOR"), static_cast<uint8_t>(page));
    };
    auto count_of = [](const RunManager& run, CardId id) {
        int n = 0;
        for (const DeckCard& card : run.master_deck) n += card.id == id;
        return n;
    };

    // Observe the chef: one random upgrade, then the belt.
    {
        EventRun observe("ENDLESS_CONVEYOR");
        assert(observe.run.current_event.choices.size() == 1 && observe.run.current_event.page == 0);
        observe.pick(0);
        assert(observe.upgraded_cards() == 1 && observe.run.gold == 100 && observe.run.hp == 40);
        assert(observe.run.phase == RunPhase::EVENT && observe.run.current_event.page == 1);
        assert(observe.run.current_event.choices.size() == 4);
    }

    // Belt 1: Caviar (+4 Max HP), Clam Roll (+10 HP), Fried Eel (a Colorless card), each for 40 gold; the belt stays.
    {
        EventRun caviar("ENDLESS_CONVEYOR");
        belt(caviar, 130, 1);
        assert(caviar.run.current_event.choices[0].enabled);
        caviar.pick(0);
        assert(caviar.run.gold == 90 && caviar.run.max_hp == 84 && caviar.run.hp == 44);
        assert(caviar.run.phase == RunPhase::EVENT && caviar.run.current_event.page == 1);
        caviar.pick(0);
        caviar.pick(0);
        assert(caviar.run.gold == 10 && caviar.run.max_hp == 92);
        // 10 gold left: every dish is greyed out, the way on is not.
        for (int i = 0; i < 3; ++i) assert(!caviar.run.current_event.choices[i].enabled);
        assert(caviar.run.current_event.choices[3].enabled);
        assert(!caviar.run.choose_event_choice(0));

        EventRun clam("ENDLESS_CONVEYOR");
        belt(clam, 100, 1);
        clam.pick(1);
        assert(clam.run.gold == 60 && clam.run.hp == 50 && clam.run.max_hp == 80 && clam.run.current_event.page == 1);

        EventRun eel("ENDLESS_CONVEYOR");
        belt(eel, 100, 1);
        const size_t deck = eel.run.master_deck.size();
        eel.pick(2);
        assert(eel.run.gold == 60 && eel.run.master_deck.size() == deck + 1);
        assert(card_meta(eel.run.master_deck.back().id).pool == CardPool::COLORLESS && eel.run.current_event.page == 1);
    }

    // Belt 2: Golden Fysh (free, +75 gold, then the belt without it), Jelly Liver (transform a chosen card), Seapunk Salad.
    {
        EventRun golden("ENDLESS_CONVEYOR");
        belt(golden, 0, 2);
        assert(golden.run.current_event.choices[0].enabled && !golden.run.current_event.choices[1].enabled);
        golden.pick(0);
        assert(golden.run.gold == 75 && golden.run.current_event.page == 5 && golden.run.current_event.choices.size() == 3);
        for (int i = 0; i < golden.run.current_event.choices.size(); ++i) {
            assert(golden.run.current_event.choices[i].label != "Poisson d'or");
        }
        // The last page of the belt leads back to the belt without the Fysh.
        golden.pick(2);
        assert(golden.run.current_event.page == 6);
        golden.pick(2);
        assert(golden.run.current_event.page == 4 && golden.run.current_event.choices.size() == 4);

        EventRun jelly("ENDLESS_CONVEYOR");
        belt(jelly, 100, 2);
        jelly.pick(1);
        assert(jelly.run.gold == 60 && jelly.run.deck_choice.kind == DeckChoiceKind::TRANSFORM && jelly.run.deck_choice.remaining == 1);
        const CardId before = jelly.run.master_deck[0].id;
        assert(jelly.run.resolve_deck_choice(0) && jelly.run.master_deck[0].id != before);
        assert(jelly.run.current_event.page == 2);

        EventRun salad("ENDLESS_CONVEYOR");
        belt(salad, 100, 2);
        salad.pick(2);
        assert(salad.run.gold == 60 && count_of(salad.run, CardId::FEEDING_FRENZY) == 1 && salad.run.current_event.page == 2);
    }

    // Belt 3: Spicy Snappy (a random upgrade), Suspicious Condiment (a potion), back to belt 1, leave.
    {
        EventRun snappy("ENDLESS_CONVEYOR");
        belt(snappy, 100, 3);
        snappy.pick(0);
        assert(snappy.run.gold == 60 && snappy.upgraded_cards() == 1 && snappy.run.current_event.page == 3);

        EventRun condiment("ENDLESS_CONVEYOR");
        belt(condiment, 100, 3);
        assert(condiment.run.potions.empty());
        condiment.pick(1);
        assert(condiment.run.gold == 60 && condiment.run.potions.size() == 1 && condiment.run.current_event.page == 3);

        EventRun back("ENDLESS_CONVEYOR");
        belt(back, 100, 3);
        back.pick(2);
        assert(back.run.current_event.page == 1 && back.run.gold == 100 && back.run.phase == RunPhase::EVENT);

        EventRun leave("ENDLESS_CONVEYOR");
        belt(leave, 100, 3);
        const size_t deck = leave.run.master_deck.size();
        leave.pick(3);
        assert(leave.run.phase == RunPhase::MAP_NAVIGATION && leave.run.gold == 100 && leave.run.hp == 40);
        assert(leave.run.master_deck.size() == deck && leave.run.potions.empty());
    }

    std::cout << "[PASS] Endless Conveyor: observe (upgrade) / 7 dishes at 40 gold (greyed out without it) / free Golden Fysh once / leave"
              << std::endl;
}
