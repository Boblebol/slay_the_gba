// Sere Talon: lose 9 Max HP and add 3 Wishes when obtained, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_wishes(const RunManager& run) {
    int n = 0;
    for (const auto& c : run.master_deck) if (c.id == CardId::WISH) ++n;
    return n;
}

void test_sere_talon() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t max_before = run.max_hp;
    size_t deck_before = run.master_deck.size();
    assert(run.hp == run.max_hp);
    run.add_relic("SERE_TALON");
    assert(run.has_relic("SERE_TALON"));
    assert(run.max_hp == max_before - 9);
    assert(run.hp <= run.max_hp);
    assert(count_wishes(run) == 3);
    assert(run.master_deck.size() == deck_before + 3);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t start_max = other.max_hp;
    size_t start_deck = other.master_deck.size();
    other.add_relic("STRAWBERRY");
    assert(other.max_hp == start_max + 7);
    assert(other.master_deck.size() == start_deck);
    assert(count_wishes(other) == 0);
}

}  // namespace

int main() {
    std::cout << "=== Sere Talon ===" << std::endl;
    test_sere_talon();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
