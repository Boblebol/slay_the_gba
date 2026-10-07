// Sai: an Ancient relic that gives 7 Block at the start of every turn.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

void enter_monster_room(RunManager& run) {
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
}

void test_sai() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("SAI");
    enter_monster_room(run);
    assert(run.combat.player.block == 7);

    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_monster_room(plain);
    assert(plain.combat.player.block == 0);
    std::cout << "[PASS] Sai: 7 Block at the start of the turn" << std::endl;
}

} // namespace

int main() {
    test_sai();
    return 0;
}
