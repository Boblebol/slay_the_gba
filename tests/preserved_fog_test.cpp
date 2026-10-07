// Preserved Fog: removes 3 cards from the deck, then adds Hubris (Eternal, never removable).
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

int count_folly(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::FOLLY) ++n;
    }
    return n;
}

void test_preserved_fog() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.master_deck.size();
    run.add_relic("PRESERVED_FOG");
    assert(run.has_deck_choice());
    assert(count_folly(run) == 0);
    for (int i = 0; i < 3; ++i) {
        assert(run.has_deck_choice());
        assert(run.resolve_deck_choice(0));
    }
    assert(!run.has_deck_choice());
    assert(count_folly(run) == 1);
    assert(run.master_deck.size() == before - 3 + 1);
    // Hubris is Eternal: it cannot be removed afterwards.
    run.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::FOLLY) assert(!run.deck_choice_allows(static_cast<int>(i)));
    }

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = other.master_deck.size();
    other.add_relic("STRAWBERRY");
    assert(!other.has_deck_choice());
    assert(other.master_deck.size() == start);
    assert(count_folly(other) == 0);
}

}  // namespace

int main() {
    std::cout << "=== Preserved Fog ===" << std::endl;
    test_preserved_fog();
    std::cout << "OK" << std::endl;
    return 0;
}
