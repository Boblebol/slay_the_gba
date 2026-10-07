// Wish (event card). Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    return combat;
}

void play_wish(bool upgraded) {
    CombatEngine combat = make_fight();
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.draw_pile.push_back(DeckCard(CardId::BASH));
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::WISH, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 3);
    assert(combat.choice_pending());
    // Pick Bash (index 1) from the draw pile.
    const uint8_t pick[] = { 1 };
    assert(combat.resolve_choice(pick));
    assert(!combat.choice_pending());
    assert(combat.hand.size() == 1 && combat.hand[0].id == CardId::BASH);
    assert(combat.draw_pile.size() == 2);
    assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == CardId::WISH);
}

void test_wish() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::WISH, up) == 0);
        assert(CardDef::get(CardId::WISH).type == CardType::SKILL);
        assert(CardDef::has_keyword(CardId::WISH, up, KEYWORD_EXHAUST));
        assert(CardDef::has_keyword(CardId::WISH, up, KEYWORD_RETAIN) == up);
        play_wish(up);
    }
    std::cout << "[PASS] Wish: 0 energy, draw pile card to hand, Exhaust, Retain once upgraded" << std::endl;
}

} // namespace

int main() {
    test_wish();
    return 0;
}
