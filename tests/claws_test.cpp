// Claws: on pickup, transform up to 6 chosen cards into Maul.
#include <cassert>
#include "engine/run.h"

using namespace engine;

namespace {

int count_maul(const RunManager& run) {
    int n = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::MAUL) ++n;
    }
    return n;
}

void test_claws_transforms_six() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 11);
    size_t size = run.master_deck.size();
    run.add_relic("CLAWS");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
    assert(run.deck_choice.remaining == 6 && run.deck_choice.optional);
    for (int i = 0; i < 6; ++i) assert(run.resolve_deck_choice(i));
    assert(!run.has_deck_choice());
    assert(count_maul(run) == 6 && run.master_deck.size() == size);
    for (int i = 0; i < 6; ++i) assert(!run.master_deck[i].upgraded);
}

void test_claws_can_stop_early() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 12);
    run.add_relic("CLAWS");
    assert(run.resolve_deck_choice(0));
    assert(run.skip_deck_choice());
    assert(count_maul(run) == 1);
}

void test_other_transform_stays_random() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 13);
    run.request_deck_choice(DeckChoiceKind::TRANSFORM, 1);
    assert(run.deck_choice.transform_into == CardId::NONE);
    assert(run.resolve_deck_choice(0));
    assert(count_maul(run) == 0);
}

void test_no_relic_no_effect() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 14);
    assert(!run.has_deck_choice() && count_maul(run) == 0);
}

}  // namespace

int main() {
    test_claws_transforms_six();
    test_claws_can_stop_early();
    test_other_transform_stays_random();
    test_no_relic_no_effect();
    return 0;
}
