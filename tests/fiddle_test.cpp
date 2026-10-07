// Fiddle: two more cards in every opening draw, and no draw during the turn.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

RunManager start_fight(bool with_fiddle) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 2024);
    if (with_fiddle) run.add_relic("FIDDLE");
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    return run;
}

void test_fiddle() {
    RunManager plain = start_fight(false);
    assert(!plain.combat.has_power(PowerId::FIDDLE));
    assert(plain.combat.hand.size() == 5);
    assert(plain.combat.draw_card());

    RunManager run = start_fight(true);
    assert(run.combat.has_power(PowerId::FIDDLE));
    assert(run.combat.hand.size() == 7);
    assert(!run.combat.draw_card());
    assert(run.combat.hand.size() == 7);
    std::cout << "[PASS] Fiddle: 2 more cards at the start of the turn, no draw afterwards, nothing without it" << std::endl;
}

} // namespace

int main() {
    test_fiddle();
    return 0;
}
