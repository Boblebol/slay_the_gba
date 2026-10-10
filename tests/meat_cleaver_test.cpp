// Meat Cleaver: the rest site offers Cook, which removes 2 cards of the deck then gives +5 Max HP.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

RunManager make_run() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 321);
    return run;
}

bool offers_cook(const RunManager& run) {
    FixedVector<CampfireAction, 5> actions = run.campfire_actions();
    for (size_t i = 0; i < actions.size(); ++i) {
        if (actions[i] == CampfireAction::COOK) return true;
    }
    return false;
}

void test_cook() {
    RunManager run = make_run();
    run.add_relic("MEAT_CLEAVER");
    assert(offers_cook(run));
    size_t before = run.master_deck.size();
    int16_t max_before = run.max_hp;
    run.phase = RunPhase::CAMPFIRE;

    assert(run.cook_at_campfire());
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.remaining == 2);
    assert(run.max_hp == max_before); // the Max HP come when the choice ends
    assert(run.resolve_deck_choice(0));
    assert(run.max_hp == max_before);
    assert(run.resolve_deck_choice(0));
    assert(!run.has_deck_choice());
    assert(run.master_deck.size() == before - 2);
    assert(run.max_hp == max_before + 5);
    assert(run.phase == RunPhase::MAP_NAVIGATION); // the rest site is used up
}

void test_eternal_refused() {
    RunManager run = make_run();
    run.add_relic("MEAT_CLEAVER");
    run.add_card_to_deck(DeckCard{ CardId::FOLLY, false });
    int folly = static_cast<int>(run.master_deck.size()) - 1;
    size_t before = run.master_deck.size();
    assert(run.cook_at_campfire());
    assert(!run.deck_choice_allows(folly));
    assert(!run.resolve_deck_choice(folly));
    assert(run.master_deck.size() == before);
    assert(run.resolve_deck_choice(0) && run.resolve_deck_choice(0));
    assert(run.master_deck.size() == before - 2);
}

void test_short_deck() {
    RunManager run = make_run();
    run.add_relic("MEAT_CLEAVER");
    // Only one removable card left: it goes, and the Max HP still come.
    while (run.master_deck.size() > 0) run.master_deck.erase(0);
    run.add_card_to_deck(DeckCard{ CardId::FOLLY, false });
    run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    int16_t max_before = run.max_hp;
    assert(run.cook_at_campfire());
    assert(run.deck_choice.remaining == 1);
    assert(run.resolve_deck_choice(1));
    assert(!run.has_deck_choice() && run.master_deck.size() == 1 && run.max_hp == max_before + 5);

    // Nothing removable at all: the Max HP come at once.
    int16_t max_now = run.max_hp;
    assert(run.cook_at_campfire());
    assert(!run.has_deck_choice() && run.max_hp == max_now + 5);
}

void test_without_relic() {
    RunManager run = make_run();
    assert(!offers_cook(run));
    size_t before = run.master_deck.size();
    int16_t max_before = run.max_hp;
    assert(!run.cook_at_campfire());
    assert(!run.has_deck_choice() && run.master_deck.size() == before && run.max_hp == max_before);
}

}  // namespace

int main() {
    std::cout << "=== Meat Cleaver ===" << std::endl;
    test_cook();
    test_eternal_refused();
    test_short_deck();
    test_without_relic();
    std::cout << "All Meat Cleaver tests passed." << std::endl;
    return 0;
}
