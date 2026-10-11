// Enemy summons and Minions (#221): a SUMMON move brings enemies into the fight, and Minions flee without their
// leader.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// A Jaw Worm whose next move summons `count` Tough Eggs, Minions of it when `minions` is set.
CombatEngine summoner(uint8_t count = 1, bool minions = true) {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.enemies[0].hp = combat.enemies[0].max_hp = 200;
    EnemyIntent summon;
    summon.type = EnemyIntentType::SUMMON;
    summon.summon_type = EnemyType::TOUGH_EGG;
    summon.summons = count;
    summon.summon_minions = minions;
    summon.description = "Ponte";
    combat.enemies[0].current_intent = summon;
    return combat;
}

void test_summon_intent() {
    CombatEngine combat = summoner();
    const int16_t hp = combat.player.hp;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    // The egg is in the fight, alive, a Minion of the worm.
    assert(combat.enemies.size() == 2);
    const Enemy& egg = combat.enemies[1];
    assert(egg.type == EnemyType::TOUGH_EGG && egg.is_alive());
    assert(egg.hp >= 14 && egg.hp <= 18 && egg.hp == egg.max_hp);
    assert(egg.minion && egg.leader == 0);
    // It did not act on the turn it came: the move of the worm did nothing to the player.
    assert(combat.player.hp == hp);
    assert(combat.last_enemy_actions.size() == 1);
    assert(combat.last_enemy_actions[0].intent_type == EnemyIntentType::SUMMON);
    assert(combat.last_enemy_actions[0].summoned == 0b10);
    // Its first move is planned with the others: Hatch, then Nibble 4.
    assert(egg.current_intent.type == EnemyIntentType::SUMMON && egg.current_intent.summons == 0);
    combat.player.block = 0;
    combat.end_player_turn();
    assert(combat.last_enemy_actions.size() == 2 && combat.last_enemy_actions[1].enemy_index == 1);
    assert(combat.enemies.size() == 2);
    assert(combat.enemies[1].current_intent.type == EnemyIntentType::ATTACK);
    assert(combat.enemies[1].intent_damage(combat.player) == 4);
    // It can be targeted and hit like any enemy.
    assert(combat.count_alive_enemies() == 2);
    std::cout << "[PASS] Summon: the move brings a Tough Egg in, which acts from the next enemy turn" << std::endl;
}

void test_summon_room() {
    // Three places at most: a move that summons three eggs brings two.
    CombatEngine combat = summoner(3);
    combat.end_player_turn();
    assert(combat.enemies.size() == 3 && combat.count_alive_enemies() == 3);
    assert(combat.last_enemy_actions[0].summoned == 0b110);

    // No place left: nothing comes.
    combat.enemies[0].current_intent = summoner().enemies[0].current_intent;
    combat.end_player_turn();
    assert(combat.enemies.size() == 3 && combat.last_enemy_actions[0].summoned == 0);

    // A dead enemy's place is taken again, by a fresh enemy.
    combat.damage_enemy(combat.enemies[1], 999);
    assert(!combat.enemies[1].is_alive() && combat.enemies[2].is_alive());
    combat.enemies[0].current_intent = summoner().enemies[0].current_intent;
    combat.end_player_turn();
    assert(combat.last_enemy_actions[0].summoned == 0b10);
    assert(combat.enemies[1].is_alive() && combat.enemies[1].type == EnemyType::TOUGH_EGG);
    assert(combat.enemies[1].hp == combat.enemies[1].max_hp && combat.enemies[1].block == 0);
    assert(combat.enemies[1].current_intent.type == EnemyIntentType::SUMMON); // its pattern starts again
    std::cout << "[PASS] Summon: 3 enemies at most, a dead enemy's place is taken again" << std::endl;
}

void test_minions_flee_without_leader() {
    // Killed by a card: its Minions flee, the fight is won, and it is not "everyone fled" (rewards follow).
    CombatEngine combat = summoner(2);
    combat.end_player_turn();
    assert(combat.count_alive_enemies() == 3);
    combat.enemies[0].hp = 1;
    combat.enemies[0].block = 0;
    combat.player.energy = 3;
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::STRIKE, false));
    assert(combat.play_card(0, 0));
    assert(!combat.enemies[0].is_alive() && !combat.enemies[0].escaped);
    for (size_t i = 1; i < 3; ++i) assert(!combat.enemies[i].is_alive() && combat.enemies[i].escaped);
    assert(combat.state == CombatState::VICTORY);
    assert(!combat.every_enemy_escaped());

    // A leader that runs away takes its Minions with it.
    combat = summoner(1);
    combat.end_player_turn();
    combat.enemies[0].current_intent = EnemyIntent{};
    combat.enemies[0].current_intent.type = EnemyIntentType::ESCAPE;
    combat.end_player_turn();
    assert(combat.enemies[0].escaped && combat.enemies[1].escaped);
    assert(combat.state == CombatState::VICTORY && combat.every_enemy_escaped());

    // A Minion whose leader comes after it in the list flees too.
    combat = summoner(1);
    combat.end_player_turn();
    combat.enemies[0].minion = true;
    combat.enemies[0].leader = 1;
    combat.enemies[1].minion = false;
    combat.enemies[1].leader = -1;
    combat.damage_enemy(combat.enemies[1], 999);
    assert(combat.enemies[0].escaped && !combat.enemies[0].is_alive());

    // Summoned enemies that are not Minions stay.
    combat = summoner(1, false);
    combat.end_player_turn();
    assert(!combat.enemies[1].minion && combat.enemies[1].leader == -1);
    combat.damage_enemy(combat.enemies[0], 999);
    assert(combat.enemies[1].is_alive() && combat.has_alive_enemies());
    std::cout << "[PASS] Minion: flees the fight when its leader dies or runs away" << std::endl;
}

void test_summon_ascension() {
    // A summoned egg gets the Ascension of the room: 15 to 19 HP and Nibble 5.
    CombatEngine combat = summoner();
    combat.enemy_hp_percent = 110;
    combat.enemy_damage_percent = 110;
    combat.end_player_turn();
    const Enemy& egg = combat.enemies[1];
    assert(egg.hp >= 15 && egg.hp <= 19 && egg.hp == egg.max_hp);
    combat.end_player_turn();
    assert(combat.enemies[1].intent_damage(combat.player) == 5);
    std::cout << "[PASS] Summon: a summoned enemy gets the Ascension of the room" << std::endl;
}

} // namespace

int main() {
    test_summon_intent();
    test_summon_room();
    test_minions_flee_without_leader();
    test_summon_ascension();
    return 0;
}
