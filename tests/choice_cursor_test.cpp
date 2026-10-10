// Cursor of a card choice: it starts on, and moves between, the cards that can be picked (#869).
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    return combat;
}

// Dual Wield copies an Attack or a Power: a Defend is not eligible.
void test_hand_choice_skips_ineligible_cards() {
    CombatEngine combat = make_fight();
    combat.player.energy = 3;
    combat.hand.push_back(DeckCard(CardId::DUAL_WIELD));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.choice_pending());
    // The Dual Wield left the hand: Defend, Defend, Strike, Strike, Defend.
    const std::span<const uint8_t> none;
    assert(!combat.choice_allows(0) && combat.choice_allows(2) && combat.choice_allows(3));
    assert(combat.next_pickable_choice(none, 0, 1, true) == 2);
    assert(combat.next_pickable_choice(none, 4, 1, true) == 2); // wraps past the last Defend
    assert(combat.next_pickable_choice(none, 1, -1, true) == 3); // wraps backwards
    assert(combat.next_pickable_choice(none, 4, 1, false) == -1);
    assert(combat.next_pickable_choice(none, 1, -1, false) == -1);
    assert(combat.next_pickable_choice(none, 3, -1, false) == 3);
    std::cout << "[PASS] Hand choice: the cursor lands on the first eligible card" << std::endl;
}

// A pile choice shows the cards in a sorted order: positions follow it, not the pile indices.
void test_pile_choice_follows_the_view_order() {
    CombatEngine combat = make_fight();
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.ask_pile_choice(Pile::DRAW, ChoiceKind::TO_HAND);
    assert(combat.choice_pending());
    // Every card of the pile can be taken: position 0 is the first of the view order, whatever its pile index.
    const uint8_t order[] = { 2, 0, 1 };
    assert(combat.next_pickable_choice(order, 0, 1, false) == 0);
    assert(combat.next_pickable_choice(order, 2, 1, false) == 2);
    assert(combat.next_pickable_choice(order, 3, 1, false) == -1);
    std::cout << "[PASS] Choice order: positions go through the given view order" << std::endl;
}

void test_no_choice_pending() {
    CombatEngine combat = make_fight();
    assert(combat.next_pickable_choice({}, 0, 1, true) == -1);
    std::cout << "[PASS] No choice: nothing to pick" << std::endl;
}

} // namespace

int main() {
    test_hand_choice_skips_ineligible_cards();
    test_pile_choice_follows_the_view_order();
    test_no_choice_pending();
    return 0;
}
