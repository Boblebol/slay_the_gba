#include <iostream>
#include <cassert>
#include "engine/combat.h"

using namespace engine;

int main() {
    std::cout << "=== Slay The GBA Engine Test ===" << std::endl;

    CombatEngine combat;
    combat.start_combat(EnemyType::CULTIST);

    assert(combat.player.hp == 80);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    assert(combat.enemy.hp == 50);
    std::cout << "[PASS] Combat initialized properly" << std::endl;

    // Test playing Strike or Bash if in hand
    bool played = false;
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        if (combat.hand[i] == CardId::STRIKE) {
            int16_t old_hp = combat.enemy.hp;
            bool ok = combat.play_card(i);
            assert(ok);
            assert(combat.enemy.hp == old_hp - 6);
            assert(combat.player.energy == 2);
            played = true;
            std::cout << "[PASS] Played Strike! Cultist HP: " << combat.enemy.hp << std::endl;
            break;
        }
    }
    assert(played);

    // End turn
    combat.end_player_turn();
    assert(combat.turn_number == 2);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    // Cultist had Ritual +3 in turn 1, so after turn 1 ends, Cultist should have 3 Strength!
    assert(combat.enemy.get_status(StatusType::STRENGTH) == 3);
    std::cout << "[PASS] Turn ended, Cultist executed Ritual! Strength is now: " 
              << combat.enemy.get_status(StatusType::STRENGTH) << std::endl;

    std::cout << "=== All Engine Tests Passed Successfully! ===" << std::endl;
    return 0;
}
