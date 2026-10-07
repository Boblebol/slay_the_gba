// War Hammer: killing an Elite upgrades 4 random cards of the deck.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int upgraded_count(const RunManager& run) {
    int count = 0;
    for (const DeckCard& card : run.master_deck) count += card.upgraded ? 1 : 0;
    return count;
}

void test_war_hammer() {
    for (bool with_relic : { true, false }) {
        for (RoomType room : { RoomType::ELITE, RoomType::MONSTER }) {
            RunManager run;
            run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
            if (with_relic) run.add_relic("WAR_HAMMER");
            int before = upgraded_count(run);
            run.map.nodes[0].type = room;
            run.enter_room(0);
            run.proceed_after_combat();
            int expected = with_relic && room == RoomType::ELITE ? 4 : 0;
            assert(upgraded_count(run) == before + expected);
        }
    }
    std::cout << "[PASS] War Hammer upgrades 4 cards after an Elite only" << std::endl;
}

} // namespace

int main() {
    test_war_hammer();
    return 0;
}
