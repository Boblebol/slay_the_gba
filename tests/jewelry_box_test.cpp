// Jewelry Box: adds one Apotheosis to the deck when obtained, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_apotheosis(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::APOTHEOSIS) ++n;
    }
    return n;
}

void test_jewelry_box() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.master_deck.size();
    assert(count_apotheosis(run) == 0);
    run.add_relic("JEWELRY_BOX");
    assert(run.has_relic("JEWELRY_BOX"));
    assert(run.master_deck.size() == before + 1);
    assert(count_apotheosis(run) == 1);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = other.master_deck.size();
    other.add_relic("STRAWBERRY");
    assert(other.master_deck.size() == start);
    assert(count_apotheosis(other) == 0);
}

}  // namespace

int main() {
    std::cout << "=== Jewelry Box ===" << std::endl;
    test_jewelry_box();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
