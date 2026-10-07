// Lord's Parasol: everything the merchant sells is obtained on arrival, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

void test_lords_parasol() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("LORDS_PARASOL");
    int16_t gold = run.gold;
    size_t deck = run.master_deck.size();
    run.open_shop();
    for (const auto& c : run.current_shop.cards) assert(c.purchased);
    for (const auto& r : run.current_shop.relics) assert(r.purchased && run.has_relic(r.relic_id));
    assert(run.master_deck.size() == deck + run.current_shop.cards.size());
    assert(run.gold == gold);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = other.master_deck.size();
    other.open_shop();
    assert(other.master_deck.size() == start);
    for (const auto& c : other.current_shop.cards) assert(!c.purchased);
}

}  // namespace

int main() {
    std::cout << "=== Lord's Parasol ===" << std::endl;
    test_lords_parasol();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
