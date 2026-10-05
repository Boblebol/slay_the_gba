// Cards created during a combat (never in a deck or a reward). One test per card, with the numbers of
// bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

void test_luminesce() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.hand.push_back(DeckCard(CardId::LUMINESCE));
    combat.player.energy = 3;
    assert(CardDef::get_cost(CardId::LUMINESCE, false) == 0);
    assert(CardDef::has_keyword(CardId::LUMINESCE, false, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::LUMINESCE, false, KEYWORD_EXHAUST));
    assert(combat.play_card(0, -1));
    assert(combat.player.energy == 5);
    assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == CardId::LUMINESCE);
    assert(combat.hand.empty());
    std::cout << "[PASS] Luminesce: Retain, +2 energy, Exhaust" << std::endl;
}

} // namespace

int main() {
    test_luminesce();
    return 0;
}
