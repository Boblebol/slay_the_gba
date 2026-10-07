// Folly (Hubris), the first Curse card: cost X, Unplayable, Innate, Ethereal, Eternal. Same upgraded.
// Values of bible/data/card_v0.111.0.json. Also checks that an Eternal card resists every way to leave the deck.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

void test_folly_definition() {
    assert(CardDef::get(CardId::FOLLY).type == CardType::CURSE);
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::FOLLY, up) == COST_X);
        assert(CardDef::has_keyword(CardId::FOLLY, up, KEYWORD_UNPLAYABLE));
        assert(CardDef::has_keyword(CardId::FOLLY, up, KEYWORD_INNATE));
        assert(CardDef::has_keyword(CardId::FOLLY, up, KEYWORD_ETHEREAL));
        assert(CardDef::has_keyword(CardId::FOLLY, up, KEYWORD_ETERNAL));
        assert(CardDef::is_eternal(DeckCard(CardId::FOLLY, up)));
    }
    std::cout << "[PASS] Folly: X cost, Curse, Unplayable, Innate, Ethereal, Eternal, same upgraded" << std::endl;
}

void test_folly_in_combat() {
    for (bool up : { false, true }) {
        CombatEngine combat;
        combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::FOLLY, up));
        combat.player.energy = 3;
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);
        // Ethereal: exhausted when it is still in hand at the end of the turn.
        combat.end_player_turn();
        assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == CardId::FOLLY);
    }
    std::cout << "[PASS] Folly cannot be played and is exhausted at the end of the turn" << std::endl;
}

void test_folly_is_drawn_first() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.discard_pile.clear();
    combat.draw_pile.clear();
    combat.draw_pile.push_back(DeckCard(CardId::FOLLY));
    for (int i = 0; i < 9; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.begin_first_turn();
    bool in_hand = false;
    for (const DeckCard& card : combat.hand) in_hand = in_hand || card.id == CardId::FOLLY;
    assert(in_hand);
    std::cout << "[PASS] Folly is Innate: in the opening hand" << std::endl;
}

void test_eternal_blocks_removal_and_transform() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
    assert(run.add_card_to_deck(DeckCard(CardId::FOLLY)));
    const int index = static_cast<int>(run.master_deck.size()) - 1;
    const size_t size = run.master_deck.size();

    assert(!run.remove_card_from_deck(index));
    assert(!run.transform_card_in_deck(index));
    assert(run.master_deck.size() == size && run.master_deck[index].id == CardId::FOLLY);

    // Deck pickers offer neither Remove nor Transform on it.
    run.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    assert(!run.deck_choice_allows(index));
    run.request_deck_choice(DeckChoiceKind::TRANSFORM, 1);
    assert(!run.deck_choice_allows(index));

    // The shop refuses it without charging anything, and still removes a normal card.
    run.gold = 500;
    run.current_shop.card_removal_used = false;
    run.current_shop.card_removal_cost = 75;
    assert(!run.remove_card_at_shop(index));
    assert(run.gold == 500 && !run.current_shop.card_removal_used && run.master_deck.size() == size);
    assert(run.remove_card_at_shop(0));
    assert(run.gold == 425);
    std::cout << "[PASS] Eternal card refused by removal, transform and shop removal" << std::endl;
}

} // namespace

int main() {
    test_folly_definition();
    test_folly_in_combat();
    test_folly_is_drawn_first();
    test_eternal_blocks_removal_and_transform();
    return 0;
}
