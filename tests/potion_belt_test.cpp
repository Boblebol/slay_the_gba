// The potion belt as the GBA combat scene drives it: RunManager::use_potion drinks the potion at an index of the
// belt, removes it only when the engine resolved it, and the belt shows each potion's name and description.
#include <cassert>
#include <iostream>
#include <string_view>
#include "engine/potion.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A run in a fight against three enemies, with `belt` as its potions.
RunManager fight_with(std::initializer_list<std::string_view> belt) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.enter_room(run.get_available_rooms()[0]);
    assert(run.phase == RunPhase::IN_COMBAT);

    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    run.combat.start_combat_multi(pack, run.character);

    for (std::string_view potion : belt) run.potions.push_back(potion);
    return run;
}

void test_drinking_removes_the_potion_and_keeps_the_order() {
    RunManager run = fight_with({ "BLOCK_POTION", "FIRE_POTION", "STRENGTH_POTION" });
    int16_t aimed_hp = run.combat.enemies[2].hp;
    int16_t other_hp = run.combat.enemies[0].hp;

    // The potion in the middle of the belt, thrown at the enemy the target cursor picked.
    assert(run.use_potion(1, 2));
    assert(run.combat.enemies[2].hp < aimed_hp);
    assert(run.combat.enemies[0].hp == other_hp);
    assert(run.potions.size() == 2);
    assert(run.potions[0] == "BLOCK_POTION" && run.potions[1] == "STRENGTH_POTION");

    assert(run.use_potion(1));
    assert(run.combat.player.get_status(StatusType::STRENGTH) == 2);
    assert(run.potions.size() == 1 && run.potions[0] == "BLOCK_POTION");

    assert(run.use_potion(0));
    assert(run.combat.player.block == 12);
    assert(run.potions.empty());
    assert(!run.use_potion(0));
    std::cout << "[PASS] Drinking removes the potion, the others keep their order" << std::endl;
}

void test_a_potion_the_engine_does_not_resolve_stays_in_the_belt() {
    RunManager run = fight_with({ "FAIRY_IN_A_BOTTLE", "BLOCK_POTION" });
    assert(!run.use_potion(0));
    assert(run.potions.size() == 2 && run.potions[0] == "FAIRY_IN_A_BOTTLE");
    assert(run.combat.player.block == 0);
    std::cout << "[PASS] An unresolved potion stays in the belt" << std::endl;
}

void test_entropic_brew_fills_the_empty_slots() {
    // Drunk from a belt of one potion: the two free slots get random potions, the drunk one is not counted.
    RunManager run = fight_with({ "ENTROPIC_BREW", "BLOCK_POTION" });
    assert(run.use_potion(0));
    assert(run.potions.size() == run.potion_slots);
    assert(run.potions.size() == 3 && run.potions[2] != "ENTROPIC_BREW");
    assert(run.potions[0] == "BLOCK_POTION");
    std::cout << "[PASS] Entropic Brew fills the empty potion slots" << std::endl;
}

void test_nothing_is_drunk_outside_the_player_turn() {
    RunManager run = fight_with({ "BLOCK_POTION" });
    run.combat.state = CombatState::ENEMY_TURN;
    assert(!run.use_potion(0));
    assert(run.potions.size() == 1);
    std::cout << "[PASS] No potion outside the player's turn" << std::endl;
}

void test_a_killing_potion_ends_the_combat() {
    RunManager run = fight_with({ "EXPLOSIVE_AMPOULE" });
    for (Enemy& foe : run.combat.enemies) foe.hp = 5;
    assert(run.use_potion(0));
    assert(run.combat.state == CombatState::VICTORY);
    assert(run.potions.empty());
    std::cout << "[PASS] A potion that kills every enemy wins the combat" << std::endl;
}

// Every potion the engine resolves can be shown in the belt and is removed once drunk.
void test_every_resolved_potion_has_a_belt_entry() {
    int resolved = 0;
    for (const PotionData& potion : get_all_potions()) {
        assert(!potion.name.empty() && !potion.description.empty());
        assert(find_potion(potion.id) == &potion);

        RunManager run = fight_with({ potion.id });
        if (run.use_potion(0, 1)) {
            ++resolved;
            // Entropic Brew refills the belt it was drunk from; every other potion leaves it empty.
            assert(run.potions.empty() || potion.id == "ENTROPIC_BREW");
        } else {
            assert(run.potions.size() == 1);
        }
    }
    assert(resolved > 0);
    std::cout << "[PASS] " << resolved << " resolved potions have a name and a description for the belt" << std::endl;
}

} // namespace

int main() {
    test_drinking_removes_the_potion_and_keeps_the_order();
    test_a_potion_the_engine_does_not_resolve_stays_in_the_belt();
    test_entropic_brew_fills_the_empty_slots();
    test_nothing_is_drunk_outside_the_player_turn();
    test_a_killing_potion_ends_the_combat();
    test_every_resolved_potion_has_a_belt_entry();
    std::cout << "All potion belt tests passed." << std::endl;
    return 0;
}
