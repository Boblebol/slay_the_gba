// Event cards (offered by events). One test per card, with the values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// An Ironclad against one Jaw Worm, empty piles and 3 energy.
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

void test_apparition() {
    assert(CardDef::get_cost(CardId::APPARITION, false) == 1);
    assert(CardDef::get_cost(CardId::APPARITION, true) == 1);
    assert(CardDef::get(CardId::APPARITION).type == CardType::SKILL);
    assert(CardDef::has_keyword(CardId::APPARITION, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::APPARITION, false, KEYWORD_EXHAUST));

    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::APPARITION));
    assert(combat.play_card(0, 0));
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 1);
    assert(combat.player.energy == 2);
    assert(combat.hand.empty() && combat.exhaust_pile.size() == 1);

    // Upgraded: still Intangible 1 and Exhaust, but no longer Ethereal.
    assert(!CardDef::has_keyword(CardId::APPARITION, true, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::APPARITION, true, KEYWORD_EXHAUST));
    CombatEngine upgraded = make_fight();
    upgraded.hand.push_back(DeckCard(CardId::APPARITION, true));
    assert(upgraded.play_card(0, 0));
    assert(upgraded.player.get_status(StatusType::INTANGIBLE) == 1);
    assert(upgraded.player.energy == 2);
    assert(upgraded.hand.empty() && upgraded.exhaust_pile.size() == 1);
    std::cout << "[PASS] Apparition: 1 energy, Intangible 1, Ethereal (base only), Exhaust" << std::endl;
}

} // namespace

int main() {
    test_apparition();
    return 0;
}
