// Distinguished Cape: adds 2 random curses and 3 Apparitions on pickup, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_id(const RunManager& run, CardId id) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == id) ++n;
    }
    return n;
}

int count_curses(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (card_meta(run.master_deck[i].id).pool == CardPool::CURSE) ++n;
    }
    return n;
}

void test_distinguished_cape() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.master_deck.size();
    int curses_before = count_curses(run);
    run.add_relic("DISTINGUISHED_CAPE");
    assert(run.has_relic("DISTINGUISHED_CAPE"));
    assert(run.master_deck.size() == before + 5);
    assert(count_curses(run) == curses_before + 2);
    assert(count_id(run, CardId::APPARITION) == 3);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = other.master_deck.size();
    other.add_relic("STRAWBERRY");
    assert(other.master_deck.size() == start);
    assert(count_id(other, CardId::APPARITION) == 0);
}

}  // namespace

int main() {
    std::cout << "=== Distinguished Cape ===" << std::endl;
    test_distinguished_cape();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
