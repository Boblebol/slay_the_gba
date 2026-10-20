#pragma once
// Philosophes multicolores (COLORFUL_PHILOSOPHERS): two pages, one card reward per hero color, and Equality.
#include <iostream>
#include "event_test_support.h"

inline void test_colorful_philosophers() {
    using namespace event_test;

    const EventDef* def = EventManager::find("COLORFUL_PHILOSOPHERS");
    assert(def && def->acts == EVENT_ACT_2 && def->pages.size() == 2);
    assert(def->pages[0].options.size() == 4 && def->pages[1].options.size() == 4);

    // A color: three cards of that hero's pool, on the reward screen; nothing else changes.
    auto color = [](int page_option, bool second_page, CardPool pool) {
        EventRun r("COLORFUL_PHILOSOPHERS");
        if (second_page) {
            r.pick(3);
            assert(r.run.phase == RunPhase::EVENT && r.run.current_event.page == 1);
        }
        const size_t deck = r.run.master_deck.size();
        r.pick(page_option);
        assert(r.run.phase == RunPhase::REWARD_SCREEN && r.run.current_card_rewards.size() == 3);
        for (const DeckCard& offered : r.run.current_card_rewards) assert(card_meta(offered.id).pool == pool);
        assert(r.run.master_deck.size() == deck && r.run.gold == 100 && r.run.hp == 40);
        assert(!r.run.has_relic("PRISMATIC_SHARD"));
    };
    color(0, false, CardPool::DEFECT);
    color(1, false, CardPool::IRONCLAD);
    color(2, false, CardPool::NECROBINDER);
    color(0, true, CardPool::REGENT);
    color(1, true, CardPool::SILENT);

    // Equality: the Prismatic Shard, no reward screen.
    {
        EventRun eq("COLORFUL_PHILOSOPHERS");
        eq.pick(3);
        eq.pick(2);
        assert(eq.run.phase == RunPhase::MAP_NAVIGATION && eq.run.has_relic("PRISMATIC_SHARD"));
        assert(eq.run.current_card_rewards.empty() && eq.run.gold == 100);
    }

    // "Retour" leads back to the first page.
    {
        EventRun back("COLORFUL_PHILOSOPHERS");
        back.pick(3);
        back.pick(3);
        assert(back.run.phase == RunPhase::EVENT && back.run.current_event.page == 0);
    }

    std::cout << "[PASS] Colorful Philosophers: a card reward per hero color / Prismatic Shard for Equality" << std::endl;
}
