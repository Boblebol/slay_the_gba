#pragma once
// Sphère de cristal (CRYSTAL_SPHERE): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_crystal_sphere() {
    using namespace event_test;

    const EventDef* def = EventManager::find("CRYSTAL_SPHERE");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Uncover the Future: 56 gold paid, a potion, then a card reward.
    {
        EventRun uncover("CRYSTAL_SPHERE");
        assert(uncover.run.current_event.choices[0].enabled && uncover.run.current_event.choices[1].enabled);
        const size_t deck = uncover.run.master_deck.size();
        uncover.pick(0);
        assert(uncover.run.gold == 100 - 56 && uncover.run.hp == 40 && uncover.run.max_hp == 80);
        assert(uncover.run.potions.size() == 1 && uncover.run.master_deck.size() == deck);
        assert(uncover.run.phase == RunPhase::REWARD_SCREEN && !uncover.run.current_card_rewards.empty());
    }

    // Too poor to pay: the option is greyed out, the payment plan stays open.
    {
        EventRun poor("CRYSTAL_SPHERE");
        poor.run.gold = 55;
        EventManager::open_page(poor.run, *def, 0);
        assert(!poor.run.current_event.choices[0].enabled && poor.run.current_event.choices[1].enabled);
        assert(!poor.run.choose_event_choice(0, poor.message, sizeof(poor.message)));
    }

    // Payment Plan: Debt joins the deck, 2 potions and a card reward, no gold paid.
    {
        EventRun plan("CRYSTAL_SPHERE");
        const size_t deck = plan.run.master_deck.size();
        plan.pick(1);
        assert(plan.run.master_deck.size() == deck + 1 && plan.run.master_deck.back().id == CardId::DEBT);
        assert(plan.run.gold == 100 && plan.run.hp == 40 && plan.run.potions.size() == 2);
        assert(plan.run.phase == RunPhase::REWARD_SCREEN && !plan.run.current_card_rewards.empty());
    }

    std::cout << "[PASS] Crystal Sphere: Uncover the Future (-56 gold) / Payment Plan (Debt)" << std::endl;
}
