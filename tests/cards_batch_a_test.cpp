// Byrd Swoop (event) and three colorless cards, with the values of bible/data/card_v0.111.0.json.
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
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    return combat;
}

void check_attack(CardId id, int cost, int base_damage, int upgraded_damage) {
    assert(CardDef::get(id).type == CardType::ATTACK);
    assert(CardDef::get_cost(id, false) == cost && CardDef::get_cost(id, true) == cost);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(id));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - base_damage);
    assert(combat.player.energy == 3 - cost);
    CombatEngine up = make_fight();
    up.hand.push_back(DeckCard(id, true));
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 200 - upgraded_damage);
    assert(up.player.energy == 3 - cost);
}

void test_byrd_swoop() {
    check_attack(CardId::BYRD_SWOOP, 0, 14, 18);
    std::cout << "[PASS] Byrd Swoop: 0 cost, 14/18 damage" << std::endl;
}

void test_ultimate_strike() {
    check_attack(CardId::ULTIMATE_STRIKE, 1, 14, 20);
    std::cout << "[PASS] Ultimate Strike: 1 cost, 14/20 damage" << std::endl;
}

void test_ultimate_defend() {
    assert(CardDef::get(CardId::ULTIMATE_DEFEND).type == CardType::SKILL);
    assert(CardDef::get_cost(CardId::ULTIMATE_DEFEND, false) == 1);
    assert(CardDef::get_cost(CardId::ULTIMATE_DEFEND, true) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::ULTIMATE_DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 11 && combat.player.energy == 2);
    CombatEngine up = make_fight();
    up.hand.push_back(DeckCard(CardId::ULTIMATE_DEFEND, true));
    assert(up.play_card(0, 0));
    assert(up.player.block == 15 && up.player.energy == 2);
    std::cout << "[PASS] Ultimate Defend: 1 cost, 11/15 Block" << std::endl;
}

void test_finesse() {
    assert(CardDef::get(CardId::FINESSE).type == CardType::SKILL);
    assert(CardDef::get_cost(CardId::FINESSE, false) == 0);
    assert(CardDef::get_cost(CardId::FINESSE, true) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::FINESSE));
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 4 && combat.player.energy == 3);
    assert(combat.hand.size() == 1 && combat.hand[0].id == CardId::STRIKE);
    CombatEngine up = make_fight();
    up.hand.push_back(DeckCard(CardId::FINESSE, true));
    up.draw_pile.push_back(DeckCard(CardId::STRIKE));
    up.draw_pile.push_back(DeckCard(CardId::STRIKE));
    assert(up.play_card(0, 0));
    assert(up.player.block == 7 && up.player.energy == 3);
    assert(up.hand.size() == 1);
    std::cout << "[PASS] Finesse: 0 cost, 4/7 Block, draw 1" << std::endl;
}

} // namespace

int main() {
    test_byrd_swoop();
    test_ultimate_strike();
    test_ultimate_defend();
    test_finesse();
    return 0;
}
