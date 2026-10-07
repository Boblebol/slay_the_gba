// Flash of Steel (#399), Peck (#425), Production (#389), Master of Strategy (#387).
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
    combat.exhaust_pile.clear();
    combat.player.energy = 3;
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    return combat;
}

void test_flash_of_steel(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::FLASH_OF_STEEL, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::FLASH_OF_STEEL, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - damage);
    assert(combat.hand.size() == 1);
    assert(combat.player.energy == 3);
}

void test_peck(bool upgraded, int hits) {
    assert(CardDef::get_cost(CardId::PECK, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::PECK, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - 2 * hits);
    assert(combat.player.energy == 2);
}

void test_production(bool upgraded, int energy) {
    assert(CardDef::get_cost(CardId::PRODUCTION, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::PRODUCTION, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 3 + energy);
    assert(combat.exhaust_pile.size() == 1);
}

void test_master_of_strategy(bool upgraded, int draws) {
    assert(CardDef::get_cost(CardId::MASTER_OF_STRATEGY, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::MASTER_OF_STRATEGY, upgraded));
    assert(combat.play_card(0, 0));
    assert(static_cast<int>(combat.hand.size()) == draws);
    assert(combat.exhaust_pile.size() == 1);
    assert(combat.player.energy == 3);
}

} // namespace

int main() {
    test_flash_of_steel(false, 5);
    test_flash_of_steel(true, 8);
    test_peck(false, 3);
    test_peck(true, 4);
    test_production(false, 2);
    test_production(true, 3);
    test_master_of_strategy(false, 3);
    test_master_of_strategy(true, 4);
    std::cout << "[PASS] Cards batch B: Flash of Steel, Peck, Production, Master of Strategy" << std::endl;
    return 0;
}
