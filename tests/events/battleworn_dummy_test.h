#pragma once
// Mannequin de combat usé (BATTLEWORN_DUMMY), Act 3: three settings, a fight of 3 turns against the dummy.
#include <iostream>
#include "event_test_support.h"

namespace {

// The fight of `setting` (1 to 3) from the event, started from an event room of the map.
inline event_test::EventRun start_dummy_fight(int setting) {
    using namespace engine;
    event_test::EventRun fight("BATTLEWORN_DUMMY");
    int room = -1;
    for (size_t i = 0; i < fight.run.map.nodes.size() && room < 0; ++i) {
        if (fight.run.map.nodes[i].type == RoomType::EVENT) room = static_cast<int>(i);
    }
    assert(room >= 0);
    fight.run.current_node_index = room;
    fight.pick(setting - 1);
    return fight;
}

} // namespace

inline void test_battleworn_dummy() {
    using namespace event_test;

    const EventDef* def = EventManager::find("BATTLEWORN_DUMMY");
    assert(def && def->acts == EVENT_ACT_3 && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    // The dummy has the HP of its setting, alone in the fight.
    const int16_t hp[3] = { 75, 150, 300 };
    for (int setting = 1; setting <= 3; ++setting) {
        EventRun fight = start_dummy_fight(setting);
        assert(fight.run.phase == RunPhase::IN_COMBAT && fight.run.combat.enemies.size() == 1);
        const Enemy& dummy = fight.run.combat.enemies[0];
        assert(dummy.type == EnemyType::BATTLEWORN_DUMMY && dummy.hp == hp[setting - 1] && dummy.max_hp == hp[setting - 1]);
        assert(dummy.current_intent.type == EnemyIntentType::BUFF);
    }

    // Left alive, it goes after the player's third turn and gives nothing.
    {
        EventRun fight = start_dummy_fight(3);
        CombatEngine& combat = fight.run.combat;
        combat.player.max_hp = combat.player.hp = 500;
        combat.end_player_turn();
        combat.end_player_turn();
        assert(combat.enemies[0].current_intent.type == EnemyIntentType::ESCAPE && combat.enemies[0].is_alive());
        combat.end_player_turn();
        assert(combat.state == CombatState::VICTORY && combat.every_enemy_escaped());
        const int gold = fight.run.gold;
        const size_t relics = fight.run.relics.size();
        fight.run.proceed_after_combat();
        assert(fight.run.phase == RunPhase::MAP_NAVIGATION && fight.run.event_fight_ref.empty());
        assert(fight.run.gold == gold && fight.run.relics.size() == relics && fight.run.potions.empty());
        assert(fight.run.current_card_rewards.empty() && !fight.run.has_deck_choice());
    }

    // Beaten, setting 1: a potion, no gold, no card.
    {
        EventRun fight = start_dummy_fight(1);
        fight.run.combat.state = CombatState::VICTORY;
        fight.run.proceed_after_combat();
        assert(fight.run.phase == RunPhase::REWARD_SCREEN && fight.run.gold_reward == 0);
        assert(fight.run.current_card_rewards.empty() && fight.run.relic_reward.empty() && !fight.run.has_deck_choice());
        assert(!fight.run.potion_reward.empty() && fight.run.take_potion_reward() && fight.run.potions.size() == 1);
        assert(fight.run.event_fight_ref.empty());
    }

    // Beaten, setting 2: 2 cards of the deck to upgrade, no other reward.
    {
        EventRun fight = start_dummy_fight(2);
        fight.run.combat.state = CombatState::VICTORY;
        fight.run.proceed_after_combat();
        assert(fight.run.phase == RunPhase::REWARD_SCREEN && fight.run.has_deck_choice());
        assert(fight.run.deck_choice.kind == DeckChoiceKind::UPGRADE && fight.run.deck_choice.remaining == 2);
        assert(fight.run.potion_reward.empty() && fight.run.relic_reward.empty() && fight.run.current_card_rewards.empty());
        assert(fight.run.resolve_deck_choice(0) && fight.run.resolve_deck_choice(1));
        assert(fight.upgraded_cards() == 2 && !fight.run.has_deck_choice());
    }

    // Beaten, setting 3: a relic.
    {
        EventRun fight = start_dummy_fight(3);
        const size_t relics = fight.run.relics.size();
        fight.run.combat.state = CombatState::VICTORY;
        fight.run.proceed_after_combat();
        assert(fight.run.phase == RunPhase::REWARD_SCREEN && !fight.run.relic_reward.empty());
        assert(fight.run.potion_reward.empty() && fight.run.current_card_rewards.empty() && fight.run.gold_reward == 0);
        assert(fight.run.take_relic_reward() && fight.run.relics.size() == relics + 1);
    }

    std::cout << "[PASS] Battleworn Dummy: 75 / 150 / 300 HP in 3 turns for a potion / 2 upgrades / a relic, nothing if it leaves"
              << std::endl;
}
