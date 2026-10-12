// Fights of more than three enemies (#224): up to six enemies take part, are aimed at one by one, play their turn,
// and a run brings the encounters of four.
#include <cassert>
#include <iostream>
#include <string_view>
#include "engine/combat.h"
#include "engine/encounter.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Six Jaw Worms (42 HP each, no HP drawn), and a seventh that finds no place.
CombatEngine make_crowd() {
    EnemyType types[MAX_ENEMIES + 1];
    for (EnemyType& type : types) type = EnemyType::JAW_WORM;
    CombatEngine combat;
    combat.start_combat_multi(types, CharacterId::IRONCLAD);
    return combat;
}

void play(CombatEngine& combat, CardId id, int target) {
    combat.hand.clear();
    combat.hand.push_back(DeckCard(id, false));
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

void test_capacity_is_six() {
    static_assert(MAX_ENEMIES == 6);
    CombatEngine combat = make_crowd();
    assert(combat.enemies.capacity() == MAX_ENEMIES && combat.last_enemy_actions.capacity() == MAX_ENEMIES);
    assert(combat.enemies.size() == MAX_ENEMIES);
    for (const Enemy& worm : combat.enemies) {
        assert(worm.type == EnemyType::JAW_WORM && worm.hp == 42 && worm.is_alive());
        assert(worm.current_intent.type == EnemyIntentType::ATTACK); // Chomp
    }
    assert(combat.count_alive_enemies() == MAX_ENEMIES);
    std::cout << "[PASS] Six enemies: a fight holds six, a seventh is left out" << std::endl;
}

void test_each_of_six_can_be_aimed_at() {
    for (int target = 0; target < MAX_ENEMIES; ++target) {
        CombatEngine combat = make_crowd();
        play(combat, CardId::STRIKE, target);
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            assert(combat.enemies[i].hp == (i == target ? 42 - 6 : 42));
            assert(combat.counters.hits_on_enemy[i] == (i == target ? 1 : 0));
        }
    }
    // A card that hits every enemy reaches the sixth.
    CombatEngine cleave = make_crowd();
    play(cleave, CardId::CLEAVE, -1);
    for (const Enemy& worm : cleave.enemies) assert(worm.hp == 42 - 8);
    std::cout << "[PASS] Six enemies: a Strike aimed at any of the six hits it alone, Cleave hits all six" << std::endl;
}

void test_six_enemies_play_their_turn() {
    CombatEngine combat = make_crowd();
    combat.player.max_hp = combat.player.hp = 500;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    assert(combat.last_enemy_actions.size() == MAX_ENEMIES);
    for (int i = 0; i < MAX_ENEMIES; ++i) assert(combat.last_enemy_actions[i].enemy_index == i);
    // Chomp, 11 damage each.
    assert(combat.player.hp == 500 - 6 * 11);
    std::cout << "[PASS] Six enemies: all six play their move on the enemy turn" << std::endl;
}

void test_the_last_enemy_dies_and_the_fight_ends() {
    CombatEngine combat = make_crowd();
    for (int i = 0; i < MAX_ENEMIES - 1; ++i) combat.hit(combat.enemies[i], 999);
    assert(combat.count_alive_enemies() == 1 && combat.get_first_alive_enemy_index() == MAX_ENEMIES - 1);
    combat.hit(combat.enemies[MAX_ENEMIES - 1], 999);
    assert(!combat.has_alive_enemies());
    std::cout << "[PASS] Six enemies: the sixth is the one left, the fight is won once it falls" << std::endl;
}

void test_summons_fill_up_to_six() {
    CombatEngine combat;
    FixedVector<EnemyType, MAX_ENEMIES> four;
    for (int i = 0; i < 4; ++i) four.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(four, CharacterId::IRONCLAD);
    assert(combat.summon_enemy(EnemyType::JAW_WORM, 0) == 4);
    assert(combat.summon_enemy(EnemyType::JAW_WORM, 0) == 5);
    assert(combat.enemies.size() == MAX_ENEMIES);
    assert(combat.summon_enemy(EnemyType::JAW_WORM, 0) == -1); // no place left
    combat.hit(combat.enemies[2], 999);
    assert(combat.summon_enemy(EnemyType::JAW_WORM, 0) == 2); // the place of a dead one
    assert(combat.enemies[2].is_alive() && combat.count_alive_enemies() == MAX_ENEMIES);
    std::cout << "[PASS] Six enemies: summons fill the fight up to six, then the places of the dead" << std::endl;
}

void test_a_run_meets_four_scrolls_of_biting() {
    bool met = false;
    for (uint32_t seed = 1; seed <= 200 && !met; ++seed) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        while (run.current_act < 3) run.advance_act();
        for (int step = 0; step < 20 && !met; ++step) {
            int room = -1;
            for (size_t i = 0; i < run.map.nodes.size() && room < 0; ++i) {
                const MapNode& node = run.map.nodes[i];
                if (node.type == RoomType::MONSTER &&
                    encounter_tier(node.type, static_cast<uint8_t>(node.floor + 1)) == EncounterTier::NORMAL) {
                    room = static_cast<int>(i);
                }
            }
            if (room < 0) break;
            run.enter_room(room);
            if (run.current_encounter != "ScrollsOfBitingNormal") continue;
            met = true;
            assert(run.combat.enemies.size() == 4);
            for (const Enemy& scroll : run.combat.enemies) {
                assert(scroll.type == EnemyType::SCROLL_OF_BITING && scroll.name == "Parchemord");
                assert(scroll.current_intent.type == EnemyIntentType::ATTACK);
            }
        }
    }
    assert(met);
    std::cout << "[PASS] Six enemies: a run of Act 3 meets the four Scrolls of Biting" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Six enemies ===" << std::endl;
    test_capacity_is_six();
    test_each_of_six_can_be_aimed_at();
    test_six_enemies_play_their_turn();
    test_the_last_enemy_dies_and_the_fight_ends();
    test_summons_fill_up_to_six();
    test_a_run_meets_four_scrolls_of_biting();
    std::cout << "=== All six enemies tests passed! ===" << std::endl;
    return 0;
}
