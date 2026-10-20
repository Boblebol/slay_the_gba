#pragma once
// Le Marchand ??? (FAKE_MERCHANT): fake relics over four pages (50 gold each, guessed), or a Foul Potion thrown
// to start a fight.
#include <iostream>
#include "event_test_support.h"

inline void test_fake_merchant() {
    using namespace event_test;

    const EventDef* def = EventManager::find("FAKE_MERCHANT");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 5 && def->pages[0].options.size() == 3);

    // Every fake relic of the reference is for sale once, and each costs 50 gold.
    {
        const std::string_view all[] = { "FAKE_ANCHOR", "FAKE_BLOOD_VIAL", "FAKE_HAPPY_FLOWER", "FAKE_LEES_WAFFLE",
                                         "FAKE_MANGO", "FAKE_MERCHANTS_RUG", "FAKE_ORICHALCUM", "FAKE_SNECKO_EYE",
                                         "FAKE_STRIKE_DUMMY", "FAKE_VENERABLE_TEA_SET" };
        int sold = 0;
        for (std::string_view relic : all) {
            int found = 0;
            for (const EventPage& page : def->pages) {
                for (const EventOption& option : page.options) {
                    for (const EventEffect& effect : option.effects) found += effect.type == EventEffectType::GAIN_RELIC && effect.ref == relic;
                }
            }
            assert(found == 1);
            sold += found;
        }
        assert(sold == 10);
    }

    // Buying: page 0 -> shop, then the first relic of the first page (Anchor ???).
    {
        EventRun buy("FAKE_MERCHANT");
        assert(buy.run.current_event.choices.size() == 3 && buy.run.current_event.choices[0].enabled);
        assert(!buy.run.current_event.choices[1].enabled); // no Foul Potion on the belt
        const size_t deck = buy.run.master_deck.size();
        buy.pick(0);
        assert(buy.run.phase == RunPhase::EVENT && buy.run.current_event.page == 1 && buy.run.gold == 100);
        buy.pick(0);
        assert(buy.run.has_relic("FAKE_ANCHOR") && buy.run.gold == 50 && buy.run.master_deck.size() == deck);
        assert(buy.run.hp == 40 && buy.run.potions.empty() && buy.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Following the pages to the last relic; with too little gold every relic is greyed out and "leave" remains.
    {
        EventRun poor("FAKE_MERCHANT");
        poor.run.gold = 49;
        poor.pick(0);
        for (int page = 1; page <= 3; ++page) {
            assert(poor.run.current_event.page == page);
            assert(!poor.run.current_event.choices[0].enabled && poor.run.current_event.choices[3].enabled);
            poor.pick(3);
        }
        assert(poor.run.current_event.page == 4 && !poor.run.current_event.choices[0].enabled);
        assert(poor.run.current_event.choices[1].enabled);
        poor.pick(1);
        assert(poor.run.phase == RunPhase::MAP_NAVIGATION && poor.run.gold == 49 && poor.run.relics.size() == 1);
    }
    {
        EventRun tea("FAKE_MERCHANT");
        tea.pick(0);
        tea.pick(3);
        tea.pick(3);
        tea.pick(3);
        tea.pick(0);
        assert(tea.run.has_relic("FAKE_VENERABLE_TEA_SET") && tea.run.gold == 50);
    }

    // Leaving: nothing changes.
    {
        EventRun leave("FAKE_MERCHANT");
        leave.pick(2);
        assert(leave.run.gold == 100 && leave.run.hp == 40 && leave.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // The Foul Potion: spent, and a fight starts (the other potions stay).
    {
        EventRun throw_it("FAKE_MERCHANT");
        throw_it.run.potions.push_back("FIRE_POTION");
        throw_it.run.potions.push_back("FOUL_POTION");
        throw_it.run.start_event("FAKE_MERCHANT");
        assert(throw_it.run.current_event.choices[1].enabled);
        throw_it.pick(1);
        assert(throw_it.run.phase == RunPhase::IN_COMBAT);
        assert(throw_it.run.potions.size() == 1 && throw_it.run.potions[0] == "FIRE_POTION");
        assert(throw_it.run.gold == 100);
    }

    std::cout << "[PASS] Fake Merchant: 10 fake relics for 50 gold over 4 pages / Foul Potion thrown to fight / leave"
              << std::endl;
}
