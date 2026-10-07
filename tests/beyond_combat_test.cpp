// Card effects that outlive the fight (#94): Max HP lost for the rest of the run, and a stunned enemy.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

RunManager make_fight_run() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    return run;
}

void test_lose_max_hp_lasts_for_the_run() {
    RunManager run = make_fight_run();
    const int16_t max_hp = run.max_hp;
    run.combat.player.lose_max_hp(2);
    assert(run.combat.player.max_hp == max_hp - 2);
    assert(run.combat.player.hp == max_hp - 2); // a full-HP hero loses the HP above the new maximum
    run.proceed_after_combat();
    assert(run.max_hp == max_hp - 2);
    assert(run.hp <= run.max_hp);
    std::cout << "[PASS] Max HP lost in combat stays lost for the run" << std::endl;
}

void test_lose_max_hp_bounds() {
    CombatEngine combat;
    combat.start_combat(EnemyType::SEWER_CLAM, CharacterId::IRONCLAD);
    combat.player.hp = 10;
    const int16_t max_hp = combat.player.max_hp;
    combat.player.lose_max_hp(0);
    combat.player.lose_max_hp(-5);
    assert(combat.player.max_hp == max_hp && combat.player.hp == 10); // nothing to lose
    combat.player.lose_max_hp(2);
    assert(combat.player.max_hp == max_hp - 2 && combat.player.hp == 10); // wounded hero keeps its HP
    combat.player.lose_max_hp(999);
    assert(combat.player.max_hp == 1 && combat.player.hp == 1); // never below 1
    std::cout << "[PASS] Max HP loss: no-op on 0, keeps HP below the cap, floors at 1" << std::endl;
}

void test_stunned_enemy_skips_its_move() {
    CombatEngine combat;
    combat.start_combat(EnemyType::SEWER_CLAM, CharacterId::IRONCLAD);
    Enemy& clam = combat.enemies[0];
    assert(clam.current_intent.type == EnemyIntentType::ATTACK);
    clam.stun();
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    assert(combat.last_enemy_actions[0].intent_type == EnemyIntentType::STUN);
    assert(clam.current_intent.type != EnemyIntentType::STUN); // only one turn is lost
    std::cout << "[PASS] Stunned enemy skips its next move" << std::endl;
}

} // namespace

int main() {
    test_lose_max_hp_lasts_for_the_run();
    test_lose_max_hp_bounds();
    test_stunned_enemy_skips_its_move();
    return 0;
}
