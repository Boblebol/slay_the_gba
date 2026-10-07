// Storybook: adds one Brightest Flame to the deck when obtained, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_flame(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::BRIGHTEST_FLAME) ++n;
    }
    return n;
}

void test_storybook() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.master_deck.size();
    assert(count_flame(run) == 0);
    run.add_relic("STORYBOOK");
    assert(run.has_relic("STORYBOOK"));
    assert(run.master_deck.size() == before + 1);
    assert(count_flame(run) == 1);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = other.master_deck.size();
    other.add_relic("STRAWBERRY");
    assert(other.master_deck.size() == start);
    assert(count_flame(other) == 0);
}

}  // namespace

int main() {
    std::cout << "=== Storybook ===" << std::endl;
    test_storybook();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
