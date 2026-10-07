// Event cards. One test per card, with the values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// An Ironclad against one Jaw Worm with 200 HP, empty piles and 3 energy.
CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.player.energy = 3;
    return combat;
}

void check_apotheosis(bool upgraded) {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::APOTHEOSIS, upgraded) == 2);
    assert(CardDef::get(CardId::APOTHEOSIS).type == CardType::SKILL);
    assert(CardDef::get(CardId::APOTHEOSIS).keywords & KEYWORD_INNATE);
    assert(CardDef::get(CardId::APOTHEOSIS).keywords & KEYWORD_EXHAUST);
    combat.hand.push_back(DeckCard(CardId::APOTHEOSIS, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.discard_pile.push_back(DeckCard(CardId::BASH));
    combat.exhaust_pile.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 1);
    assert(combat.exhaust_pile.size() == 2);
    assert(combat.hand.size() == 1 && combat.hand[0].upgraded);
    assert(combat.draw_pile[0].upgraded);
    assert(combat.discard_pile[0].upgraded);
    assert(combat.exhaust_pile[0].upgraded);
}

void test_apotheosis() {
    check_apotheosis(false);
    check_apotheosis(true);
    std::cout << "[PASS] Apotheosis: 2 energy, Innate, upgrades all cards, Exhaust" << std::endl;
}

} // namespace

int main() {
    test_apotheosis();
    return 0;
}
