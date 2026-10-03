// Engine fixes: rules that used to break silently, one test per fix.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

void test_enemy_buff_does_not_leak() {
    // Each move sets the whole intent: Strength only comes with a buff move, never with the attacks after it.
    for (EnemyType type : { EnemyType::JAW_WORM, EnemyType::LOUSE, EnemyType::GREMLIN_NOB, EnemyType::CEREMONIAL_BEAST }) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, type);
        combat.player.max_hp = 999;
        combat.player.hp = 999;
        Enemy& foe = combat.enemies[0];
        int16_t expected = 0;
        for (int turn = 1; turn <= 6; ++turn) {
            const EnemyIntent intent = foe.current_intent;
            if (intent.type == EnemyIntentType::BUFF) {
                assert(intent.status_type == StatusType::STRENGTH && intent.status_amount > 0);
                expected = static_cast<int16_t>(expected + intent.status_amount);
            } else {
                assert(intent.status_type == StatusType::NONE && intent.status_amount == 0);
            }
            combat.hand.clear();
            combat.end_player_turn();
            assert(combat.state == CombatState::PLAYER_TURN);
            assert(foe.get_status(StatusType::STRENGTH) == expected);
        }
        // Gremlin Nob and the Beast roar once, the Jaw Worm bellows on turns 3 and 5, the Louse never buffs.
        int16_t final_strength = type == EnemyType::JAW_WORM ? 6 : type == EnemyType::LOUSE ? 0 : 2;
        assert(expected == final_strength);
    }
    std::cout << "[PASS] Enemy intents: Jaw Worm, Louse, Gremlin Nob and the Beast gain Strength only on their buff moves" << std::endl;
}

} // namespace

int main() {
    test_enemy_buff_does_not_leak();
    std::cout << "All engine fix tests passed." << std::endl;
    return 0;
}
