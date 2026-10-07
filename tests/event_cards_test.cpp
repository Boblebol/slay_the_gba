// Event cards (cards given by events and ancients). One test per card, with the values of bible/data/card_v0.111.0.json.
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
    combat.player.energy = 3;
    return combat;
}

void test_maul() {
    assert(CardDef::get_cost(CardId::MAUL, false) == 1);
    assert(CardDef::get(CardId::MAUL).type == CardType::ATTACK);
    // Base: 5 damage twice, then every Maul gains 2.
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::MAUL));
    combat.hand.push_back(DeckCard(CardId::MAUL));
    combat.draw_pile.push_back(DeckCard(CardId::MAUL));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 190);
    assert(combat.player.energy == 2);
    assert(combat.hand[0].bonus == 2 && combat.draw_pile[0].bonus == 2);
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 190 - 14);
    assert(combat.draw_pile[0].bonus == 4);
    // Upgraded: 6 damage twice, then every Maul gains 3.
    CombatEngine up = make_fight();
    up.hand.push_back(DeckCard(CardId::MAUL, true));
    up.hand.push_back(DeckCard(CardId::MAUL, true));
    assert(CardDef::get_cost(CardId::MAUL, true) == 1);
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 188);
    assert(up.hand[0].bonus == 3);
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 188 - 18);
    std::cout << "[PASS] Maul: 5/6 damage twice, all Maul +2/+3 for the combat" << std::endl;
}

} // namespace

int main() {
    test_maul();
    return 0;
}
