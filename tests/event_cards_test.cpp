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

// A fight against a Jaw Worm with 200 HP, so that the damage can be read from its HP.
CombatEngine make_big_fight() {
    CombatEngine combat = make_fight();
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    return combat;
}

void test_maul() {
    assert(CardDef::get_cost(CardId::MAUL, false) == 1);
    assert(CardDef::get(CardId::MAUL).type == CardType::ATTACK);
    // Base: 5 damage twice, then every Maul gains 2.
    CombatEngine combat = make_big_fight();
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
    CombatEngine up = make_big_fight();
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
    test_apparition();
    test_maul();
    return 0;
}
