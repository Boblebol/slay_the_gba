#pragma once
// À coups de poing (PUNCH_OFF): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_punch_off() {
    using namespace event_test;

    const EventDef* def = EventManager::find("PUNCH_OFF");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 2);

    // Nab: an Injury in the deck and one random relic, no fight.
    {
        EventRun nab("PUNCH_OFF");
        assert(nab.run.current_event.choices.size() == 2 && nab.run.current_event.choices[1].enabled);
        const size_t deck = nab.run.master_deck.size();
        const size_t relics = nab.run.relics.size();
        nab.pick(0);
        assert(nab.run.master_deck.size() == deck + 1 && nab.run.master_deck.back().id == CardId::INJURY);
        assert(nab.run.relics.size() == relics + 1);
        assert(nab.run.hp == 40 && nab.run.max_hp == 80 && nab.run.gold == 100 && nab.run.potions.empty());
        assert(!nab.run.has_deck_choice() && nab.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Fight: the two Punch Constructs, then the rewards with a relic.
    {
        EventRun fight("PUNCH_OFF");
        const size_t deck = fight.run.master_deck.size();
        const size_t relics = fight.run.relics.size();
        int room = -1;
        for (size_t i = 0; i < fight.run.map.nodes.size() && room < 0; ++i) {
            if (fight.run.map.nodes[i].type == RoomType::EVENT) room = static_cast<int>(i);
        }
        assert(room >= 0);
        fight.run.current_node_index = room;
        fight.pick(1);
        assert(fight.run.phase == RunPhase::IN_COMBAT && fight.run.combat.enemies.size() == 2);
        assert(fight.run.combat.enemies[0].type == EnemyType::PUNCH_CONSTRUCT);
        assert(fight.run.combat.enemies[1].type == EnemyType::PUNCH_CONSTRUCT);
        assert(fight.run.master_deck.size() == deck && fight.run.relics.size() == relics);
        fight.run.combat.state = CombatState::VICTORY;
        fight.run.proceed_after_combat();
        assert(fight.run.phase == RunPhase::REWARD_SCREEN && !fight.run.relic_reward.empty());
        assert(fight.run.take_relic_reward() && fight.run.relics.size() == relics + 1);
        assert(fight.run.event_fight_ref.empty());
    }

    std::cout << "[PASS] Punch Off: Injury and a random relic / fight 2 Punch Constructs for a relic" << std::endl;
}
