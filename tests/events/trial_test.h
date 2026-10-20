#pragma once
// Le procès (TRIAL): the two first pages, then the three trials (merchant, noble, nondescript) with both verdicts.
#include <iostream>
#include "event_test_support.h"

// One draw of the run's random stream, through a random curse that is taken back out of the deck.
inline void trial_burn_draw(engine::RunManager& run) {
    engine::EventManager::apply_effect(run, { .type = engine::EventEffectType::ADD_CURSE });
    run.master_deck.erase(run.master_deck.size() - 1);
}

// Number of random draws to burn before accepting the verdict so that the trial drawn is `page` (2 to 4).
inline int trial_draws_to_reach(int page) {
    using namespace event_test;
    for (int burn = 0; burn < 200; ++burn) {
        EventRun probe("TRIAL");
        probe.pick(0);
        for (int n = 0; n < burn; ++n) trial_burn_draw(probe.run);
        probe.pick(0);
        if (probe.run.current_event.page == page) return burn;
    }
    return -1;
}

// A run at the verdict of trial `page`, the first option of each page picked on the way.
inline void trial_open(event_test::EventRun& run, int page) {
    using namespace event_test;
    run.pick(0);
    for (int n = trial_draws_to_reach(page); n > 0; --n) trial_burn_draw(run.run);
    run.pick(0);
    assert(run.run.phase == engine::RunPhase::EVENT && run.run.current_event.page == page);
    assert(run.run.current_event.choices.size() == 2);
}

inline void test_trial() {
    using namespace event_test;

    const EventDef* def = EventManager::find("TRIAL");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 5);
    for (const EventPage& page : def->pages) assert(page.options.size() == 2);

    // Accept, then accept: the three trials can be drawn.
    assert(trial_draws_to_reach(2) >= 0 && trial_draws_to_reach(3) >= 0 && trial_draws_to_reach(4) >= 0);

    // Reject: not allowed, the event goes on with the warning like "accept".
    {
        EventRun reject("TRIAL");
        reject.pick(1);
        assert(reject.run.phase == RunPhase::EVENT && reject.run.current_event.page == 1);
        assert(reject.run.hp == 40 && reject.run.gold == 100 && reject.run.master_deck.size() == 10);
    }

    // Double Down: lethal, the run ends on the event.
    {
        EventRun doubled("TRIAL");
        doubled.pick(0);
        assert(doubled.run.current_event.page == 1);
        doubled.pick(1);
        assert(doubled.run.hp == 0 && doubled.run.phase == RunPhase::EVENT);
    }

    // Merchant, guilty: 2 relics and Regret.
    {
        EventRun run("TRIAL");
        trial_open(run, 2);
        const size_t deck = run.run.master_deck.size();
        const size_t relics = run.run.relics.size();
        run.pick(0);
        assert(run.run.relics.size() == relics + 2 && run.run.master_deck.size() == deck + 1);
        assert(run.run.master_deck.back().id == CardId::REGRET && run.run.hp == 40 && run.run.gold == 100);
        assert(run.run.phase == RunPhase::MAP_NAVIGATION && !run.run.has_deck_choice());
    }
    // Merchant, innocent: Shame and 2 cards upgraded (the player picks them).
    {
        EventRun run("TRIAL");
        trial_open(run, 2);
        const size_t deck = run.run.master_deck.size();
        const size_t relics = run.run.relics.size();
        run.pick(1);
        assert(run.run.master_deck.back().id == CardId::SHAME && run.run.master_deck.size() == deck + 1);
        assert(run.run.has_deck_choice() && run.run.deck_choice.kind == DeckChoiceKind::UPGRADE);
        assert(run.run.deck_choice.remaining == 2);
        assert(run.run.resolve_deck_choice(0) && run.run.resolve_deck_choice(1));
        assert(run.upgraded_cards() == 2 && !run.run.has_deck_choice() && run.run.relics.size() == relics);
    }
    // Noble, guilty: 10 HP.
    {
        EventRun run("TRIAL");
        trial_open(run, 3);
        const size_t deck = run.run.master_deck.size();
        run.pick(0);
        assert(run.run.hp == 40 + 10 && run.run.gold == 100 && run.run.master_deck.size() == deck);
        assert(run.run.phase == RunPhase::MAP_NAVIGATION && run.run.relics.size() == 1);
    }
    // Noble, innocent: 300 gold and Regret.
    {
        EventRun run("TRIAL");
        trial_open(run, 3);
        const size_t deck = run.run.master_deck.size();
        run.pick(1);
        assert(run.run.gold == 100 + 300 && run.run.hp == 40);
        assert(run.run.master_deck.size() == deck + 1 && run.run.master_deck.back().id == CardId::REGRET);
    }
    // Nondescript, guilty: Doubt, then 2 card rewards in a row.
    {
        EventRun run("TRIAL");
        trial_open(run, 4);
        const size_t deck = run.run.master_deck.size();
        run.pick(0);
        assert(run.run.master_deck.size() == deck + 1 && run.run.master_deck.back().id == CardId::DOUBT);
        assert(run.run.phase == RunPhase::REWARD_SCREEN && run.run.current_card_rewards.size() == 3);
        assert(run.run.extra_card_rewards == 1);
        run.run.pick_card_reward(0);
        assert(run.run.phase == RunPhase::REWARD_SCREEN && run.run.current_card_rewards.size() == 3);
        run.run.pick_card_reward(0);
        assert(run.run.master_deck.size() == deck + 3 && run.run.phase == RunPhase::MAP_NAVIGATION);
    }
    // Nondescript, innocent: Doubt and 2 cards transformed (the player picks them).
    {
        EventRun run("TRIAL");
        trial_open(run, 4);
        const size_t deck = run.run.master_deck.size();
        run.pick(1);
        assert(run.run.master_deck.size() == deck + 1 && run.run.master_deck.back().id == CardId::DOUBT);
        assert(run.run.has_deck_choice() && run.run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
        assert(run.run.deck_choice.remaining == 2);
        assert(run.run.resolve_deck_choice(0) && run.run.resolve_deck_choice(0));
        assert(!run.run.has_deck_choice() && run.run.master_deck.size() == deck + 1);
    }

    std::cout << "[PASS] Trial: accept / reject (forbidden) / double down (lethal), 3 trials x 2 verdicts" << std::endl;
}
