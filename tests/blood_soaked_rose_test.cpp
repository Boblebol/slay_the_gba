// Blood Soaked Rose: adds one Enthralled on pickup and grants 1 energy each turn; nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_enthralled(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::ENTHRALLED) ++n;
    }
    return n;
}

void enter_monster_room(RunManager& run) {
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
}

void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) {
        foe.current_intent = EnemyIntent{};
        foe.current_intent.type = EnemyIntentType::DEFEND;
    }
    combat.end_player_turn();
}

void test_blood_soaked_rose() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.master_deck.size();
    assert(count_enthralled(run) == 0);
    run.add_relic("BLOOD_SOAKED_ROSE");
    assert(run.has_relic("BLOOD_SOAKED_ROSE"));
    assert(run.master_deck.size() == before + 1);
    assert(count_enthralled(run) == 1);

    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = plain.master_deck.size();
    plain.add_relic("STRAWBERRY");
    assert(plain.master_deck.size() == start);
    assert(count_enthralled(plain) == 0);

    enter_monster_room(run);
    enter_monster_room(plain);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);
    end_turn_idle(run.combat);
    end_turn_idle(plain.combat);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);
}

}  // namespace

int main() {
    std::cout << "=== Blood Soaked Rose ===" << std::endl;
    test_blood_soaked_rose();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
